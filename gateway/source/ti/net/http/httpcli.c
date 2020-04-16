/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <pthread.h>
#include <string.h>

#ifndef __IAR_SYSTEMS_ICC__
#include <strings.h>
#endif

#include <ti/net/common.h>
#include "httpcli.h"

#ifndef NET_SL
#include <errno.h>
#endif

/* Configurable lengths */
#define CONTENT_BUFLEN 128
#define URI_BUFLEN 128
#define SEND_BUFLEN 256
#define DOMAIN_BUFLEN 64
#define MAX_FIELD_NAME_LEN 24

/* Minimum size required for HTTP status */
#define STATUS_BUFLEN 16

#define HTTP_PREFIX "http://"
#define HTTPS_PREFIX "https://"

/* HTTP client instance state flags */
#define READ_FLAG       (0x01)
#define REDIRECT_FLAG   (0x02)
#define CHUNKED_FLAG    (0x04)
#define INPROGRESS_FLAG (0x08)
/* Proxy address */
static struct sockaddr proxyAddr = {0};

static ssize_t bufferedRecv(HTTPCli_Handle cli, Ssock_Handle ssock,
        void *buf, size_t len, int flags);
static int checkContentField(HTTPCli_Handle cli, char *fname, char *fvalue,
        bool moreFlag);
static int getChunkedData(HTTPCli_Handle cli, char *body, int len,
        bool *moreFlag);
static int getStatus(HTTPCli_Handle cli);
static int lookUpResponseFields(HTTPCli_Handle cli, char *field);
static int readLine(HTTPCli_Handle cli, char *line, int len, bool *moreFlag);
static int skipLine(HTTPCli_Handle cli);
static int sprsend(HTTPCli_Handle cli, const char *fmt, ...);

#if defined(NET_NDK) || defined(__linux__)
static const char *getRequestFieldValue(HTTPCli_Handle cli, const char *name);
static int httpProxyTunnel(HTTPCli_Handle cli);
#endif

#ifndef HTTPCli_LIBTYPE_MIN
static void *asyncThread(void *arg);
static void contentHandler(HTTPCli_Handle cli, int status);
static int redirect(HTTPCli_Handle cli, int status);
#endif

/*
 *  ======== getErrno ========
 *  Get errno from the socket layer
 */
static int getErrno(int ret)
{
    if (ret == -1) {
        return (errno);
    }
    else {
        return (ret);
    }
}

/*
 *  ======== getCliState ========
 *  Get the state of HTTP client instance for the input flag
 */
static inline bool getCliState(HTTPCli_Handle cli, int flag)
{
    return ((cli->state) & flag);
}

/*
 *  ======== setCliState ========
 *  Set or clear the flag in the HTTP client instance
 */
static inline void setCliState(HTTPCli_Handle cli, int flag, bool value)
{
    if (value) {
        cli->state |= flag;
    }
    else {
        cli->state &= ~flag;
    }
}

#ifndef HTTPCli_LIBTYPE_MIN
/*
 *  ======== is1xx ========
 *  Check if the HTTP status code is informational codes
 */
static inline int is1xx(int status)
{
    return ((status >= HTTPStd_CONTINUE) && (status < HTTPStd_OK));
}

/*
 *  ======== is2xx ========
 *  Check if the HTTP status code is success codes
 */
static inline int is2xx(int status)
{
    return ((status >= HTTPStd_OK) && (status < HTTPStd_MULTIPLE_CHOICES));
}

/*
 *  ======== is3xx ========
 *  Check if the HTTP status code is redirectional codes
 */
static inline int is3xx(int status)
{
    return ((status >= HTTPStd_MULTIPLE_CHOICES)
            && (status < HTTPStd_BAD_REQUEST));
}

/*
 *  ======== asyncThread ========
 *  Thread handling HTTP response asynchronously
 */
static void *asyncThread(void *arg)
{
    int status;
    HTTPCli_Handle cli;

    cli = (HTTPCli_Handle) arg;
    status = getStatus(cli);

    if (is2xx(status)  || (status == HTTPStd_NOT_MODIFIED)) {
        if (cli->chandle != NULL) {
            contentHandler(cli, status);
        }
        else {
            cli->shandle->handle2xx(cli, status);
        }
    }
    else if (is1xx(status)) {
        cli->shandle->handle1xx(cli, status);
    }
    else {
        cli->shandle->handle4xx(cli, status);
    }

    pthread_exit(NULL);

    return (NULL);
}

/*
 *  ======== contentHandler ========
 *  Invokes registered content handles based on response content type
 */
