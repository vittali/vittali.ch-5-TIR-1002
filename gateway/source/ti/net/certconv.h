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
#ifndef ti_net_CertConv__include
#define ti_net_CertConv__include

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/*
 *  @brief Convert certificate from PEM to DER format
 *
 *  @param[in] pem   PEM certificate buffer
 *
 *  @param[in] plen  Length of `pem` buffer
 *
 *  @param[out] der  Pointer that will hold the converted certificate in
 *                   DER format
 *
 *  @param[out] dlen  Length of allocated buffer pointed by `der`
 *
 *  @return 0 on success or < 0 on failure
 */
extern int CertConv_pem2der(const uint8_t *pem, uint32_t plen, uint8_t **der,
         uint32_t *dlen);

/*
 *  @brief Free the previously allocated buffer
 *
 *  @param[out] der  Pointer to DER certificate buffer
 */
extern void CertConv_free(uint8_t **der);

#ifdef __cplusplus
}
#endif
#endif
