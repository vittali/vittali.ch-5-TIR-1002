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
#include <string.h>

#include <ti/net/common.h>

#include "http2utils.h"
#include "http2cli.h"

#ifdef NET_SL
#define TLS_SOCKET_OPT SL_SEC_SOCKET
#else
#include <errno.h>
#define TLS_SOCKET_OPT 0
#endif

#define NULL_CHAR_LEN  (1)

#define SETTINGS_PUSH_DISABLE            (0)
#define FRAME_RECV_TIMEOUT_MILLISECS     (5000)
#define GOAWAY_SERVER_LAST_STREAM_ID     (0)

#define URI_PREFIX "https"
#define URI_SCHEME_SEPARATOR "://"
#define URI_PORT_SEPARATOR ':'
#define URI_PATH_SEPARATOR '/'
#define URI_IPV6_ADDRESS_START '['
#define URI_IPV6_ADDRESS_STOP  ']'

#define USER_AGENT_VALUE "http2cli/1.0"

const HTTP2Std_Settings defaultServerSettings = {
    .headerTableSize = 0,
    .concurrentStreams = HTTP2Std_SETTINGS_CONCURRENT_STREAMS_DEFAULT,
    .windowSize = HTTP2Std_SETTINGS_WINDOW_SIZE_DEFAULT,
    .frameSize = HTTP2Std_SETTINGS_FRAME_SIZE_DEFAULT,
    .headersListSize = HTTP2Std_SETTINGS_HEADER_LIST_SIZE_DEFAULT
};

static uint8_t pingPayload[HTTP2Std_PING_PAYLOAD_LEN] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
};

static void sendConnectionError(HTTP2Cli_Handle cli, uint32_t streamId,
        uint32_t errorType);
static int sendAck(HTTP2Cli_Handle cli, uint8_t type, uint8_t *payload,
        uint32_t len);

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
 *  ======== encodeOctet ========
 *  Converts the integer value into an octet array
 *
 *  The `len` length of the `buf` determines the number of octets to encode
 *
 *  The octet numeric values are encoded in network byte order (i.e. big
 *  endian).
 *
 *  For ex: an integer with octet order 3 (MSB), 2, 1, 0 (LSB)
 *  buf[0] = octet 3;
 *  buf[1] = octet 2;
 *  buf[2] = octet 1;
 *  buf[3] = octet 0;
 *
 *  Returns the number of octets encoded
 */
static uint8_t encodeOctet(uint32_t value, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    for (i = 0; i < len; i++) {
        buf[i] = (value >> ((len - 1 - i) * 8)) & 0xFF;
    }

    return (i);
}

/*
 *  ======== decodeOctet ========
 *  Converts the octet array into the integer value
 *
 *  The `len` length of the `buf` determined the number of octets to decode
 *
 *  The octets in the array will be in network byte order (i.e. big endian).
 *
 *  Returns the number of octets decoded
 */
static uint8_t decodeOctet(uint8_t *buf, uint8_t len, void *value)
{
    switch (len) {
        case 1:
            *(uint8_t *)value = buf[0];
            break;
        case 2:
            *(uint16_t *)value = (((uint16_t)buf[0]) << 8)  | buf[1];
            break;
        case 3:
            *(uint32_t *)value = (((uint32_t)buf[0]) << 16)
                    | (((uint32_t)buf[1]) << 8) | buf[2];
            break;
        case 4:
            *(uint32_t *)value = (((uint32_t)buf[0]) << 24)
                    | (((uint32_t)buf[1]) << 16) | (((uint32_t)buf[2]) << 8)
                    | buf[3];
            break;
        default:
            len = 0;
    }

    return (len);
}

/*
 *  ======== freeHeaderBlock ========
 */
static void freeHeaderBlock(uint8_t **headerBlock)
{
    if (headerBlock && *headerBlock) {
        free(*headerBlock);
        *headerBlock = NULL;
    }
}

/*
 *  ======== addHeader ========
 *  Add a header field to the headers list
 *
 *  This function adds the header field (name and value) to the list. The
 *  sum of the name string length and the value string length is returned back.
 *  This sum is useful to calculate header list size which has a max limit
 *  for a HTTP/2 connection.
 *
 *  Returns the number of octets added on success or < 0 on failure
 */
static int addHeader(HTTP2Hdr_Field **headersList, uint32_t len,
        const char *name, const char *value, size_t valueLen)
{
    int ret = HTTP2Cli_EINVALIDREQUESTPARAMS;
    char *lname;
    char *lvalue;
    size_t nameLen;

    if (headersList && name && value) {
        nameLen = strlen(name);
        lname = HTTP2Utils_stringDuplicate(name, nameLen);
        if (lname) {
            lvalue = HTTP2Utils_stringDuplicate(value, valueLen);
            if (lvalue) {
                ret = HTTP2Hdr_add(headersList, len, lname, lvalue);
                if (ret > 0) {
                    ret = nameLen + valueLen;
                }
                else {
                    free(lvalue);
                    ret = HTTP2Cli_EINSUFFICIENTHEAP;
                }
            }
            else {
               free(lname);
            }
        }
        else {
            ret = HTTP2Cli_EINSUFFICIENTHEAP;
        }
    }

    return (ret);
}

/*
 *  ======== getHeadersList ========
 *  Creates a list of headers
 *
 *  This functions generaters a list of common headers based on the input
 *  URI (i.e. ":method", ":scheme", ":authority", ":path" and "user-agent")
 *  and appends the additional user supplied headers.
 *
 *  The newly created list is returned in 'headersList' and the total number of
 *  headers in 'headersList' is returned by the function.
 *
 *  Returns the number of headers in the list
 */
static int getHeadersList(HTTP2Cli_Handle cli, const char *method,
        const char *uri, HTTP2Hdr_Field *additionalHeaders, uint8_t headersLen,
        HTTP2Hdr_Field **headersList)
{
    int ret = 0;
    int i;
    size_t len;
    char *substr;
    uint16_t numHeaders = 0;
    uint32_t headersListSize = 0;

    ret = addHeader(headersList, numHeaders, HTTP2Std_PSEUDO_HEADER_METHOD,
            method, strlen(method));
    if (ret > 0) {
        numHeaders++;
        headersListSize += ret;

        substr = strstr(uri, URI_SCHEME_SEPARATOR);
        if (substr) {
            len = substr - uri;
            ret = addHeader(headersList, numHeaders,
                    HTTP2Std_PSEUDO_HEADER_SCHEME, uri, len);
            uri += len + sizeof(URI_SCHEME_SEPARATOR) - 1;
        }
        else {
            ret = addHeader(headersList, numHeaders,
                    HTTP2Std_PSEUDO_HEADER_SCHEME, URI_PREFIX,
                    sizeof(URI_PREFIX) - 1);
        }

        if (ret > 0) {
            numHeaders++;
            headersListSize += ret;

            substr = strchr(uri, '/');
            if (substr) {
                len = substr - uri;
            }
            else {
                len = strlen(uri);
            }

            ret = addHeader(headersList, numHeaders,
                    HTTP2Std_PSEUDO_HEADER_AUTHORITY, uri, len);
            if (ret > 0) {
                numHeaders++;
                headersListSize += ret;

                uri += len;
                if (*uri) {
                    ret = addHeader(headersList, numHeaders,
                            HTTP2Std_PSEUDO_HEADER_PATH, uri, strlen(uri));
                }
                else {
                    ret = addHeader(headersList, numHeaders,
                            HTTP2Std_PSEUDO_HEADER_PATH, "/", 1);
                }

                if (ret > 0) {
                    numHeaders++;
                    headersListSize += ret;

                    ret = addHeader(headersList, numHeaders,
                            HTTPStd_FIELD_NAME_USER_AGENT, USER_AGENT_VALUE,
                            sizeof(USER_AGENT_VALUE) - 1);

                    if (ret > 0) {
                        numHeaders++;
                        headersListSize += ret;

                        for (i = 0; i < headersLen; i++) {
                            ret = addHeader(headersList, numHeaders,
                                    additionalHeaders[i].name,
                                    additionalHeaders[i].value,
                                    strlen(additionalHeaders[i].value));
                            if (ret < 0) {
                                break;
                            }

                            numHeaders++;
                            headersListSize += ret;
                        }
                    }
                }
            }
        }
    }
    else {
        ret = HTTP2Cli_EINSUFFICIENTHEAP;
    }

    if (headersListSize > cli->serverSettings.headersListSize) {
        ret = HTTP2Cli_EHEADERSLISTTOOBIG;
    }

    if (ret < 0 && headersList) {
        HTTP2Hdr_free(headersList, numHeaders);
    }
    else {
        ret = numHeaders;
    }

    return (ret);
}

/*
 *  ======== recvFrame ========
 *  Reads a HTTP/2 frame from the server
 *
 *  Reads and decodes the frame fields from the socket and returns the fields
 *  in 'type', 'flags', 'streamId', 'payload' and 'plen'.
 *
 *  A 'timeoutMs' value in units of milliseconds will be set for socket recv
 *  operation.
 *
 *  Returns 0 on success or < 0 on failure
 */
static int recvFrame(HTTP2Cli_Handle cli, uint32_t timeoutMs, uint8_t *type,
       uint8_t *flags, uint32_t *streamId, uint8_t **payload, uint32_t *plen)
{
    int ret;
    uint8_t header[HTTP2Std_FRAME_HEADER_LEN];
    uint8_t hlen = 0;
    uint8_t *pload = NULL;
    struct timeval tv;

    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    ret = Ssock_getSocket(&(cli->ssock));

    ret = setsockopt(ret, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret < 0) {
        cli->sockError = getErrno(ret);
        ret = HTTP2Cli_ESOCKETFAIL;
    }
    else {
        ret = Ssock_recvall(&(cli->ssock), header, HTTP2Std_FRAME_HEADER_LEN,
                0);
        if (ret <= 0) {
            if (ret == Ssock_TIMEOUT) {
                ret = HTTP2Cli_ERECVTIMEOUT;
            }
            else {
                cli->sockError = getErrno(ret);
                ret = HTTP2Cli_ERECVFAIL;
            }
        }
        else {
            /* Frame field: Length */
            hlen += decodeOctet(header, HTTP2Std_FRAME_HEADER_FIELD_LENGTH_LEN,
                    plen);

            /* Frame field: Type */
            hlen += decodeOctet(header + hlen,
                    HTTP2Std_FRAME_HEADER_FIELD_TYPE_LEN, type);

            /* Frame field: Flags */
            hlen += decodeOctet(header + hlen,
                    HTTP2Std_FRAME_HEADER_FIELD_FLAGS_LEN, flags);

            /* Frame field: Stream Identifier */
            hlen += decodeOctet(header + hlen,
                    HTTP2Std_FRAME_HEADER_FIELD_STREAM_ID_LEN, streamId);

            if (*plen) {
                if (*plen > HTTP2Std_SETTINGS_FRAME_SIZE_DEFAULT) {
                    sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                            HTTP2Std_ECODE_PROTOCOL_ERROR);
                    ret = HTTP2Cli_EPROTOCOL;
                }
                else {
                    pload = (uint8_t *) malloc(*plen);
                    if (pload) {
                        ret = Ssock_recvall(&(cli->ssock), pload, *plen, 0);
                        if (ret <= 0) {
                            if (ret == Ssock_TIMEOUT) {
                                ret = HTTP2Cli_ERECVTIMEOUT;
                            }
                            else {
                                cli->sockError = getErrno(ret);
                                ret = HTTP2Cli_ERECVFAIL;
                            }
                            free(pload);
                            pload = NULL;
                        }
                    }
                    else {
                        ret = HTTP2Cli_EINSUFFICIENTHEAP;
                    }
                }
            }
        }
    }
    *payload = pload;

    return ((ret < 0) ? ret : 0);
}

/*
 *  ======== sendFrame ========
 *  Send a HTTP/2 frame to the server
 *
 *  Encode the HTTP/2 frame in correct octet format and send it to
 *  the server.
 *
 *  Returns 0 on success or < 0 on failure.
 */
static int sendFrame(HTTP2Cli_Handle cli, uint8_t type, uint8_t flags,
        uint32_t streamId, uint8_t *payload, uint32_t plen)
{
    int ret;
    uint8_t header[HTTP2Std_FRAME_HEADER_LEN];
    uint8_t hlen = 0;

    if (streamId > HTTP2Std_STREAM_ID_MAX) {
        return (HTTP2Cli_ESTREAMID);
    }

    /* Frame field: Length */
    hlen += encodeOctet(plen, header + hlen,
            HTTP2Std_FRAME_HEADER_FIELD_LENGTH_LEN);

    /* Frame field: Type */
    hlen += encodeOctet(type, header + hlen,
            HTTP2Std_FRAME_HEADER_FIELD_TYPE_LEN);

    /* Frame field: Flags */
    hlen += encodeOctet(flags, header + hlen,
            HTTP2Std_FRAME_HEADER_FIELD_FLAGS_LEN);

    /* Frame field: Stream Identifier */
    hlen += encodeOctet(streamId, header + hlen,
            HTTP2Std_FRAME_HEADER_FIELD_STREAM_ID_LEN);

    ret = Ssock_send(&(cli->ssock), header, hlen, 0);
    if (ret <= 0) {
        cli->sockError = getErrno(ret);
        ret = HTTP2Cli_ESENDFAIL;
    }
    else if (payload != NULL) {
        ret = Ssock_send(&(cli->ssock), payload, plen, 0);
        if (ret <= 0) {
            cli->sockError = getErrno(ret);
            ret = HTTP2Cli_ESENDFAIL;
        }
    }

    return ((ret < 0) ? ret : 0);
}

/*
 *  ======== sendFrameAll ========
 *  Breaks large payload into multiple frames and sends them all
 *
 *  Large payloads > max frame payload allowed are broken into smaller
 *  chunks of size <= max frame payload and sent over multiple frames.
 *  This is useful for sending large header and data payloads.
 *
 *  Returns 0 on success or < 0 on failure.
 */
static int sendFrameAll(HTTP2Cli_Handle cli, uint8_t type,
        uint32_t streamId, uint8_t *payload, uint32_t plen, bool endStream)
{
    int ret;
    uint32_t len;
    uint32_t frameSize;
    uint8_t flags = 0;

    frameSize = cli->serverSettings.frameSize;

    do {
        if (plen > frameSize) {
            len = frameSize;
        }
        else {
            len = plen;
            if ((type == HTTP2Std_FRAME_TYPE_HEADERS)
                    || (type == HTTP2Std_FRAME_TYPE_CONTINUATION)) {
                flags = HTTP2Std_FLAGS_END_HEADERS;
            }

            if (endStream) {
                flags |= HTTP2Std_FLAGS_END_STREAM;
            }
        }

        ret = sendFrame(cli, type, flags, streamId, payload, len);
        if (ret < 0) {
            break;
        }

        payload += len;
        plen -= len;

        if (type == HTTP2Std_FRAME_TYPE_HEADERS) {
            type = HTTP2Std_FRAME_TYPE_CONTINUATION;
        }

    } while (plen);

    return (ret);
}

/*
 *  ======== sendConnectionError ========
 */
static void sendConnectionError(HTTP2Cli_Handle cli, uint32_t streamId,
        uint32_t errorType)
{
    uint8_t payload[HTTP2Std_GOAWAY_PAYLOAD_LEN];
    uint32_t plen = 0;

    /* GOAWAY payload: Last server initiated stream identifier */
    plen += encodeOctet(GOAWAY_SERVER_LAST_STREAM_ID, payload + plen,
            HTTP2Std_GOAWAY_PAYLOAD_FIELD_STREAM_ID_LEN);

    /* GOAWAY payload: Error code */
    plen += encodeOctet(errorType, payload + plen,
            HTTP2Std_GOAWAY_PAYLOAD_FIELD_ERROR_CODE_LEN);

    sendFrame(cli, HTTP2Std_FRAME_TYPE_GOAWAY, 0, streamId, payload, plen);

    HTTP2Cli_disconnect(cli);
}

/*
 *  ======== updateSettings ========
 *  Update the HTTP/2 settings received from the server
 */
static void updateSettings(HTTP2Cli_Handle cli, uint8_t *payload,
        uint32_t plen)
{
    uint16_t id;
    uint32_t value;

    while (plen) {
        /* Settings: Identifier */
        plen -= decodeOctet(payload, HTTP2Std_SETTINGS_FIELD_ID_LEN,
                (uint32_t *)&id);

        /* Settings: Value */
        plen -= decodeOctet(payload, HTTP2Std_SETTINGS_FIELD_VALUE_LEN, &value);

        switch (id) {
            case HTTP2Std_SETTINGS_HEADER_TABLE_SIZE:
                /* Not used as client doesn't send dynamic headers */
                cli->serverSettings.headerTableSize = value;
                break;

            case HTTP2Std_SETTINGS_ENABLE_PUSH:
                /* Push promise is not supported currently */
                break;

            case HTTP2Std_SETTINGS_MAX_CONCURRENT_STREAMS:
                cli->serverSettings.concurrentStreams = value;
                break;

            case HTTP2Std_SETTINGS_INITIAL_WINDOW_SIZE:
                cli->serverSettings.windowSize = value;
                break;

            case HTTP2Std_SETTINGS_MAX_FRAME_SIZE:
                cli->serverSettings.frameSize = value;
                break;

            case HTTP2Std_SETTINGS_MAX_HEADER_LIST_SIZE:
                cli->serverSettings.headersListSize = value;
                break;

            default:
                break;
        }
    }
}

