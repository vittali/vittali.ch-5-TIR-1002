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
#include <wolfssl/ssl.h>

#include "ssock.h"

static int wolfsslGetError(WOLFSSL *ctx, int ret);
static ssize_t wolfsslSend(const void *ctx, int s, const void *buf,
        size_t len, int flags);
static ssize_t wolfsslRecv(void *ctx, int s, void *buf, size_t len,
        int flags);

static Ssock_SecureFxns fxns = {
    NULL,
    NULL,
    wolfsslSend,
    wolfsslRecv,
    (Ssock_DeleteFxn)wolfSSL_free,
    0
};

/*
 *  ======== wolfsslGetError ========
 */
static int wolfsslGetError(WOLFSSL *ctx, int ret)
{
    int errCode;

    errCode = wolfSSL_get_error(ctx, ret);

    /* TLS/SSL I/O function should be called again later */
    if (errCode == SSL_ERROR_WANT_READ || errCode == SSL_ERROR_WANT_WRITE) {
        errCode = Ssock_TIMEOUT;
    }
    else if (errCode > 0) {
        /* Other positive error codes treated as SSL failures */
        errCode = -1;
    }

    return (errCode);
}

/*
 *  ======== wolfsslSend ========
 */
static ssize_t wolfsslSend(const void *ctx, int s, const void *buf,
        size_t len, int flags)
{
    int ret;

    ret = wolfSSL_send((WOLFSSL *)ctx, buf, len, flags);
    if (ret <= 0) {
        ret = wolfsslGetError((WOLFSSL *)ctx, ret);
    }

    return (ret);
}

/*
 *  ======== wolfsslRecv ========
 */
static ssize_t wolfsslRecv(void *ctx, int s, void *buf, size_t len,
        int flags)
{
    int ret;

    if (flags & MSG_DONTWAIT) {
        ret = (wolfSSL_peek((WOLFSSL *)ctx, buf, len));
        if (ret < 0) {
            return (wolfsslGetError((WOLFSSL *)ctx, ret));
        }

        len = ret;
    }

    ret = wolfSSL_recv((WOLFSSL *)ctx, buf, len, flags);
    if (ret <= 0) {
        ret = wolfsslGetError((WOLFSSL *)ctx, ret);
    }

    return (ret);
}

/*
 *  ======== Ssock_startWolfSSLTLS ========
 */
int Ssock_startWolfSSLTLS(Ssock_Handle ss, TLS_Handle tls)
{
    WOLFSSL *tlsInst;
    int status = -1;

    if (ss && tls) {
        if ((tlsInst = wolfSSL_new((WOLFSSL_CTX *) tls))) {
            wolfSSL_set_fd(tlsInst, Ssock_getSocket(ss));
            if ((status = Ssock_startSecure(ss, &fxns, tlsInst)) == -1) {
                wolfSSL_free(tlsInst);
            }
        }
    }

    return (status);
}

/*
 *  ======== Ssock_startWolfSSLTLSWithALPN ========
 */
int Ssock_startWolfSSLTLSWithALPN(Ssock_Handle ss, TLS_Handle tls,
        char *alpnList)
{
    WOLFSSL *tlsInst;
    int status = -1;

    if (ss && tls) {
        if ((tlsInst = wolfSSL_new((WOLFSSL_CTX *) tls))) {
            wolfSSL_set_fd(tlsInst, Ssock_getSocket(ss));

#ifdef HAVE_SUPPORTED_CURVES
            /* Set up supported ecc required for HK */
            if ((status = wolfSSL_UseSupportedCurve(tlsInst,
                    WOLFSSL_ECC_SECP256R1) != SSL_SUCCESS)) {
                wolfSSL_free(tlsInst);

                return (status);
            }
#endif

            /* Set application protocols for negotiation in TLS handshake */
            if (alpnList) {
                if ((status = wolfSSL_UseALPN(tlsInst, alpnList,
                        strlen(alpnList), WOLFSSL_ALPN_FAILED_ON_MISMATCH))
                        != SSL_SUCCESS) {
                    wolfSSL_free(tlsInst);

                    return (status);
                }
            }

            if ((status = Ssock_startSecure(ss, &fxns, tlsInst)) == -1) {
                wolfSSL_free(tlsInst);
            }
        }
    }

    return (status);
}
