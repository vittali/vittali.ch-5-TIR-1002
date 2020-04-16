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
#include <stdlib.h>
#include <string.h>

#include "http2hdr.h"

/*
 *  ======== freeField ========
 */
static void freeField(HTTP2Hdr_Field *header)
{
    if (header) {
        free(header->name);
        free(header->value);
    }
}

/*
 *  ======== HTTP2Hdr_free ========
 */
void HTTP2Hdr_free(HTTP2Hdr_Field **headersList, uint16_t len)
{
    uint16_t i;

    if (headersList && *headersList) {

        for (i = 0; i < len; i++) {
            freeField(*headersList + i);
        }

        free(*headersList);
        *headersList = NULL;
    }
}

/*
 *  ======== HTTP2Hdr_add ========
 *  Allocates and appends a new header to 'headersList'
 *
 *  Returns the number of headers in 'headersList' or < 0 on failure.
 */
int HTTP2Hdr_add(HTTP2Hdr_Field **headersList, uint32_t len, char *name,
        char *value)
{
    HTTP2Hdr_Field *hl;

    hl = (HTTP2Hdr_Field *) realloc(*headersList,
            ((len + 1) * sizeof(HTTP2Hdr_Field)));
    if (hl) {
        hl[len].name = name;
        hl[len].value = value;

        *headersList = hl;
        len++;

        return (len);
    }
    else {
        HTTP2Hdr_free(headersList, len);

        return (HTTP2Hdr_EINSUFFICIENTHEAP);
    }
}