static void contentHandler(HTTPCli_Handle cli, int status)
{
    bool moreFlag = false;
    char respBuf[CONTENT_BUFLEN];
    int i = 0;
    int ret = 0;
    HTTPCli_ContentHandler *chandle;
    HTTPCli_ContentCallback callback = NULL;
    char **respFields = NULL;
    const char *fields[2] = {
        HTTPStd_FIELD_NAME_CONTENT_TYPE,
        NULL
    };

    if ((status == HTTPStd_OK) && cli->chandle) {
        /* Save the previously set field Ids by the app */
        respFields = HTTPCli_setResponseFields(cli, fields);

        do {
            ret = HTTPCli_getResponseField(cli, respBuf, sizeof(respBuf),
                    &moreFlag);

            /* Zero is the index to HTTPStd_FIELD_NAME_CONTENT_TYPE */
            if (ret == 0) {
                if (moreFlag) {
                    status = HTTPCli_ECONTENTTYPELONG;
                    cli->shandle->handle2xx(cli, status);
                    return;
                }

                chandle = cli->chandle;
                for (i = 0; chandle[i].contentType != NULL; i++) {
                    ret = strncmp(chandle[i].contentType, respBuf,
                            sizeof(respBuf));
                    if (ret == 0) {
                        callback = chandle[i].handle;
                        break;
                    }
                }
            }
        }
        while (ret != HTTPCli_FIELD_ID_END);

        if (respFields) {
            HTTPCli_setResponseFields(cli, (const char **)respFields);
        }
        else {
            /* force assign */
            cli->respFields = respFields;
        }

        if (callback == NULL) {
            status = HTTPCli_ENOCONTENTCALLBACK;
            cli->shandle->handle2xx(cli, status);
            return;
        }

        do {
            moreFlag = false;
            ret = HTTPCli_readResponseBody(cli, respBuf, sizeof(respBuf),
                    &moreFlag);
            if (ret < 0) {
                status = ret;
            }
            i = callback(cli, status, respBuf, ret, moreFlag);
        }
        while ((ret > 0) && moreFlag && i);

        /* If the app wants to flush the remaining data */
        if ((ret >= 0) && (i == 0)) {
            do {
                ret = HTTPCli_readResponseBody(cli, respBuf, sizeof(respBuf),
                        &moreFlag);
                if (ret < 0) {
                    status = ret;
                    callback(cli, status, respBuf, ret, moreFlag);
                }
            }
            while ((ret > 0) && moreFlag);
        }
    }
    else {
        cli->shandle->handle2xx(cli, status);
    }

}
#endif

/*
 *  ======== bufferedRecv ========
 */
static ssize_t bufferedRecv(HTTPCli_Handle cli, Ssock_Handle ssock,
        void *buf, size_t len, int flags)
{
    ssize_t numRead = 0;

#ifdef NET_SLP
    if (len > HTTPCli_BUF_LEN) {
        return (HTTPCli_EINTERNALBUFSMALL);
    }

    if (cli->buflen < len) {
        if (cli->buflen) {
            memcpy(buf, cli->bufptr, cli->buflen);
            buf = (uint8_t *)buf + cli->buflen;
        }

        numRead = Ssock_recv(ssock, cli->buf, HTTPCli_BUF_LEN, MSG_PEEK);
        if (numRead > 0) {
            numRead = Ssock_recv(ssock, cli->buf, numRead, flags);
        }

        if (numRead >= 0) {
            if (numRead < (len - cli->buflen)) {
                if (cli->buflen) {
                    memcpy(buf, cli->buf, numRead);
                    /* Not enough bytes yet, copy into the internal buf */
                    cli->buflen += numRead;
                    memcpy(cli->buf, buf, cli->buflen);
                }
                else {
                    cli->buflen = numRead;
                }

                cli->bufptr = cli->buf;
                numRead = HTTPCli_EINPROGRESS;
            }
            else {
                memcpy(buf, cli->buf, (len - cli->buflen));
                cli->bufptr = cli->buf + (len - cli->buflen);
                cli->buflen = numRead - (len - cli->buflen);
                numRead = len;
            }
        }
        else if (numRead == Ssock_TIMEOUT) {
            numRead = HTTPCli_EINPROGRESS;
        }
    }
    else {
        memcpy(buf, cli->bufptr, len);
        cli->bufptr += len;
        cli->buflen -= len;
        numRead = len;
    }

    if (numRead == HTTPCli_EINPROGRESS) {
        return (numRead);
    }

#else

    if (cli->buflen) {
        if (len > cli->buflen) {
            memcpy(buf, cli->bufptr, cli->buflen);

            numRead = Ssock_recv(ssock, ((uint8_t *)buf + cli->buflen),
                    (len - cli->buflen), flags);
            if (numRead >= 0) {
                numRead += cli->buflen;
                cli->buflen = 0;
            }
        }
        else {
            memcpy(buf, cli->bufptr, len);
            cli->buflen -= len;
            cli->bufptr += len;
            numRead = len;
        }
    }
    else {
        if (len > HTTPCli_BUF_LEN) {
            numRead = Ssock_recv(ssock, buf, len, flags);
        }
        else {
            numRead = Ssock_recv(ssock, cli->buf, HTTPCli_BUF_LEN,
                    MSG_DONTWAIT);
            if (numRead > 0) {
                cli->bufptr = cli->buf;
                cli->buflen = numRead;
            }

            if (numRead == Ssock_TIMEOUT || (numRead >= 0 && numRead < len)) {
                if (numRead == Ssock_TIMEOUT) {
                    numRead = 0;
                }

                numRead = Ssock_recv(ssock, (cli->buf + numRead),
                        (len - numRead), flags);
                if (numRead > 0) {
                    cli->buflen += numRead;
                }
            }

            if (numRead >= 0) {
                if (len > cli->buflen) {
                    len = cli->buflen;
                }

                memcpy(buf, cli->buf, len);
                cli->buflen -= len;
                cli->bufptr += len;
                numRead = len;
            }
        }
    }

#endif

    if (numRead < 0) {
        if (numRead != Ssock_TIMEOUT) {
            cli->sockerr = getErrno(numRead);
        }

        return (HTTPCli_ERECVFAIL);
    }

    return (numRead);
}

