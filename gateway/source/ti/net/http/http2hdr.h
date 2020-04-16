/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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
#ifndef ti_net_http_HTTP2Hdr__include
#define ti_net_http_HTTP2Hdr__include

#ifdef __cplusplus
extern "C" {
#endif
/*!
 *  @file ti/net/http/http2hdr.h
 *
 *  @brief Header field utility module
 */
#include <stdint.h>

#define HTTP2Hdr_EINSUFFICIENTHEAP      (-101)

/*!
 *  @brief HTTP/2 request header field
 */
typedef struct HTTP2Hdr_Field {
    char *name;        /*!< Field name,  ex: "Accept" */
    char *value;       /*!< Field value, ex: "text/plain" */
} HTTP2Hdr_Field;

/*!
 *  @brief Add a new Header Field to the list, and assign name and value
 *
 *  @remarks The name and the value strings must be on heap and should not
 *           be freed unless this function returns an error. After the
 *           'headersList' has been processed, a call to HTTP2Hdr_free()
 *           will free the 'headersList' and all members of each
 *           'HTTP2Hdr_Field' in the 'headersList'.
 *
 *  @param[in]  headersList   List of HTTP/2 Header Fields
 *
 *  @param[in]  len           Length of `headersList` list
 *
 *  @param[out] name          Field name to be assigned in the Header Field
 *
 *  @param[out] value         Field value to be assigned in the Header Field
 *
 *  @return number of headers in 'headersList' on success or < 0 on failure
 */
extern int HTTP2Hdr_add(HTTP2Hdr_Field **headersList, uint32_t len, char *name,
        char *value);

/*!
 *  @brief Free the Header Field list, and the name and the value in each Field.
 *
 *  @param[in]  headersList   List of HTTP/2 Fields
 *
 *  @param[in]  len           Length of `headersList` list
 */
extern void HTTP2Hdr_free(HTTP2Hdr_Field **headersList, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif
