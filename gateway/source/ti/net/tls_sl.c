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

#include "tls.h"
#include "tls_sl.h"

/*
 *  ======== flashFile ========
 *  Flashes file to the SFLASH using SimpleLink WiFi APIs
 *
 *  Returns number of bytes written on success or < 0 on failure
 */
static int flashFile(const char *path, const uint8_t *buffer, uint32_t len)
{
    int32_t ret = -1;
    int32_t fileHandle;

    if (path && buffer) {
        fileHandle = sl_FsOpen((unsigned char *)path,
                SL_FS_CREATE | SL_FS_CREATE_SECURE | SL_FS_CREATE_NOSIGNATURE
                | SL_FS_CREATE_PUBLIC_WRITE | SL_FS_OVERWRITE
                | SL_FS_CREATE_MAX_SIZE(len), NULL);
        if (fileHandle > 0) {
            ret = sl_FsWrite(fileHandle, 0, (unsigned char *)buffer, len);
            sl_FsClose(fileHandle, NULL, NULL, 0);
        }
    }

    return (ret);
}

/*
 *  ======== fileExists ========
 *  Check if the file exists on SFLASH
 */
static bool fileExists(const char *filePath)
{
    int32_t ret;
    SlFsFileInfo_t fsFileInfo;

    ret = sl_FsGetInfo((uint8_t *)filePath, 0, &fsFileInfo);
    if (ret == 0) {
        return (true);
    }

    return (false);
}

/*
 *  ======== getTLSMethod ========
 */
static int8_t getTLSMethod(TLS_Method method)
{
    switch (method) {
        case TLS_METHOD_CLIENT_TLSV1:
        case TLS_METHOD_SERVER_TLSV1:
            return (SL_SO_SEC_METHOD_TLSV1);
        case TLS_METHOD_CLIENT_TLSV1_1:
        case TLS_METHOD_SERVER_TLSV1_1:
            return (SL_SO_SEC_METHOD_TLSV1_1);
        case TLS_METHOD_CLIENT_TLSV1_2:
        case TLS_METHOD_SERVER_TLSV1_2:
        default:
            return (SL_SO_SEC_METHOD_TLSV1_2);
    }
}

/*
 *  ======== setFilePath ========
 */
static int setFilePath(TLS_SlContext *tls, TLS_Cert_Type type,
        const char *filePath)
{
    int ret = 0;

    switch (type) {
        case TLS_CERT_TYPE_CA:
            tls->caPath = filePath;
            break;

        case TLS_CERT_TYPE_CERT:
            tls->certPath = filePath;
            break;

        case TLS_CERT_TYPE_KEY:
            tls->keyPath = filePath;
            break;

        case TLS_CERT_TYPE_DH_KEY:
            tls->dhkeyPath = filePath;
            break;

        default:
            ret = TLS_EINVALIDPARAMS;
    }

    return (ret);
}

/*
 *  ======== TLS_create ========
 */
TLS_Handle TLS_create(TLS_Method method)
{
    TLS_SlContext *tls;

    tls = (TLS_SlContext *) calloc(1, sizeof(TLS_SlContext));
    if (tls) {
        tls->method.SecureMethod = getTLSMethod(method);
    }

    return (tls);
}

/*
 *  ======== TLS_delete ========
 */
void TLS_delete(TLS_Handle *tls)
{
    if (tls && *tls) {
        free(*tls);
        *tls = NULL;
    }
}

/*
 *  ======== TLS_setCertFile ========
 */
int TLS_setCertFile(TLS_Handle tls, TLS_Cert_Type type, TLS_Cert_Format format,
        const char *filePath)
{
    xassert(tls != NULL);
    xassert(filePath && fileExists(filePath));

    return (setFilePath(tls, type, filePath));
}

/*
 *  ======== TLS_setCertBuf ========
 */
int TLS_setCertBuf(TLS_Handle tls, TLS_Cert_Type type, TLS_Cert_Format format,
        uint8_t *buf, uint32_t buflen)
{
    return (TLS_ENOTSUPPORTED);
}

/*
 *  ======== TLS_writeDerFile ========
 */
int TLS_writeDerFile(uint8_t *buf, uint32_t buflen, TLS_Cert_Format format,
        const char *filePath)
{
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
        ret = flashFile(filePath, der, derlen);
        ret = (ret < 0) ? TLS_ECERTWRITEFAIL : 0;

        if (format == TLS_CERT_FORMAT_PEM) {
            CertConv_free(&der);
        }
    }

    return (ret);
}

/*
 *  ======== TLS_writePemFile ========
 */
int TLS_writePemFile(uint8_t *buf, uint32_t buflen, TLS_Cert_Type type,
        TLS_Cert_Format format, const char *filePath)
{
    int ret;

    xassert(buf && buflen);
    xassert(filePath != NULL);
    xassert(format == TLS_CERT_FORMAT_PEM);

    ret = flashFile(filePath, buf, buflen);
    ret = (ret < 0) ? TLS_ECERTWRITEFAIL : 0;

    return (ret);
}
