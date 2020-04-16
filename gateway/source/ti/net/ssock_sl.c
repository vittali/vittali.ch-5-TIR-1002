/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
#include <ti/net/tls_sl.h>

#include "ssock.h"

/*
 *  ======== getALPNBitMap ========
 */
uint32_t getALPNBitMap(char *alpnList)
{
    uint32_t alpnBitMap = 0;
    const char delim = ',';
    char *token;
    uint32_t tokenLen;
    uint32_t tokenIndex = 0;
    uint32_t i;
    uint32_t alpnLen;

    alpnLen = strlen(alpnList) + 1;
    for (i = 0; i < alpnLen; i++) {
        if ((alpnList[i] != delim) && (alpnList[i] != '\0')) {
            continue;
        }

        alpnList[i] = '\0';
        token = &alpnList[tokenIndex];
        tokenLen = strlen(token);

        if ((strncmp(token, ALPN_HTTP2_TLS_ID, tokenLen)) == 0) {
            alpnBitMap |= SL_SECURE_ALPN_H2;
        }
        else if ((strncmp(token, ALPN_HTTP2_CLEAR_ID, tokenLen)) == 0) {
            alpnBitMap |= SL_SECURE_ALPN_H2C;
        }

        if ((i + 1) < alpnLen) {
            alpnList[i] = delim;
            tokenIndex = i + 1;
        }
    }

    return (alpnBitMap);
}

/*
 *  ======== Ssock_startSimpleLinkTLS ========
 */
int Ssock_startSimpleLinkTLS(Ssock_Handle ss, TLS_Handle tls)
{
    int skt;
    int ret;
    TLS_SlContext *tlsCtx = (TLS_SlContext *)tls;

    if (!ss || !tls) {
        return (-1);
    }

    skt =  Ssock_getSocket(ss);

    ret = setsockopt(skt, SL_SOL_SOCKET, SL_SO_SECMETHOD, &(tlsCtx->method),
            sizeof(tlsCtx->method));
    if (ret < 0) {
        return (ret);
    }

    if (tlsCtx->caPath) {
        ret = setsockopt(skt, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME,
                tlsCtx->caPath, strlen(tlsCtx->caPath));
        if (ret < 0) {
            return (ret);
        }
    }

    if (tlsCtx->certPath) {
        ret = setsockopt(skt, SL_SOL_SOCKET,
                SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, tlsCtx->certPath,
                strlen(tlsCtx->certPath));
        if (ret < 0) {
            return (ret);
        }
    }

    if (tlsCtx->keyPath) {
        ret = setsockopt(skt, SL_SOL_SOCKET,
                SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, tlsCtx->keyPath,
                strlen(tlsCtx->keyPath));
        if (ret < 0) {
            return (ret);
        }
    }

    if (tlsCtx->dhkeyPath) {
        ret = setsockopt(skt, SL_SOL_SOCKET,
                SL_SO_SECURE_FILES_PEER_CERT_OR_DH_KEY_FILE_NAME,
                tlsCtx->dhkeyPath, strlen(tlsCtx->dhkeyPath));
        if (ret < 0) {
            return (ret);
        }
    }

    return (0);
}

/*
 *  ======== Ssock_startSimpleLinkTLSWithALPN ========
 */
int Ssock_startSimpleLinkTLSWithALPN(Ssock_Handle ss, TLS_Handle tls,
        char *alpnList)
{
    int ret = 0;

    SlSockSecureALPN_t alpn;

    ret = Ssock_startSimpleLinkTLS(ss, tls);
    if (ret < 0) {
        return (ret);
    }

    if (alpnList) {
        alpn.SecureALPN = getALPNBitMap(alpnList);
        ret = setsockopt(ss->s, SL_SOL_SOCKET, SL_SO_SECURE_ALPN, &alpn,
                 sizeof(SlSockSecureALPN_t));
        if (ret < 0) {
            return (ret);
        }
    }

    return (0);
}
