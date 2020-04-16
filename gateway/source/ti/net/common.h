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

#ifndef ti_net_Common__include
#define ti_net_Common__include

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __linux__
#include <stdio.h>
#else
#include <xdc/runtime/System.h>
#endif


/*
 *  ======== xassert ========
 */
static inline void xassert(int expr)
{
    assert(expr);
}

/*
 *  ======== xfree ========
 */
static inline void xfree(void *ptr)
{
    free(ptr);
}

/*
 *  ======== xmalloc ========
 */
static inline void *xmalloc(size_t size)
{
    return (malloc(size));
}

/*
 *  ======== xrealloc ========
 */
static inline void *xrealloc(void *ptr, size_t size)
{
    return (realloc(ptr, size));
}

/*
 *  ======== xvsnprintf ========
 */
static inline int xvsnprintf(char *s, size_t n, const char *fmt, va_list arg)
{
#ifdef __linux__
    return (vsnprintf(s, n, fmt, arg));
#else
    return (System_vsnprintf(s, n, fmt, arg));
#endif
}

/*
 *  ======== xsnprintf ========
 */
static inline int xsnprintf(char *s, size_t n, const char *fmt, ...)
{
    int len;
    va_list ap;

    va_start(ap, fmt);
    len = xvsnprintf(s, n, fmt, ap);
    va_end(ap);

    return (len);
}

#ifdef __cplusplus
}
#endif
#endif
