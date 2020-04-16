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
#include "certconv.h"

#define PEM_META     "-----"
#define PEM_META_LEN (sizeof(PEM_META) - 1)

/*
 *  ======== base64Decode ========
 *  Returns the decoded value
 */
static uint8_t base64Decode(uint8_t ch)
{
    uint8_t ret = 0;
    if (ch >= 'A' && ch <= 'Z') {
        ret = ch - 'A';
    }
    else if (ch >= 'a' && ch <= 'z') {
        ret = ch - 'a' + 26;
    }
    else if (ch >= '0' && ch <= '9') {
        ret = ch - '0' + 52;
    }
    else if (ch == '+') {
        ret = 62;
    }
    else if (ch == '/') {
        ret = 63;
    }

    return (ret);
}

/*
 *  ======== skipMetaData ========
 *  Skips data of format:
 *
 *  -----BEGIN CERTIFICATE-----
 *  -----END CERTIFICATE-----
 *
 *  Returns the number of bytes skipped on success or -1 on failure
 */
static int skipMetaData(const uint8_t *buf, uint32_t len)
{
    int ret = -1;
    uint32_t i;

    /* Minimum size expected */
    if (len > PEM_META_LEN) {
        /* compare with starting dashes */
        if (strncmp((const char *)buf, PEM_META, PEM_META_LEN) == 0) {
            /* skip the chars */
            for (i = PEM_META_LEN; i < len; i++) {
                if (buf[i] == '-') {
                    break;
                }
            }

            /* compare with trailing dashes */
            if (len >= (i + PEM_META_LEN)) {
                if (strncmp((const char *)(buf + i), PEM_META, PEM_META_LEN)
                        == 0) {
                    ret = i + PEM_META_LEN;
                }
            }
        }
    }

    return (ret);
}

/*
 *  ======== CertConv_pem2der ========
 *  Returns 0 on success or -1 on failure
 */
int CertConv_pem2der(const uint8_t *pem, uint32_t plen, uint8_t **der,
         uint32_t *dlen)
{
    int skipCount;
    uint32_t i;
    uint32_t j;
    uint8_t decodedByte = 0;
    uint8_t byteCount = 0;
    uint32_t decodedValue = 0;
    uint32_t padZero = 0;
    uint8_t *derPtr;
    uint32_t derLen;
    uint8_t pemByte;

    if ((pem == NULL) || (der == NULL)) {
        return (-1);
    }

    /* Base64 decode: 4 characters to 3 bytes */
    derLen = (plen / 4) * 3;
    derPtr = (uint8_t *) calloc(derLen, sizeof(uint8_t));
    if (!derPtr) {
        return (-1);
    }

    j = 0;
    for (i = 0; i < plen; i++) {
        pemByte = pem[i];
        if (pemByte == '-') {
            skipCount = skipMetaData(&pem[i], (plen - i));
            if (skipCount < 0) {
                free(derPtr);

                return (-1);
            }

            /* -1 as continuing to loop will increment to the next data */
            i += skipCount - 1;
            continue;
        }
        else if ((pemByte == '\r') || (pemByte == '\n')) {
            /* skipping characters that are not part of Base64 value */
            continue;
        }

        decodedByte = base64Decode(pemByte);
        if (!decodedByte && (pemByte == '=')) {
            padZero++;
        }

        /* Re-create 3 byte value */
        decodedValue = (decodedValue << 6) | decodedByte;
        byteCount++;
        if (byteCount == 4) {
            if ((j + 2) < derLen) {
                /* Add the 3 bytes to der buffer */
                byteCount = 3;
                while (byteCount--) {
                    decodedByte = (decodedValue >> (8 * byteCount)) & 0xFF;
                    /* Discard the padding */
                    if (!decodedByte && (pemByte == '=')) {
                        break;
                    }
                    derPtr[j++] = decodedByte;
                }
            }
            else {
                free(derPtr);

                return (-1);
            }

            decodedValue = 0;
            byteCount = 0;
        }
    }

    *dlen = derLen - padZero;
    *der = derPtr;

    return (0);
}

/*
 *  ======== CertConv_free ========
 */
void CertConv_free(uint8_t **der)
{
    if (der && *der) {
        xfree(*der);
        *der = NULL;
    }
}
