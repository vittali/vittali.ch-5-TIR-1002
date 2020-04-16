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
#include <stdbool.h>

#include <ti/net/common.h>
#include <ti/net/certconv.h>

#include <wolfssl/ssl.h>

#include "tls.h"

#ifndef NET_NDK
/*
 *  ======== writeFile ========
 *  Returns number of characters written on success or < 0 on failure
 */
static int writeFile(const char *path, const uint8_t *buffer, uint32_t len)
{
    FILE *file;
    int ret = -1;

    if (path && buffer) {
        file = fopen(path, "w");
        if (file) {
            ret = fwrite(buffer, sizeof(uint8_t), len, file);
            fclose(file);
        }
    }

    return (ret);
}

/*
 *  ======== fileExists ========
 */
static bool fileExists(const char *filename)
{
    FILE *file;

    if (filename) {
        file = fopen(filename, "r");
        if (file) {
            fclose(file);

            return (true);
        }
    }

    return (false);
}
#endif

/*
 *  ======== getTLSMethod ========
 */
static WOLFSSL_METHOD *getTLSMethod(TLS_Method method)
{
    switch (method) {
        case TLS_METHOD_CLIENT_TLSV1:
            return (wolfTLSv1_client_method());
        case TLS_METHOD_CLIENT_TLSV1_1:
            return (wolfTLSv1_1_client_method());
        case TLS_METHOD_CLIENT_TLSV1_2:
            return (wolfTLSv1_2_client_method());
        case TLS_METHOD_SERVER_TLSV1:
            return (wolfTLSv1_server_method());
        case TLS_METHOD_SERVER_TLSV1_1:
            return (wolfTLSv1_1_server_method());
        case TLS_METHOD_SERVER_TLSV1_2:
            return (wolfTLSv1_2_server_method());
        default:
            return (NULL);
    }
}

/*
 *  ======== TLS_create ========
 */
TLS_Handle TLS_create(TLS_Method method)
{
    WOLFSSL_CTX *context = NULL;
    WOLFSSL_METHOD *wmethod;

    wmethod = getTLSMethod(method);
    if (wmethod) {
        context = wolfSSL_CTX_new(wmethod);
    }

    return ((TLS_Handle) context);
}

/*
 *  ======== TLS_delete ========
 */
void TLS_delete(TLS_Handle *tls)
{
    if (tls && *tls) {
        wolfSSL_CTX_free((WOLFSSL_CTX *) *tls);
        wolfSSL_Cleanup();
        tls = NULL;
    }
}

/*
 *  ======== TLS_setCertFile ========
 */
int TLS_setCertFile(TLS_Handle tls, TLS_Cert_Type type, TLS_Cert_Format format,
        const char *filePath)
{
#ifdef NET_NDK
    return (TLS_ENOTSUPPORTED);

#else
    int ret;
    int fmt;

    xassert(tls != NULL);
    xassert(filePath && fileExists(filePath));

    if (format == TLS_CERT_FORMAT_PEM) {
        fmt = SSL_FILETYPE_PEM;
    }
    else {
        fmt = SSL_FILETYPE_ASN1;
    }

    switch (type) {
        case TLS_CERT_TYPE_CA:
            ret = wolfSSL_CTX_der_load_verify_locations(tls, filePath, fmt);
            break;

        case TLS_CERT_TYPE_CERT:
            ret = wolfSSL_CTX_use_certificate_file(tls, filePath, fmt);
            break;

        case TLS_CERT_TYPE_KEY:
            ret = wolfSSL_CTX_use_PrivateKey_file(tls, filePath, fmt);
            break;

        default:
            ret = -1;
    }

    return ((ret == SSL_SUCCESS) ? 0 : TLS_ECERTLOADFAIL);
#endif
}

/*
 *  ======== TLS_setCertBuf ========
 */
int TLS_setCertBuf(TLS_Handle tls, TLS_Cert_Type type, TLS_Cert_Format format,
        uint8_t *buf, uint32_t buflen)
{
    int ret;
    uint8_t *der = NULL;
    uint32_t derlen;

    xassert(tls != NULL);
    xassert(buf && buflen);

    if (format == TLS_CERT_FORMAT_PEM) {
        ret = CertConv_pem2der(buf, buflen, &der, &derlen);
        if (ret < 0) {
            ret = TLS_EALLOCFAIL;
        }
    }
    else {
        der = buf;
        derlen = buflen;
    }

    switch (type) {
        case TLS_CERT_TYPE_CA:
            ret = wolfSSL_CTX_load_verify_buffer(tls, der, derlen,
                    SSL_FILETYPE_ASN1);
            break;

        case TLS_CERT_TYPE_CERT:
            ret = wolfSSL_CTX_use_certificate_buffer(tls, der, derlen,
                    SSL_FILETYPE_ASN1);
            break;

        case TLS_CERT_TYPE_KEY:
            ret = wolfSSL_CTX_use_PrivateKey_buffer(tls, der, derlen,
                    SSL_FILETYPE_ASN1);
            break;

        default:
             ret = -1;
    }

    if (format == TLS_CERT_FORMAT_PEM) {
        CertConv_free(&der);
    }

    return ((ret == SSL_SUCCESS) ? 0 : TLS_ECERTLOADFAIL);
}

/*
 *  ======== TLS_writeDerFile ========
 */
int TLS_writeDerFile(uint8_t *buf, uint32_t buflen, TLS_Cert_Format format,
        const char *filePath)
{
#ifdef NET_NDK
    return (TLS_ENOTSUPPORTED);

#else
    int ret = 0;
    uint8_t *der = NULL;
    uint32_t derlen;

    xassert(buf && buflen);
    xassert(filePath != NULL);

    if (format == TLS_CERT_FORMAT_PEM) {
        ret = CertConv_pem2der(buf, buflen, &der, &derlen);
        if (ret < 0) {
            ret = TLS_EALLOCFAIL;
        }
    }
    else {
        der = buf;
        derlen = buflen;
    }

    if (ret == 0) {
        ret = writeFile(filePath, der, derlen);
        ret = (ret < 0) ? TLS_ECERTWRITEFAIL : 0;

        if (format == TLS_CERT_FORMAT_PEM) {
            CertConv_free(&der);
        }
    }

    return (ret);
#endif
}

/*
 *  ======== TLS_writePemFile ========
 */
int TLS_writePemFile(uint8_t *buf, uint32_t buflen, TLS_Cert_Type type,
        TLS_Cert_Format format, const char *filePath)
{
#ifdef NET_NDK
    return (TLS_ENOTSUPPORTED);

#else
    int ret = 0;

    xassert(buf && buflen);
    xassert(filePath != NULL);
    xassert(format == TLS_CERT_FORMAT_PEM);

    ret = writeFile(filePath, buf, buflen);
    ret = (ret < 0) ? TLS_ECERTWRITEFAIL : 0;

    return (ret);
#endif
}