/*
 *  ======== syncSettings ========
 *  Performs an initial HTTP/2 settings sync between the client and the server
 *
 *  Returns < 0 on failure
 */
static int syncSettings(HTTP2Cli_Handle cli)
{
    int ret;
    uint8_t payload[HTTP2Std_SETTINGS_PAYLOAD_LEN];
    uint32_t plen = 0;
    uint8_t type;
    uint8_t flags;
    uint32_t srvStreamId;
    uint8_t *srvPayload = NULL;
    uint32_t srvPlen;

    /* Header table size */
    plen += encodeOctet(HTTP2Std_SETTINGS_HEADER_TABLE_SIZE, payload + plen,
            HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(HTTP2Std_SETTINGS_HEADER_TABLE_SIZE_DEFAULT,
            payload + plen, HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    /* Server push: disable */
    plen += encodeOctet(HTTP2Std_SETTINGS_ENABLE_PUSH, payload + plen,
            HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(SETTINGS_PUSH_DISABLE, payload + plen,
            HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    /* Concurrent streams */
    plen += encodeOctet(HTTP2Std_SETTINGS_MAX_CONCURRENT_STREAMS,
            payload + plen, HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(HTTP2Std_SETTINGS_CONCURRENT_STREAMS_DEFAULT,
            payload + plen, HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    /* Flow control window size */
    plen += encodeOctet(HTTP2Std_SETTINGS_INITIAL_WINDOW_SIZE, payload + plen,
            HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(HTTP2Std_SETTINGS_WINDOW_SIZE_DEFAULT, payload + plen,
            HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    /* Frame size */
    plen += encodeOctet(HTTP2Std_SETTINGS_MAX_FRAME_SIZE, payload + plen,
            HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(HTTP2Std_SETTINGS_FRAME_SIZE_DEFAULT, payload + plen,
            HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    /* Header list size */
    plen += encodeOctet(HTTP2Std_SETTINGS_MAX_HEADER_LIST_SIZE, payload + plen,
            HTTP2Std_SETTINGS_FIELD_ID_LEN);
    plen += encodeOctet(HTTP2Std_SETTINGS_HEADER_LIST_SIZE_DEFAULT,
            payload + plen, HTTP2Std_SETTINGS_FIELD_VALUE_LEN);

    ret = sendFrame(cli, HTTP2Std_FRAME_TYPE_SETTINGS, 0,
            HTTP2Std_STREAM_ID_DEFAULT, payload, plen);
    if (ret == 0) {
        ret = recvFrame(cli, FRAME_RECV_TIMEOUT_MILLISECS, &type, &flags,
                &srvStreamId, &srvPayload, &srvPlen);
        if (ret < 0) {
            if (ret == HTTP2Cli_ERECVTIMEOUT) {
                sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                        HTTP2Std_ECODE_SETTINGS_TIMEOUT);
            }
        }
        else if ((type != HTTP2Std_FRAME_TYPE_SETTINGS) || srvStreamId) {
            sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                    HTTP2Std_ECODE_PROTOCOL_ERROR);
            ret = HTTP2Cli_EPROTOCOL;
        }
        else if (srvPlen % HTTP2Std_SETTINGS_PARAMS_LEN) {
            sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                    HTTP2Std_ECODE_FRAME_SIZE_ERROR);
            ret = HTTP2Cli_EFRAMESIZE;
        }
        else {
            updateSettings(cli, srvPayload, srvPlen);
            free(srvPayload);
            srvPayload = NULL;

            ret = sendAck(cli, HTTP2Std_FRAME_TYPE_SETTINGS, NULL, 0);
            if (ret == 0) {
                /* Wait for Settings ACK */
                do {
                    ret = HTTP2Cli_processResponse(cli,
                            FRAME_RECV_TIMEOUT_MILLISECS, NULL);
                    if (ret < 0) {
                        if (ret == HTTP2Cli_ERECVTIMEOUT) {
                            sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                                    HTTP2Std_ECODE_SETTINGS_TIMEOUT);
                        }
                        break;
                    }
                } while (ret != HTTP2Std_FRAME_TYPE_SETTINGS);

                if (ret == HTTP2Std_FRAME_TYPE_SETTINGS) {
                    ret = 0;
                }
            }
        }

        if (srvPayload != NULL) {
            free(srvPayload);
            srvPayload = NULL;
        }
    }

    return (ret);
}

/*
 *  ======== getNextStreamId ========
 */
static uint32_t getNextStreamId(HTTP2Cli_Handle cli)
{
   uint32_t streamId = cli->nextStreamId;

   cli->nextStreamId += 2;

   return ((streamId == HTTP2Std_STREAM_ID_MAX) ? 0 : streamId);
}

/*
 *  ======== sendAck ========
 */
static int sendAck(HTTP2Cli_Handle cli, uint8_t type, uint8_t *payload,
        uint32_t len)
{
    return (sendFrame(cli, type, HTTP2Std_FLAGS_ACK, HTTP2Std_STREAM_ID_DEFAULT,
            payload, len));
}

/*
 *  ======== getContinuationHeaders ========
 *  Reads all continuation header frames from the server
 *
 *  Reads all continuation header frames and returns the header payload
 *  in 'headerBlock' and the length of payload in 'len'. The 'endStream'
 *  is set if no further frames are available in this stream.
 *
 *  Returns 0 on success or < 0 on error
 */
static int getContinuationHeaders(HTTP2Cli_Handle cli, uint8_t **headerBlock,
        uint32_t *len, bool *endStream)
{
    int ret = 0;
    uint8_t type;
    uint8_t flags;
    uint32_t streamId;
    uint32_t plen;
    uint8_t *payload = NULL;
    uint8_t *hb = NULL;

    flags = 0;

    while (!(flags & HTTP2Std_FLAGS_END_HEADERS) && (ret == 0)) {
        ret = recvFrame(cli, FRAME_RECV_TIMEOUT_MILLISECS, &type, &flags,
                &streamId, &payload, &plen);

        if (ret == 0) {
            if (type != HTTP2Std_FRAME_TYPE_CONTINUATION || !streamId) {
                sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                ret = HTTP2Cli_EPROTOCOL;
            }
            else {
                if (payload && plen) {
                    hb = (uint8_t *) realloc(*headerBlock,
                            *len + plen);
                    if (hb) {
                        memcpy(hb + *len, payload, plen);
                        *len += plen;
                        *headerBlock = hb;
                    }
                    else {
                        freeHeaderBlock(headerBlock);
                        ret =  HTTP2Cli_EINSUFFICIENTHEAP;
                    }
                }
            }
        }

        free(payload);
        payload = NULL;
    }

    *endStream = flags & HTTP2Std_FLAGS_END_STREAM;

    return (ret);
}

/*
 *  ======== processResponseHeaders ========
 *  Process the HTTP/2 headers received from the server
 *
 *  All headers from the server are read and the header decompression
 *  is applied. If a callback is registered, the headers are returned
 *  to the user application through the callback.
 *
 *  Returns 0 on success < 0 on failure.
 */
static int processResponseHeaders(HTTP2Cli_Handle cli, uint8_t flags,
        uint32_t streamId, uint8_t *payload, uint32_t len)
{
    int i;
    int ret = 0;
    bool endStream;
    HTTP2Hdr_Field *headersList;
    uint32_t status = 0;
    uint8_t *headerBlock = NULL;
    uint32_t headerBlockLen = 0;
    uint8_t skipOctets = 0;
    uint8_t padding = 0;

    if (!streamId) {
        sendConnectionError(cli, HTTP2Std_STREAM_ID_DEFAULT,
                HTTP2Std_ECODE_PROTOCOL_ERROR);
        ret = HTTP2Cli_EPROTOCOL;
    }
    else {
        endStream = flags & HTTP2Std_FLAGS_END_STREAM;

        if (flags & HTTP2Std_FLAGS_PADDED) {

            /* Headers: Padded Length */
            decodeOctet(payload, HTTP2Std_HEADER_FIELD_PAD_LEN, &padding);

            /* Error if pad length > size remaining for header block */
            if (padding >= (len - padding)) {
                sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                ret = HTTP2Cli_EPROTOCOL;
            }

            skipOctets += HTTP2Std_HEADER_FIELD_PAD_LEN;
        }

        if (ret == 0) {
            if (flags & HTTP2Std_FLAGS_PRIORITY) {
                /* ignore stream dependency and weights */
                skipOctets += HTTP2Std_HEADER_STREAM_DEPENDENCY_LEN
                        + HTTP2Std_HEADER_WEIGHT_LEN;
            }

            headerBlockLen = len - skipOctets - padding;
            headerBlock = (uint8_t *) malloc(headerBlockLen);
            if (headerBlock) {
                memcpy(headerBlock, payload + skipOctets, headerBlockLen);
                if (!(flags & HTTP2Std_FLAGS_END_HEADERS)) {
                    ret = getContinuationHeaders(cli, &headerBlock,
                            &headerBlockLen, &endStream);
                }

                if ((ret == 0) && cli->headersFxn) {
                    ret = HPACK_decode(&(cli->hpack), headerBlock,
                            headerBlockLen, &headersList);

                    if (ret < 0) {
                        if (ret == HPACK_EINSUFFICIENTHEAP) {
                            ret = HTTP2Cli_EINSUFFICIENTHEAP;
                        }
                        else {
                            sendConnectionError(cli, 0,
                                    HTTP2Std_ECODE_PROTOCOL_ERROR);
                            ret = HTTP2Cli_EPROTOCOL;
                        }
                    }
                }

                free(headerBlock);
            }
            else {
                ret = HTTP2Cli_EINSUFFICIENTHEAP;
            }

        }

        if (ret > 0) {
            if (cli->headersFxn) {
                for (i = 0; i < ret; i++) {
                    if (strncmp(HTTP2Std_PSEUDO_HEADER_STATUS,
                            headersList[i].name,
                            sizeof(HTTP2Std_PSEUDO_HEADER_STATUS) - 1) == 0) {
                        status = strtoul(headersList[i].value, NULL, 10);

                        break;
                    }
                }

                cli->headersFxn(cli, cli->arg, streamId, status, headersList,
                        ret, endStream);
            }

            HTTP2Hdr_free(&headersList, ret);
            ret = 0;
        }
    }

    return (ret);
}

/*
 *  ======== processResponseData ========
 *  Process the data frame received from the server
 *
 *  Processes the data frame and checks from RFC data frame conformance.
 *  The processed payload will be returned to user application, if a
 *  callback function was registered.
 *
 *  Returns 0 on success < 0 on failure
 */
static int processResponseData(HTTP2Cli_Handle cli, uint8_t flags,
        uint32_t streamId, uint8_t *payload, uint32_t len)
{
    int ret = 0;
    uint32_t padlen;
    bool endStream;

    if (!streamId) {
        sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
        ret = HTTP2Cli_EPROTOCOL;
    }
    else {
        endStream = flags & HTTP2Std_FLAGS_END_STREAM;

        if (flags & HTTP2Std_FLAGS_PADDED) {
            /* Data: Pad Length */
            payload += decodeOctet(payload, HTTP2Std_DATA_FIELD_PAD_LEN,
                    &padlen);
            if (padlen >= len) {
                sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                ret = HTTP2Cli_EPROTOCOL;
            }
            else {
                len -= padlen;
            }
        }

        if ((ret == 0) && len && cli->dataFxn) {
            cli->dataFxn(cli, cli->arg, streamId, payload, len, endStream);
        }
    }

    return (ret);
}

/*
 *  ======== HTTP2Cli_construct ========
 */
void HTTP2Cli_construct(HTTP2Cli_Struct *cli, const HTTP2Cli_Params *params)
{
    xassert(cli != NULL);
    xassert((params != NULL) && (params->tls != NULL));

    memset(cli, 0, sizeof(HTTP2Cli_Struct));
    cli->nextStreamId = HTTP2Std_STREAM_ID_CLIENT_DEFAULT;
    cli->serverSettings = defaultServerSettings;
    cli->headersFxn = params->headersFxn;
    cli->dataFxn = params->dataFxn;
    cli->tls = params->tls;
    cli->arg = params->arg;
}

/*
 *  ======== HTTP2Cli_create ========
 */
HTTP2Cli_Handle HTTP2Cli_create(const HTTP2Cli_Params *params)
{
    HTTP2Cli_Handle cli;

    cli = (HTTP2Cli_Handle) malloc(sizeof(HTTP2Cli_Struct));
    if (cli != NULL) {
        HTTP2Cli_construct(cli, params);
    }

    return (cli);
}

/*
 *  ======== HTTP2Cli_delete ========
 */
void HTTP2Cli_delete(HTTP2Cli_Handle *cli)
{
    xassert(*cli != NULL);

    HTTP2Cli_destruct(*cli);
    free(*cli);
    *cli = NULL;
}

/*
 *  ======== HTTP2Cli_destruct ========
 */
void HTTP2Cli_destruct(HTTP2Cli_Struct *cli)
{
    xassert(cli != NULL);

    memset(cli, 0, sizeof(HTTP2Cli_Struct));
}

/*
 *  ======== HTTP2Cli_initSockAddr ========
 */
int HTTP2Cli_initSockAddr(const char *uri, bool ipv6, struct sockaddr *addr)
{
    int ret = 0;
    char *hostname;
    uint32_t hlen;
    uint32_t port = HTTPStd_SECURE_PORT;

    xassert(addr != NULL);
    xassert(uri != NULL);

    hostname = strstr(uri, URI_SCHEME_SEPARATOR);
    if (hostname) {
        uri = hostname + sizeof(URI_SCHEME_SEPARATOR) - 1;
    }

    hostname = strchr(uri, URI_IPV6_ADDRESS_START);
    if (hostname) {
        uri += sizeof(char);
        hostname = strchr(uri, URI_IPV6_ADDRESS_STOP);
        if (!hostname){
            return (HTTP2Cli_EHOSTNAMERESOLVE);
        }
        hlen = hostname - uri;

        /* get the port */
        hostname = strchr(hostname, URI_PORT_SEPARATOR);
        if (hostname) {
            port = strtoul((hostname + 1), NULL, 10);
        }
    }
    else {
        hostname = strchr(uri, URI_PORT_SEPARATOR);
        if (hostname) {
            hlen = hostname - uri;
            port = strtoul((hostname + 1), NULL, 10);
        }
        else {
            hostname = strchr(uri, URI_PATH_SEPARATOR);
            if (hostname) {
                hlen = hostname - uri;
            }
            else {
                hlen = strlen(uri);
            }
        }
    }

    hostname = HTTP2Utils_stringDuplicate(uri, hlen);
    if (!hostname) {
        return (HTTP2Cli_EINSUFFICIENTHEAP);
    }

#ifdef NET_SL
    unsigned long ip[4];
    uint8_t family = AF_INET;

    if (ipv6) {
        family = AF_INET6;
    }

    ret = sl_NetAppDnsGetHostByName((signed char *)hostname, hlen, ip, family);
    if (ret < 0) {
        ret = HTTP2Cli_EHOSTNAMERESOLVE;
        goto error;
    }

    memset(addr, 0, sizeof(struct sockaddr));
    if (ipv6) {
        ((struct sockaddr_in6 *)addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
        ((struct sockaddr_in6 *)addr)->sin6_addr = *(struct in6_addr *) &ip;
    }
    else {
        ((struct sockaddr_in *)addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)addr)->sin_port = htons(port);
        ((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ip[0]);
    }


#else
    struct addrinfo hints;
    struct addrinfo *rAddrs = NULL;
    struct addrinfo *tAddrs;

    memset(&hints, 0, sizeof(struct addrinfo));
    memset(addr, 0, sizeof(struct sockaddr));

    if (ipv6) {
        hints.ai_family = AF_INET6;
    }
    else {
        hints.ai_family = AF_INET;
    }

    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(hostname, NULL, &hints, &rAddrs);
    if (ret != 0) {
        ret = HTTP2Cli_EHOSTNAMERESOLVE;
        goto error;
    }

    for (tAddrs = rAddrs; tAddrs != NULL; tAddrs = tAddrs->ai_next) {
        if (tAddrs->ai_family == hints.ai_family) {
            memcpy(addr, tAddrs->ai_addr, tAddrs->ai_addrlen);
            break;
        }
    }

    freeaddrinfo(rAddrs);

    if (ipv6) {
        ((struct sockaddr_in6 *)addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
    }
    else {
        ((struct sockaddr_in *)addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)addr)->sin_port = htons(port);
    }

#endif

    ret = 0;

error:
    free(hostname);

    return (ret);
}

/*
 *  ======== HTTP2Cli_Params_init ========
 */
void HTTP2Cli_Params_init(HTTP2Cli_Params *params)
{
    xassert(params != NULL);

    memset(params, 0, sizeof(HTTP2Cli_Params));
}

/*
 *  ======== HTTP2Cli_connect ========
 */
int HTTP2Cli_connect(HTTP2Cli_Handle cli, const struct sockaddr *addr,
        int flags)
{
    int skt;
    int ret;
    socklen_t addrlen;
    char alpnList[] = ALPN_HTTP2_TLS_ID;

    xassert(cli != NULL);
    xassert(addr != NULL);

    if (addr->sa_family == AF_INET) {
        addrlen = sizeof(struct sockaddr_in);
    }
    else {
        addrlen = sizeof(struct sockaddr_in6);
    }

    HPACK_construct(&(cli->hpack));

    skt = socket(addr->sa_family, SOCK_STREAM, TLS_SOCKET_OPT);
    if (skt != -1) {
        Ssock_construct(&(cli->ssock), skt);

        ret = Ssock_startTLSWithALPN(&(cli->ssock), cli->tls, alpnList);
        if (ret < 0) {
            cli->sockError = getErrno(ret);
            HTTP2Cli_disconnect(cli);

            ret = HTTP2Cli_ETLSFAIL;
        }
        else {
            ret = connect(skt, addr, addrlen);
#ifdef NET_SL
            /*
             *  SL returns unknown root CA error code if the CA certificate
             *  is not found in its certificate store. This is a warning
             *  code and not an error. So this error code is being ignored
             *  till a better alternative is found.
             */
            if (ret < 0 && getErrno(ret) != SL_ERROR_BSD_ESECUNKNOWNROOTCA) {
#else
            if (ret < 0) {
#endif
                cli->sockError = getErrno(ret);
                HTTP2Cli_disconnect(cli);

                ret = HTTP2Cli_ECONNECTFAIL;
            }
            else {
                /* Send the HTTP/2 Connection Preface */
                ret = Ssock_send(&(cli->ssock),
                        HTTP2Std_CLIENT_CONNECTION_PREFACE,
                        sizeof(HTTP2Std_CLIENT_CONNECTION_PREFACE) - 1, 0);
                if (ret <= 0) {
                    cli->sockError = getErrno(ret);
                    HTTP2Cli_disconnect(cli);

                    ret = HTTP2Cli_ESENDFAIL;
                }
                else {
                    ret = syncSettings(cli);
                    if (ret < 0) {
                        HTTP2Cli_disconnect(cli);
                    }
                }
            }
        }
    }
    else {
        cli->sockError = getErrno(skt);
        ret = HTTP2Cli_ESOCKETFAIL;
    }

    return (ret);
}

/*
 *  ======== HTTP2Cli_disconnect ========
 */
void HTTP2Cli_disconnect(HTTP2Cli_Handle cli)
{
    int skt;

    xassert(cli != NULL);

    HPACK_destruct(&(cli->hpack));

    skt = Ssock_getSocket(&(cli->ssock));
    Ssock_destruct(&(cli->ssock));
    close(skt);

    cli->nextStreamId = HTTP2Std_STREAM_ID_CLIENT_DEFAULT;
    cli->serverSettings = defaultServerSettings;
}

/*
 *  ======== HTTP2Cli_sendRequest ========
 */
int HTTP2Cli_sendRequest(HTTP2Cli_Handle cli, const char *method,
        const char *uri, HTTP2Hdr_Field *additionalHeaders, uint8_t headersLen,
        uint8_t *data, uint32_t dataLen, uint32_t *streamId)
{
    int ret;
    bool endStream = false;

    if (dataLen == 0) {
        endStream = true;
    }

    ret = HTTP2Cli_sendRequestHeaders(cli, method, uri, additionalHeaders,
            headersLen, endStream, streamId);
    if (ret == 0 && dataLen) {
        ret = HTTP2Cli_sendRequestData(cli, streamId, data, dataLen, true);
    }

    return (ret);
}

/*
 *  ======== HTTP2Cli_sendRequestHeaders ========
 */
int HTTP2Cli_sendRequestHeaders(HTTP2Cli_Handle cli, const char *method,
        const char *uri, HTTP2Hdr_Field *additionalHeaders, uint8_t headersLen,
        bool endStream, uint32_t *streamId)
{
    int ret;
    uint16_t numHeaders;
    uint8_t *headerBlock = NULL;
    HTTP2Hdr_Field *headersList = NULL;

    xassert(cli != NULL);
    xassert(method != NULL);
    xassert(uri != NULL);
    xassert(streamId != NULL);

    ret = getHeadersList(cli, method, uri, additionalHeaders, headersLen,
            &headersList);
    if (ret > 0) {
        numHeaders = ret;
        ret = HPACK_encode(&(cli->hpack), headersList, numHeaders,
                &headerBlock);
        HTTP2Hdr_free(&headersList, numHeaders);

        if (ret < 0) {
            ret = HTTP2Cli_EINSUFFICIENTHEAP;
        }
        else {
            *streamId = getNextStreamId(cli);
            ret = sendFrameAll(cli, HTTP2Std_FRAME_TYPE_HEADERS, *streamId,
                    headerBlock, ret, endStream);
            freeHeaderBlock(&headerBlock);
        }
    }

    return (ret);
}

/*
 *  ======== HTTP2Cli_sendRequestData ========
 */
int HTTP2Cli_sendRequestData(HTTP2Cli_Handle cli, uint32_t *streamId,
        uint8_t *data, uint32_t dataLen, bool endStream)
{
    int ret;

    xassert(cli != NULL);
    xassert(streamId != NULL);
    xassert(data != NULL);

    ret = sendFrameAll(cli, HTTP2Std_FRAME_TYPE_DATA, *streamId, data, dataLen,
            endStream);

    return (ret);
}

/*
 *  ======== HTTP2Cli_processResponse ========
 */
int HTTP2Cli_processResponse(HTTP2Cli_Handle cli, uint32_t timeoutMs,
        HTTP2Cli_Error *error)
{
    int ret;
    uint8_t type;
    uint8_t flags;
    uint32_t streamId;
    uint8_t  *payload = NULL;
    uint32_t len;

    xassert(cli != NULL);

    if (timeoutMs == 0) {
        return (0);
    }

    ret = recvFrame(cli, timeoutMs, &type, &flags, &streamId, &payload, &len);
    if (ret == 0) {
        switch (type) {
            case HTTP2Std_FRAME_TYPE_DATA:
                if (payload && len) {
                    ret = processResponseData(cli, flags, streamId, payload,
                            len);
                    if (ret == 0) {
                        ret = HTTP2Std_FRAME_TYPE_DATA;
                    }
                }
                break;

            case HTTP2Std_FRAME_TYPE_HEADERS:
                if (payload && len) {
                    ret = processResponseHeaders(cli, flags, streamId, payload,
                            len);
                    if (ret == 0) {
                        ret = HTTP2Std_FRAME_TYPE_HEADERS;
                    }
                }
                break;

            case HTTP2Std_FRAME_TYPE_PRIORITY:
                /* not expecting server streams, so ignoring priority */
                ret = HTTP2Std_FRAME_TYPE_PRIORITY;
                break;

            case HTTP2Std_FRAME_TYPE_RST_STREAM:
                if (streamId == HTTP2Std_STREAM_ID_DEFAULT) {
                    sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                    ret = HTTP2Cli_EPROTOCOL;
                }
                else if (len != HTTP2Std_RST_PAYLOAD_LEN) {
                    sendConnectionError(cli, 0,
                            HTTP2Std_ECODE_FRAME_SIZE_ERROR);
                    ret = HTTP2Cli_EFRAMESIZE;
                }
                else {
                    ret = HTTP2Cli_ERSTSTREAMRECV;
                    if (error && payload && len) {
                        error->streamId = streamId;
                        decodeOctet(payload, len, &(error->ecode));
                    }
                }
                break;

            case HTTP2Std_FRAME_TYPE_SETTINGS:
                if (flags & HTTP2Std_FLAGS_ACK) {
                    ret = HTTP2Std_FRAME_TYPE_SETTINGS;
                }
                else {
                    if ((len % HTTP2Std_SETTINGS_PARAMS_LEN)
                            || (payload == NULL)) {
                        sendConnectionError(cli, 0,
                                HTTP2Std_ECODE_FRAME_SIZE_ERROR);
                        ret = HTTP2Cli_EFRAMESIZE;
                    }
                    else {
                        updateSettings(cli, payload, len);
                        ret = sendAck(cli, HTTP2Std_FRAME_TYPE_SETTINGS, NULL,
                                0);
                        if (ret == 0) {
                            ret = HTTP2Std_FRAME_TYPE_SETTINGS;
                        }
                    }
                }
                break;

            case HTTP2Std_FRAME_TYPE_PUSH_PROMISE:
                sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                ret = HTTP2Cli_EPROTOCOL;
                break;

            case HTTP2Std_FRAME_TYPE_PING:
                if (streamId != HTTP2Std_STREAM_ID_DEFAULT) {
                    sendConnectionError(cli, 0,
                            HTTP2Std_ECODE_PROTOCOL_ERROR);
                    ret = HTTP2Cli_EPROTOCOL;
                }
                else if ((len != HTTP2Std_PING_PAYLOAD_LEN)
                        || (payload == NULL)) {
                    sendConnectionError(cli, 0,
                            HTTP2Std_ECODE_FRAME_SIZE_ERROR);
                    ret = HTTP2Cli_EFRAMESIZE;
                }
                else {
                    if (flags & HTTP2Std_FLAGS_ACK) {
                        ret = memcmp(pingPayload, payload,
                                 HTTP2Std_PING_PAYLOAD_LEN);
                        if (ret == 0) {
                            ret = HTTP2Std_FRAME_TYPE_PING;
                        }
                        else {
                            ret = HTTP2Cli_EPINGPAYLOAD;
                        }
                    }
                     else {
                        ret = sendAck(cli, HTTP2Std_FRAME_TYPE_PING, payload,
                                len);
                        if (ret == 0) {
                            ret = HTTP2Std_FRAME_TYPE_PING;
                        }
                     }
                 }
                 break;

            case HTTP2Std_FRAME_TYPE_GOAWAY:
                ret = HTTP2Cli_EGOAWAYRECV;
                if (payload && len && error) {
                    decodeOctet(payload,
                            HTTP2Std_GOAWAY_PAYLOAD_FIELD_STREAM_ID_LEN,
                            &(error->streamId));
                    decodeOctet(payload,
                            HTTP2Std_GOAWAY_PAYLOAD_FIELD_ERROR_CODE_LEN,
                            &(error->ecode));
                }
                break;

            case HTTP2Std_FRAME_TYPE_WINDOW_UPDATE:
                /* not implemented */
                ret = HTTP2Std_FRAME_TYPE_WINDOW_UPDATE;
                break;

            case HTTP2Std_FRAME_TYPE_CONTINUATION:
                /*
                 *  Continuation frames that occur in sequence will be read
                 *  during the header processing. Other continuation frames
                 *  are protocol violations.
                 */
                sendConnectionError(cli, 0, HTTP2Std_ECODE_PROTOCOL_ERROR);
                ret = HTTP2Cli_EPROTOCOL;
                break;
        }
    }

    if (payload) {
        free(payload);
        payload = NULL;
    }

    return (ret);
}

/*
 *  ======== HTTP2Cli_getSocket ========
 */
int HTTP2Cli_getSocket(HTTP2Cli_Handle cli)
{
    return (Ssock_getSocket(&(cli->ssock)));
}

/*
 *  ======== HTTP2Cli_getSocketError ========
 */
int HTTP2Cli_getSocketError(HTTP2Cli_Handle cli)
{
    return (cli->sockError);
}

/*
 *  ======== HTTP2Cli_sendPing ========
 */
int HTTP2Cli_sendPing(HTTP2Cli_Handle cli)
{
    return (sendFrame(cli, HTTP2Std_FRAME_TYPE_PING, 0,
            HTTP2Std_STREAM_ID_DEFAULT, pingPayload,
            HTTP2Std_PING_PAYLOAD_LEN));
}