/*
 *  ======== checkContentField ========
 *  Check field for information on content length/transfer encoding
 */
static int checkContentField(HTTPCli_Handle cli, char *fname, char *fvalue,
        bool moreFlag)
{
    const char chunk[] = "chunked";

    if ((strcasecmp(fname, HTTPStd_FIELD_NAME_TRANSFER_ENCODING) == 0)
            && (strcasecmp(chunk, fvalue) == 0)) {
        setCliState(cli, CHUNKED_FLAG, true);
    }
    else if (strcasecmp(fname, HTTPStd_FIELD_NAME_CONTENT_LENGTH) == 0) {
        if (moreFlag) {
            return (HTTPCli_ECONTENTLENLARGE);
        }
        cli->clen = strtoul(fvalue, NULL, 10);
    }

    return (0);
}

/*
 *  ======== getChunkedData ========
 *  Get chunked transfer encoded data
 */
static int getChunkedData(HTTPCli_Handle cli, char *body, int len,
        bool *moreFlag)
{
    bool lastFlag = true;
    bool mFlag = false;
    char crlf;
    int ret = 0;
    unsigned long chunkLen;

    if (!(getCliState(cli, CHUNKED_FLAG))) {
        return (HTTPCli_ENOTCHUNKDATA);
    }

    *moreFlag = true;

    /* Check if in the middle of reading respone body? */
    chunkLen = cli->clen;
    if (chunkLen == 0) {
        ret = readLine(cli, body, len, &mFlag);
        if (ret < 0) {
            return (ret);
        }

        chunkLen = strtoul(body, NULL, 16);
        /* don't need the remaining buffer */
        if (mFlag) {
            ret = readLine(cli, body, len, &mFlag);
            if (ret < 0) {
                return (ret);
            }
        }

        if (chunkLen == 0) {
            /* skip the rest (trailer headers) */
            do {
                ret = readLine(cli, body, len, &mFlag);
                if (ret < 0) {
                    return (ret);
                }

                /* Avoids fake do-while check */
                if (lastFlag) {
                    body[0] = 'a';
                    lastFlag = false;
                }

                if (mFlag) {
                    lastFlag = true;
                }

            }
            while (body[0] == '\0');

            *moreFlag = false;
            cli->clen = 0;

            return (0);
        }

        cli->clen = chunkLen;
    }

    if (chunkLen < len) {
        len = chunkLen;
    }

    ret = HTTPCli_readRawResponseBody(cli, body, len);
    if (ret > 0) {
        cli->clen -= ret;
        /* Clean up the CRLF */
        if (cli->clen == 0) {
            chunkLen = readLine(cli, &crlf, sizeof(crlf), &mFlag);
            if (chunkLen != 0) {
                return (chunkLen);
            }
        }
    }

    return (ret);
}

/*
 *  ======== getRequestFieldValue ========
 */
#if defined(NET_NDK) || defined(__linux__)
static const char *getRequestFieldValue(HTTPCli_Handle cli, const char *name)
{
    int i;
    HTTPCli_Field *fields;

    fields = cli->fields;
    if (fields) {
        for (i = 0; fields[i].name != NULL; i++) {
            if (strcmp(fields[i].name, name) == 0) {
                return (fields[i].value);
            }
        }
    }

    return (NULL);
}
#endif

/*
 *  ======== getStatus ========
 *  Processes the response line to get the status
 */
static int getStatus(HTTPCli_Handle cli)
{
    bool moreFlag = false;
    char statusBuf[STATUS_BUFLEN];
    int status;
    int vlen;
    int rflag;

    do {
        /* Set redirect repeat getStatus flag to zero */
        rflag = 0;

        vlen = strlen(HTTPStd_VER);
        status = bufferedRecv(cli, &(cli->ssock), statusBuf, vlen, 0);
        if (status < 0) {
            return (status);
        }

        /* match HTTP/1. */
        if (strncmp(statusBuf, HTTPStd_VER, (vlen - 1))) {
            /* not a valid HTTP header - give up */
            return (HTTPCli_ERESPONSEINVALID);
        }
        /* get the numeric status code (and ignore the readable status) */
        status = readLine(cli, statusBuf, sizeof(statusBuf), &moreFlag);

        /* Skip the rest of the status */
        if ((status == 0) && moreFlag) {
            status = skipLine(cli);
        }

        if (status == 0) {
            status = strtoul(statusBuf, NULL, 10);
        }

#ifndef HTTPCli_LIBTYPE_MIN
        if (cli->rhandle && is3xx(status)) {
            rflag = redirect(cli, status);
            if (rflag < 0) {
                status = rflag;
                rflag = 0;
            }
        }
#endif
    }
    while (rflag);

    return (status);
}

/*
 *  ======== httpProxyTunnel ========
 *  HTTP tunnel through proxy
 */
#if defined(NET_NDK) || defined(__linux__)
static int httpProxyTunnel(HTTPCli_Handle cli)
{
    bool moreFlag;
    char tmpBuf[STATUS_BUFLEN];
    const char *host;
    int ret = 0;

    host = getRequestFieldValue(cli, HTTPStd_FIELD_NAME_HOST);
    if (host == NULL) {
        return (HTTPCli_EHOSTFIELDNOTFOUND);
    }

    ret = sprsend(cli, "%s %s %s\r\n", HTTPStd_CONNECT, host, HTTPStd_VER);
    if (ret < 0) {
        return (ret);
    }

    ret = sprsend(cli, "%s: %s\r\n", HTTPStd_FIELD_NAME_HOST, host);
    if (ret < 0) {
        return (ret);
    }

    ret = HTTPCli_sendField(cli, NULL, NULL, true);
    if (ret < 0) {
        return (ret);
    }

    ret = getStatus(cli);
    if (ret != HTTPStd_OK) {
        if (ret < 0) {
            return (ret);
        }
        return (HTTPCli_EPROXYTUNNELFAIL);
    }

    /* Drop all the fields */
    do {
        ret = readLine(cli, tmpBuf, sizeof(tmpBuf), &moreFlag);
        if (ret < 0) {
            return (ret);
        }
    } while (tmpBuf[0] != '\0');

    return (0);
}
#endif

/*
 *  ======== lookUpResponseFields ========
 *  Is the input field in the set response field array?
 */
static int lookUpResponseFields(HTTPCli_Handle cli, char *field)
{
    char **respFields = cli->respFields;
    int index;

    if (respFields && field) {
        for (index = 0; respFields[index] != NULL; index++) {
             if (strcasecmp(field, respFields[index]) == 0) {
               return (index);
            }
        }
    }

    return (-1);
}

/*
 *  ======== readLine ========
 *  Read a header line
 */
static int readLine(HTTPCli_Handle cli, char *line, int len, bool *moreFlag)
{
    char c;
    int i = 0;
    int ret;

    *moreFlag = true;
    setCliState(cli, READ_FLAG, true);

    for (i = 0; i < len; i++) {

        ret = bufferedRecv(cli, &(cli->ssock), &c, 1, 0);
        if (ret < 0) {
            *moreFlag = false;
            setCliState(cli, READ_FLAG, false);
            return (ret);
        }

        if (c == '\n') {
            line[i] = 0;
            /* Line read completed */
            *moreFlag = false;
            setCliState(cli, READ_FLAG, false);
            break;
        }
        else if ((c == '\r') || (i == 0 && c == ' ')) {
            i--;
            /* drop CR or drop leading SP*/
        }
        else {
            line[i] = c;
        }

    }

    return (0);
}

#ifndef HTTPCli_LIBTYPE_MIN
static int redirect(HTTPCli_Handle cli, int status)
{
    bool moreFlag = false;
    char uri[URI_BUFLEN] = {0};
    char buf[HTTPCli_RECV_BUFLEN] = {0};
    int ret = 0;
    char **respFields;
    const char *fields[2] = {
        HTTPStd_FIELD_NAME_LOCATION,
        NULL
    };

    switch (status) {
        case HTTPStd_MOVED_PERMANENTLY:
        case HTTPStd_FOUND:
        case HTTPStd_SEE_OTHER:
        case HTTPStd_USE_PROXY:
        case HTTPStd_TEMPORARY_REDIRECT:
            respFields = HTTPCli_setResponseFields(cli, fields);

            ret = HTTPCli_getResponseField(cli, uri, sizeof(uri),
                    &moreFlag);
            if (ret < 0) {
                return (ret);
            }

            if (moreFlag) {
                return (HTTPCli_EREDIRECTURILONG);
            }

            do {
                ret = HTTPCli_getResponseField(cli, buf, sizeof(buf),
                        &moreFlag);
                if ((ret != HTTPCli_FIELD_ID_END) && (ret < 0)) {
                    return (ret);
                }
            }
            while (ret != HTTPCli_FIELD_ID_END);

            if (respFields) {
                HTTPCli_setResponseFields(cli, (const char **)respFields);
            }
            else {
                /* force assign */
                cli->respFields = respFields;
            }

            do {
                ret = HTTPCli_readResponseBody(cli, buf, sizeof(buf),
                        &moreFlag);
                if (ret < 0) {
                    return (ret);
                }
            }
            while ((ret > 0) && moreFlag);

            setCliState(cli, REDIRECT_FLAG, true);
            cli->rhandle(cli, status, uri);
            setCliState(cli, REDIRECT_FLAG, false);

            ret = 1;
            break;

        default:
            ret = 0;
            break;
    }

    return (ret);
}
#endif

/*
 *  ======== skipLine ========
 *  Skip the rest of the header line
 */
static int skipLine(HTTPCli_Handle cli)
{
    char c = 0;
    int ret;

    while (c != '\n') {
        ret = bufferedRecv(cli, &(cli->ssock), &c, 1, 0);
        if (ret < 0) {
            return (ret);
        }
    }

    return (0);
}

/*
 *  ======== sprsend ========
 *  Constructs an HTTP Request line to send
 */
