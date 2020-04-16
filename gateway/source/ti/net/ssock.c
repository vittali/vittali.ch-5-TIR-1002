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

#ifdef __linux__
#include <errno.h>
#endif

#ifdef NET_WOLFSSL
#include <ti/net/ssock_wolfssl.h>
#elif defined(NET_SL)
#include <ti/net/ssock_sl.h>
#endif

#include <ti/net/common.h>
#include "ssock.h"

/* should come from some SL header */
#define MAXSENDLEN 1460

/*
 *  ======== Ssock_construct ========
 */
void Ssock_construct(Ssock_Struct * ssockP, int s)
{
    if (ssockP) {
        memset(ssockP, 0, sizeof(Ssock_Struct));

        ssockP->s = s;
    }
}

/*
 *  ======== Ssock_create ========
 */
Ssock_Handle Ssock_create(int s)
{
    Ssock_Handle ssock;

    if ((ssock = xmalloc(sizeof(Ssock_Struct)))) {
        Ssock_construct(ssock, s);
    }

    return (ssock);
}

/*
 *  ======== Ssock_delete ========
 */
void Ssock_delete(Ssock_Handle * ssock)
{
    if (ssock && *ssock) {
        Ssock_Handle ss = *ssock;
        Ssock_destruct(ss);
        xfree(ss);
        *ssock = NULL;
    }
}

/*
 *  ======== Ssock_destruct ========
 */
void Ssock_destruct(Ssock_Struct * ssockP)
{
    if (ssockP) {
        if (ssockP->ctx && ssockP->sec.del) {
            ssockP->sec.del(ssockP->ctx);
        }

        memset(ssockP, 0, sizeof(Ssock_Struct));
    }
}

/*
 *  ======== Ssock_getSocket ========
 */
int Ssock_getSocket(Ssock_Handle ssock)
{
    return (ssock->s);
}

/*
 *  ======== Ssock_recv ========
 */
ssize_t Ssock_recv(Ssock_Handle ssock, void * buf, size_t len, int flags)
{
    ssize_t numRead;

    if (ssock->ctx) {
        numRead = ssock->sec.recv(ssock->ctx, ssock->s, buf, len, flags);
    }
    else {
        numRead = recv(ssock->s, buf, len, flags);
    }

#if defined (NET_SLP)
    if (numRead < 0) {
        if (errno(ssock->s) == EWOULDBLOCK) {
            numRead = Ssock_TIMEOUT;
        }
    }
#else
    if ((numRead == -1) && (errno == EAGAIN)) {
        numRead = Ssock_TIMEOUT;
    }
#endif

    return (numRead);
}

/*
 *  ======== Ssock_recvall ========
 */
ssize_t Ssock_recvall(Ssock_Handle ssock, void * buf, size_t len, int flags)
{
    ssize_t nbytes = 0;

    while (len > 0) {
        nbytes = Ssock_recv(ssock, buf, len, flags);
        if (nbytes > 0) {
            len -= nbytes;
            buf = (uint8_t *)buf + nbytes;
        }
        else {
            break;
        }
    }

    return (nbytes);
}

/*
 *  ======== Ssock_send ========
 */
ssize_t Ssock_send(Ssock_Handle ssock, const void * buf, size_t len, int flags)
{
    ssize_t nbytes;
    ssize_t status = -1;
    uint8_t * cbuf = NULL;
    size_t maxSendLen = MAXSENDLEN - ssock->sec.extraBytes;

    if (ssock->ctx && ssock->sec.encrypt) {
        if ((cbuf = xmalloc(MAXSENDLEN)) == NULL) {
            return (-1);
        }
    }

#ifdef __linux__
    flags |= MSG_NOSIGNAL;
#endif

    /*
     * This is not needed on Linux and NDK stacks, but SL does not handle
     * fragmentation, so we run the same code on all stacks for now to get
     * test coverage on this implementation.
     */
    while (len > 0) {
        ssize_t ilen = len > maxSendLen ? maxSendLen : len;

        if (ssock->ctx && ssock->sec.encrypt) {
            ssize_t clen;
            uint8_t *wbuf = cbuf;

            clen = ssock->sec.encrypt(ssock->ctx, wbuf, buf, ilen);
            do {
                nbytes = send(ssock->s, wbuf, clen, flags);

                if (nbytes > 0) {
                    wbuf += nbytes;
                    clen -= nbytes;
                }
            } while ((nbytes != -1) && (clen > 0));

            /* set to error status or input bytes consumed */
            nbytes = (nbytes == -1 ? -1 : ilen);
        }
        else if (ssock->ctx) {
            nbytes = ssock->sec.send(ssock->ctx, ssock->s, buf, ilen, flags);
        }
        else {
            nbytes = send(ssock->s, (void *)buf, ilen, flags);
        }

        if (nbytes >= 0) {
            len -= nbytes;
            buf = (uint8_t *)buf + nbytes;
            status = (status == -1) ? nbytes : (status + nbytes);
        }
        else {
#ifdef NET_SLP
            nbytes = errno(ssock->s);
            if (nbytes != EWOULDBLOCK) {
                status = -1;
            }
            else if (nbytes == EWOULDBLOCK && status == -1) {
                /* No byte was sent */
                status = 0;
            }
#else
            status = nbytes;
#endif
            break;
        }
    }

    if (cbuf) {
        xfree(cbuf);
    }

    return (status);
}

/*
 *  ======== Ssock_startSecure ========
 */
int Ssock_startSecure(Ssock_Handle ssock, Ssock_SecureFxns * sec, void * ctx)
{
    if (ssock->ctx || !sec || !ctx) {
        return (-1);
    }

    ssock->ctx = ctx;
    ssock->sec = *sec;

    return (0);
}

/*
 *  ======== Ssock_startTLS ========
 */
int Ssock_startTLS(Ssock_Handle ss, TLS_Handle tls)
{
#ifdef NET_WOLFSSL
    return (Ssock_startWolfSSLTLS(ss, tls));
#elif defined(NET_SL)
    return (Ssock_startSimpleLinkTLS(ss, tls));
#else
    return (-1);
#endif
}

/*
 *  ======== Ssock_startTLSWithALPN ========
 */
int Ssock_startTLSWithALPN(Ssock_Handle ss, TLS_Handle tls, char *alpnList)
{
#ifdef NET_WOLFSSL
    return (Ssock_startWolfSSLTLSWithALPN(ss, tls, alpnList));
#elif defined(NET_SL)
    return (Ssock_startSimpleLinkTLSWithALPN(ss, tls, alpnList));
#else
    return (-1);
#endif
}