static int sprsend(HTTPCli_Handle cli, const char * fmt, ...)
{
    char sendbuf[SEND_BUFLEN];
    int len = 0;
    int ret;
    int sendbuflen = sizeof(sendbuf);
    va_list ap;

    if (!getCliState(cli, INPROGRESS_FLAG)) {
        va_start(ap, fmt);
        len = xvsnprintf(sendbuf, sendbuflen, fmt, ap);
        va_end(ap);
    }
#ifdef NET_SLP
    else {
        memcpy(sendbuf, cli->buf, cli->buflen);
        len = cli->buflen;
    }
#endif

    if (len > sendbuflen) {
        return (HTTPCli_ESENDBUFSMALL);
    }

    ret = Ssock_send(&(cli->ssock), sendbuf, len, 0);
    if (ret < 0) {
        cli->sockerr = getErrno(ret);

        return (HTTPCli_ESENDFAIL);
    }

#ifdef NET_SLP
    if (ret < len) {
        cli->buflen = len - ret;
        if (cli->buflen > HTTPCli_BUF_LEN) {
            return (HTTPCli_EINTERNALBUFSMALL);
        }

        memcpy(cli->buf, ((uint8_t *)sendbuf + ret), cli->buflen);
        return (HTTPCli_EINPROGRESS);
    }
    cli->buflen = 0;

#endif

    return (ret);
}

/*
 *  ======== HTTPCli_Params_init ========
 */
void HTTPCli_Params_init(HTTPCli_Params *params)
{
    if (params) {
        memset(params, 0, sizeof(HTTPCli_Params));
    }
}

/*
 *  ======== HTTPCli_initSockAddr ========
 */
#ifndef NET_SLP
int HTTPCli_initSockAddr(struct sockaddr *addr, const char *uri, int flags)
{
    char luri[DOMAIN_BUFLEN];
    char *domain;
    char *delim;
    int dlen;
    int port = HTTPStd_PORT;
    xassert(addr != NULL);
    xassert(uri != NULL);

    domain = (char *)uri;

    if (strncasecmp(HTTP_PREFIX, domain, (sizeof(HTTP_PREFIX) - 1))
            == 0) {
        port = HTTPStd_PORT;
        domain = domain + sizeof(HTTP_PREFIX) - 1;
    }
    else if (strncasecmp(HTTPS_PREFIX, domain, (sizeof(HTTPS_PREFIX) - 1))
            == 0) {
        port = HTTPStd_SECURE_PORT;
        domain = domain + sizeof(HTTPS_PREFIX) - 1;
    }

    delim = strchr(domain, ':');
    if (delim != NULL) {
        dlen = delim - domain;
        port = strtoul((delim + 1), NULL, 10);
    }
    else {
        delim = strchr(domain, '/');
        if (delim != NULL) {
            dlen = delim - domain;
        }
        else {
            dlen = strlen(domain);
        }
    }

    if (dlen >= DOMAIN_BUFLEN) {
        return (HTTPCli_EURILENLONG);
    }

    strncpy(luri, domain, dlen);
    domain = luri;
    domain[dlen] = '\0';

#ifdef NET_SL
    int ret = 0;
    struct sockaddr_in taddr = {0};
    unsigned long ip;

    /* SL supports IPv4 DNS only */
    if (flags & HTTPCli_TYPE_IPV6) {
        return (HTTPCli_EHOSTNAME);
    }
    else {
        ret = sl_NetAppDnsGetHostByName((signed char *)domain, dlen, &ip,
                AF_INET);
        if (ret < 0) {
            return (HTTPCli_EHOSTNAME);
        }

        taddr.sin_family = AF_INET;
        taddr.sin_port = htons(port);
        taddr.sin_addr.s_addr = htonl(ip);
        *addr = *((struct sockaddr *)&taddr);
    }

    return (0);

#elif defined(__linux__)  || defined(NET_NDK)
    int ret = 0;
    struct addrinfo hints;
    struct addrinfo *addrs;

    memset(&hints, 0, sizeof(struct addrinfo));
    if (flags & HTTPCli_TYPE_IPV6) {
        hints.ai_family = AF_INET6;
    }
    else {
        hints.ai_family = AF_INET;
    }

    ret = getaddrinfo(domain, "0", &hints, &addrs);
    if (ret != 0) {
        return (HTTPCli_EHOSTNAME);
    }

    *addr = *(addrs->ai_addr);

    if (flags & HTTPCli_TYPE_IPV6) {
        ((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
    }
    else {
        ((struct sockaddr_in *)addr)->sin_port = htons(port);
    }
    freeaddrinfo(addrs);

    return (0);

#else

#error "Unsupported network stack"

#endif
}
#endif

/*
 *  ======== HTTPCli_connect ========
 */
int HTTPCli_connect(HTTPCli_Handle cli, const struct sockaddr *addr,
        int flags, const HTTPCli_Params *params)
{
    int skt = 0;
    int ret;
    int slen;
    int sopt = 0;
    struct sockaddr *sa;
    Ssock_Struct ssock;

#ifndef NET_SLP
    struct timeval tv;
#endif

    xassert(cli != NULL);

    if (proxyAddr.sa_family != 0) {
        sa = &proxyAddr;
    }
    else {
        xassert(addr != NULL);
        sa = (struct sockaddr *)addr;
    }

    if (sa->sa_family == AF_INET6) {
        slen = sizeof(struct sockaddr_in6);
    }

#ifndef NET_SLP
    else {
        slen = sizeof(struct sockaddr_in);
    }
#else
    else {
        return (HTTPCli_ESOCKETFAIL);
    }
#endif

    if (!getCliState(cli, INPROGRESS_FLAG)) {

        if (params != NULL) {
            cli->tls = params->tls;

#ifndef HTTPCli_LIBTYPE_MIN
            cli->shandle = params->shandle;
            cli->chandle = params->chandle;
            cli->rhandle = params->rhandle;

#ifndef __linux__
            cli->stackSize = params->stackSize;
            cli->priority  = params->priority;
#endif

            if (cli->chandle) {
                xassert(cli->shandle != NULL);
            }

            if (cli->shandle) {
                xassert(params->shandle->handle1xx != NULL);
                xassert(params->shandle->handle2xx != NULL);
                xassert(params->shandle->handle4xx != NULL);
            }
#endif
        }

#ifdef NET_SL
        if (cli->tls) {
            sopt = SL_SEC_SOCKET;
        }
#endif

        skt = socket(sa->sa_family, SOCK_STREAM, sopt);
        if (skt != -1) {
            Ssock_construct(&ssock, skt);
            cli->ssock = ssock;

#ifndef NET_SLP
            if (params != NULL && params->timeout) {
                tv.tv_sec = params->timeout;
                tv.tv_usec = 0;
                if ((ret = setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, &tv,
                        sizeof(tv))) < 0) {
                    HTTPCli_disconnect(cli);
                    cli->sockerr = getErrno(ret);

                    return (HTTPCli_ESOCKETFAIL);
                }
            }
#endif

#ifdef NET_SL
            /* In SL, the secure params have to be set before connect */
            if (cli->tls) {
                ret = Ssock_startTLS(&(cli->ssock), cli->tls);
                if (ret < 0) {
                    cli->sockerr = getErrno(ret);
                    HTTPCli_disconnect(cli);

                    return (HTTPCli_ETLSFAIL);
                }
            }
#endif
        }
        else {
            cli->sockerr = getErrno(skt);

            return (HTTPCli_ESOCKETFAIL);
        }
    }

    ret = connect(skt, sa, slen);
#ifdef NET_SL
    /*
     *  SL returns unknown root CA error code if the CA certificate
     *  is not found in its certificate store. This is a warning
     *  code and not an error. So this error code is being ignored
     *  till a better alternative is found.
     */
    if ((ret < 0) && (getErrno(ret) != SL_ERROR_BSD_ESECUNKNOWNROOTCA)) {
#else
    if (ret < 0) {
#endif
#ifdef NET_SLP
        if (errno(skt) == EINPROGRESS) {
            setCliState(cli, INPROGRESS_FLAG, true);

            return (HTTPCli_EINPROGRESS);
        }
#endif
        HTTPCli_disconnect(cli);
        cli->sockerr = getErrno(ret);

        return (HTTPCli_ECONNECTFAIL);
    }
    setCliState(cli, INPROGRESS_FLAG, false);

#if !defined(NET_SL) && !defined(NET_SLP)
    /* In WOLFSSL, the secure params are set after connect */
    if (cli->tls) {

        /* Tunnel using HTTP CONNECT for TLS through proxy */
        if (proxyAddr.sa_family != 0)  {
            ret = httpProxyTunnel(cli);
            if (ret < 0) {
                return (ret);
            }
        }

        ret = Ssock_startTLS(&(cli->ssock), cli->tls);
        if (ret < 0) {
            cli->sockerr = getErrno(ret);
            HTTPCli_disconnect(cli);

            return (HTTPCli_ETLSFAIL);
        }
    }

#endif

#ifdef NET_SLP
    if (params->rnotify) {
        ret = setsocknotify(skt, SO_READNOTIFY, params->rnotify, cli);
        if (ret < 0) {
            return (HTTPCli_ESETNOTIFYFAIL);
        }
    }

    if (params->wnotify) {
        ret = setsocknotify(skt, SO_WRITENOTIFY, params->wnotify, cli);
        if (ret < 0) {
            return (HTTPCli_ESETNOTIFYFAIL);
        }
    }

    if (params->enotify) {
        ret = setsocknotify(skt, SO_EXCEPTNOTIFY, params->enotify, cli);
        if (ret < 0) {
            return (HTTPCli_ESETNOTIFYFAIL);
        }
    }
#endif


    return (0);
}

/*
 *  ======== HTTPCli_construct ========
 */
void HTTPCli_construct(HTTPCli_Struct *cli)
{
    xassert(cli != NULL);

    HTTPCli_destruct(cli);
}

/*
 *  ======== HTTPCli_create ========
 */
HTTPCli_Handle HTTPCli_create()
{
    HTTPCli_Handle cli;

    cli = (HTTPCli_Handle)xmalloc(sizeof(HTTPCli_Struct));
    if (cli != NULL) {
        HTTPCli_construct(cli);
    }

    return (cli);
}

/*
 *  ======== HTTPCli_delete ========
 */
void HTTPCli_delete(HTTPCli_Handle *cli)
{
    xassert(*cli != NULL);

    HTTPCli_destruct(*cli);
    xfree(*cli);
    *cli = NULL;
}

/*
 *  ======== HTTPCli_destruct ========
 */
void HTTPCli_destruct(HTTPCli_Struct *cli)
{
    xassert(cli != NULL);

    memset(cli, 0, sizeof(HTTPCli_Struct));
    cli->bufptr = cli->buf;
}

/*
 *  ======== HTTPCli_disconnect ========
 */
void HTTPCli_disconnect(HTTPCli_Handle cli)
{
    int skt;

    xassert(cli != NULL);

    skt = Ssock_getSocket(&(cli->ssock));
    Ssock_destruct(&(cli->ssock));
    close(skt);

    HTTPCli_destruct(cli);
}

/*
 *  ======== HTTPCli_setRequestFields ========
 */
HTTPCli_Field *HTTPCli_setRequestFields(HTTPCli_Handle cli,
        const HTTPCli_Field *fields)
{
    HTTPCli_Field *prevField;

    xassert(cli != NULL);

    prevField = cli->fields;
    if (fields) {
        cli->fields = (HTTPCli_Field *)fields;
    }

    return (prevField);
}

/*
 *  ======== HTTPCli_setResponseFields ========
 */
char **HTTPCli_setResponseFields(HTTPCli_Handle cli, const char *fields[])
{
    char **prevField;

    xassert(cli != NULL);

    prevField = cli->respFields;
    if (fields) {
        cli->respFields = (char **)fields;
    }

    return (prevField);
}

/*
 *  ======== HTTPCli_sendRequest ========
 */
int HTTPCli_sendRequest(HTTPCli_Handle cli, const char *method,
        const char *requestURI, bool moreFlag)
{
    int i = 0;
    int ret;
    HTTPCli_Field *fields = NULL;

    xassert(cli != NULL);
    xassert(method != NULL);
    xassert(requestURI != NULL);

    ret = sprsend(cli, "%s %s %s\r\n", method, requestURI? requestURI : "/",
            HTTPStd_VER);
    if (ret < 0) {
        return (ret);
    }

    if (cli->fields) {
        fields = cli->fields;
        for (i = 0; fields[i].name != NULL; i++) {
            ret = HTTPCli_sendField(cli, fields[i].name, fields[i].value,
                    false);
            if (ret < 0) {
                return (ret);
            }
        }
    }

    if (!moreFlag) {
        ret = HTTPCli_sendField(cli, NULL, NULL, true);
        if (ret < 0) {
            return (ret);
        }
    }

    return (0);
}

/*
 *  ======== HTTPCli_sendField ========
 */
int HTTPCli_sendField(HTTPCli_Handle cli, const char *name,
        const char *value, bool lastFlag)
{
    int ret;

#ifndef HTTPCli_LIBTYPE_MIN
    pthread_t thread;
    pthread_attr_t pattrs;
#ifndef __linux__
    struct sched_param priParam;
#endif
#endif

    xassert(cli != NULL);

    if (name != NULL) {
        ret = sprsend(cli, "%s: %s\r\n", name, value);
        if (ret < 0) {
            return (ret);
        }
    }

    if (lastFlag) {
        ret = sprsend(cli, "\r\n");
        if (ret < 0) {
            return (ret);
        }

#ifndef HTTPCli_LIBTYPE_MIN
        if (cli->shandle && !(getCliState(cli, REDIRECT_FLAG))) {

            ret = pthread_attr_init(&pattrs);
            if (ret != 0) {
                return (HTTPCli_ETHREADFAIL);
            }

            ret = pthread_attr_setdetachstate(&pattrs, PTHREAD_CREATE_DETACHED);
            if (ret != 0) {
                return (HTTPCli_ETHREADFAIL);
            }

#ifndef __linux__
            if (cli->priority) {
                priParam.sched_priority = cli->priority;
                ret = pthread_attr_setschedparam(&pattrs, &priParam);
                if (ret != 0) {
                    return (HTTPCli_ETHREADFAIL);
                }
            }

            if (cli->stackSize) {
                ret = pthread_attr_setstacksize(&pattrs, cli->stackSize);
                if (ret != 0) {
                    return (HTTPCli_ETHREADFAIL);
                }
            }
#endif

            ret = pthread_create(&thread, &pattrs, asyncThread, cli);
            if (ret != 0) {
                return (HTTPCli_ETHREADFAIL);
            }
        }
#endif
    }

    return (0);
}

/*
 *  ======== HTTPCli_sendRequestBody ========
 */
int HTTPCli_sendRequestBody(HTTPCli_Handle cli, const char *body, int len)
{
    int ret;

    xassert(cli != NULL);

    ret = Ssock_send(&(cli->ssock), body, len, 0);
    if (ret < 0) {
        cli->sockerr = getErrno(ret);
        return (ret);
    }

    return (0);
}

/*
 *  ======== HTTPCli_getResponseStatus ========
 */
int HTTPCli_getResponseStatus(HTTPCli_Handle cli)
{
    int ret;

    xassert(cli != NULL);

#ifndef HTTPCli_LIBTYPE_MIN
    if (cli->shandle) {
        return (HTTPCli_EASYNCMODE);
    }
#endif

    ret = getStatus(cli);

    return (ret);
}

/*
 *  ======== HTTPCli_getResponseField ========
 */
int HTTPCli_getResponseField(HTTPCli_Handle cli, char *body, int len,
        bool *moreFlag)
{
    char c;
    char name[MAX_FIELD_NAME_LEN] = {0};
    int respFieldIndex = HTTPCli_FIELD_ID_DUMMY;
    int i;
    int ret;

    xassert(cli != NULL);
    xassert(body != NULL);
    xassert(moreFlag != NULL);

    /* Minimum size required to hold content length value */
    if (len < HTTPCli_RECV_BUFLEN) {
        return (HTTPCli_ERECVBUFSMALL);
    }

    if (!(getCliState(cli, READ_FLAG))) {
        while (1) {
            for (i = 0; i < MAX_FIELD_NAME_LEN; i++) {

                ret = bufferedRecv(cli, &(cli->ssock), &c, 1, 0);
                if (ret < 0) {
                    return (ret);
                }

                if (c == ':') {
                    name[i] = 0;
                    break;
                }
                else if (c == ' ' || c == '\r') {
                    i--;
                    /* drop SP */
                }
                else if (c == '\n') {
                    return (HTTPCli_FIELD_ID_END);
                }
                else {
                    name[i] = c;
                }
            }

            /* We cannot recognise this header, its too big. Skip it */
            if ((i == MAX_FIELD_NAME_LEN) && (name[i - 1] != 0)) {
                ret = skipLine(cli);
                if (ret < 0) {
                    return (ret);
                }
                continue;
            }

            respFieldIndex = lookUpResponseFields(cli, name);
            ret = readLine(cli, body, len, moreFlag);
            if (ret < 0) {
                return (ret);
            }

            ret = checkContentField(cli, name, body, *moreFlag);
            if (ret < 0) {
                return (ret);
            }

            if (respFieldIndex >= 0) {
                break;
            }

            if (*moreFlag) {
                ret = skipLine(cli);
                if (ret < 0) {
                    return (ret);
                }
            }
        }
    }
    else {
        /* Read field value */
        ret = readLine(cli, body, len, moreFlag);
        if (ret < 0) {
            return (ret);
        }
    }

    return (respFieldIndex);
}

/*
 *  ======== HTTPCli_readResponseHeader ========
 */
int HTTPCli_readResponseHeader(HTTPCli_Handle cli, char *header, int len,
        bool *moreFlag)
{
    char *value = NULL;
    int ret;
    int status;
    int i;

    xassert(cli != NULL);
    xassert(header != NULL);
    xassert(moreFlag != NULL);

    /* Minimum size required to hold content length value */
    if (len < HTTPCli_RECV_BUFLEN) {
        return (HTTPCli_ERECVBUFSMALL);
    }

    *moreFlag = false;
    for (i = 0; i < len; i++) {
        ret = bufferedRecv(cli, &(cli->ssock), header + i, 1, 0);
        if (ret <= 0) {
            break;
        }

        if (header[i] == ':') {
            value = header + i;
        }
        else if ((header[i] == '\n') && (header[i - 1] == '\r')) {
            header[i - 1] = '\0';
            break;
        }
    }

    if (ret >= 0) {

        ret = i;
        /*  Check if the entire data is read */
        if (header[ret - 1] == '\0') {
           /* If this is the end of headers */
           if (!getCliState(cli, READ_FLAG) && (ret == 1)) {
               ret = 0;
           }
           else {
               setCliState(cli, READ_FLAG, false);
           }
        }
        else {
           setCliState(cli, READ_FLAG, true);
           *moreFlag = true;
        }

        if (value != NULL) {
            *value = '\0';

            /* field value shouldn't have leading spaces */
            i = 1;
            while (*(value + i) == ' ') {
                i++;
            }

            status = checkContentField(cli, header, (value + i), *moreFlag);
            *value = ':';

            if (status < 0) {
                ret = status;
                status = skipLine(cli);
                if (status < 0) {
                    ret = status;
                }
            }
        }
    }

    return (ret);
}

/*
 *  ======== HTTPCli_readResponseBody ========
 */
int HTTPCli_readResponseBody(HTTPCli_Handle cli, char *body, int len,
        bool *moreFlag)
{
    int ret = 0;

    xassert(cli != NULL);

    *moreFlag = false;
    if (getCliState(cli, CHUNKED_FLAG)) {
        ret = getChunkedData(cli, body, len, moreFlag);
    }
    else {
        if (cli->clen) {
            if (cli->clen < len) {
                len = cli->clen;
            }
            ret = HTTPCli_readRawResponseBody(cli, body, len);
            if (ret > 0) {
                cli->clen -= ret;
                *moreFlag = cli->clen ? true : false;
            }
        }
    }

    return (ret);
}

/*
 *  ======== HTTPCli_readRawResponseBody ========
 */
int HTTPCli_readRawResponseBody(HTTPCli_Handle cli, char *body, int len)
{
    int ret = 0;

    xassert(cli != NULL);

    ret = bufferedRecv(cli, &(cli->ssock), body, len, 0);

    return (ret);
}

/*
 *  ======== HTTPCli_setProxy ========
 */
void HTTPCli_setProxy(const struct sockaddr *addr)
{
    proxyAddr = *addr;
}

/*
 *  ======== HTTPCli_getSocketError ========
 */
int HTTPCli_getSocketError(HTTPCli_Handle cli)
{
    return (cli->sockerr);
}
