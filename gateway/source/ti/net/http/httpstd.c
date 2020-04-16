/*
 * Copyright (c) 2012-2016, Texas Instruments Incorporated
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
 * */
/*
 * ======== httpstd.c ========
 *
 */
#include <ti/net/http/httpstd.h>
/*!
 *  @file ti/net/http/httpstd.c
 *
 *  @ingroup ti_net_http_HTTPCli ti_net_http_HTTP2Cli
 *  @{
 */
/*!
 *  @cond DONT_INCLUDE
 */
const char *HTTPStd_VER     = "HTTP/1.1";
/*! @endcond */
/*!
 *  @name HTTP Request Methods
 *  @{
 */
const char *HTTPStd_GET     = "GET";
const char *HTTPStd_POST    = "POST";
const char *HTTPStd_HEAD    = "HEAD";
const char *HTTPStd_OPTIONS = "OPTIONS";
const char *HTTPStd_PUT     = "PUT";
const char *HTTPStd_DELETE  = "DELETE";
const char *HTTPStd_CONNECT = "CONNECT";
const char *HTTPStd_PATCH   = "PATCH";
/*! @} */

/*!
 *  @name HTTP Content Types
 *  @{
 */
const char *HTTPStd_CONTENT_TYPE_APPLET  = "application/octet-stream ";
const char *HTTPStd_CONTENT_TYPE_AU      = "audio/au ";
const char *HTTPStd_CONTENT_TYPE_CSS     = "text/css ";
const char *HTTPStd_CONTENT_TYPE_DOC     = "application/msword ";
const char *HTTPStd_CONTENT_TYPE_GIF     = "image/gif ";
const char *HTTPStd_CONTENT_TYPE_HTML    = "text/html ";
const char *HTTPStd_CONTENT_TYPE_JPG     = "image/jpeg ";
const char *HTTPStd_CONTENT_TYPE_MPEG    = "video/mpeg ";
const char *HTTPStd_CONTENT_TYPE_PDF     = "application/pdf ";
const char *HTTPStd_CONTENT_TYPE_WAV     = "audio/wav ";
const char *HTTPStd_CONTENT_TYPE_ZIP     = "application/zip ";
const char *HTTPStd_CONTENT_TYPE_PLAIN   = "text/plain ";
/*! @} */

/*!
 *  @name HTTP Header Fields
 *  @{
 */
const char *HTTPStd_FIELD_NAME_ACCEPT              = "Accept";
const char *HTTPStd_FIELD_NAME_ACCEPT_CHARSET      = "Accept-Charset";
const char *HTTPStd_FIELD_NAME_ACCEPT_ENCODING     = "Accept-Encoding";
const char *HTTPStd_FIELD_NAME_ACCEPT_LANGUAGE     = "Accept-Language";
const char *HTTPStd_FIELD_NAME_ACCEPT_RANGES       = "Accept-Ranges";
const char *HTTPStd_FIELD_NAME_AGE                 = "Age";
const char *HTTPStd_FIELD_NAME_ALLOW               = "Allow";
const char *HTTPStd_FIELD_NAME_AUTHORIZATION       = "Authorization";
const char *HTTPStd_FIELD_NAME_CACHE_CONTROL       = "Cache-Control";
const char *HTTPStd_FIELD_NAME_CONNECTION          = "Connection";
const char *HTTPStd_FIELD_NAME_CONTENT_ENCODING    = "Content-Encoding";
const char *HTTPStd_FIELD_NAME_CONTENT_LANGUAGE    = "Content-Language";
const char *HTTPStd_FIELD_NAME_CONTENT_LENGTH      = "Content-Length";
const char *HTTPStd_FIELD_NAME_CONTENT_LOCATION    = "Content-Location";
const char *HTTPStd_FIELD_NAME_CONTENT_MD5         = "Content-MD5";
const char *HTTPStd_FIELD_NAME_CONTENT_RANGE       = "Content-Range";
const char *HTTPStd_FIELD_NAME_CONTENT_TYPE        = "Content-Type";
const char *HTTPStd_FIELD_NAME_COOKIE              = "Cookie";
const char *HTTPStd_FIELD_NAME_DATE                = "Date";
const char *HTTPStd_FIELD_NAME_ETAG                = "ETag";
const char *HTTPStd_FIELD_NAME_EXPECT              = "Expect";
const char *HTTPStd_FIELD_NAME_EXPIRES             = "Expires";
const char *HTTPStd_FIELD_NAME_FROM                = "From";
const char *HTTPStd_FIELD_NAME_HOST                = "Host";
const char *HTTPStd_FIELD_NAME_IF_MATCH            = "If-Match";
const char *HTTPStd_FIELD_NAME_IF_MODIFIED_SINCE   = "If-Modified-Since";
const char *HTTPStd_FIELD_NAME_IF_NONE_MATCH       = "If-None-Match";
const char *HTTPStd_FIELD_NAME_IF_RANGE            = "If-Range";
const char *HTTPStd_FIELD_NAME_IF_UNMODIFIED_SINCE = "If-Unmodified-Since";
const char *HTTPStd_FIELD_NAME_LAST_MODIFIED       = "Last-Modified";
const char *HTTPStd_FIELD_NAME_LOCATION            = "Location";
const char *HTTPStd_FIELD_NAME_MAX_FORWARDS        = "Max-Forwards";
const char *HTTPStd_FIELD_NAME_ORIGIN              = "Origin";
const char *HTTPStd_FIELD_NAME_PRAGMA              = "Pragma";
const char *HTTPStd_FIELD_NAME_PROXY_AUTHENTICATE  = "Proxy-Authenticate";
const char *HTTPStd_FIELD_NAME_PROXY_AUTHORIZATION = "Proxy-Authorization";
const char *HTTPStd_FIELD_NAME_RANGE               = "Range";
const char *HTTPStd_FIELD_NAME_REFERER             = "Referer";
const char *HTTPStd_FIELD_NAME_RETRY_AFTER         = "Retry-After";
const char *HTTPStd_FIELD_NAME_SERVER              = "Server";
const char *HTTPStd_FIELD_NAME_TE                  = "TE";
const char *HTTPStd_FIELD_NAME_TRAILER             = "Trailer";
const char *HTTPStd_FIELD_NAME_TRANSFER_ENCODING   = "Transfer-Encoding";
const char *HTTPStd_FIELD_NAME_UPGRADE             = "Upgrade";
const char *HTTPStd_FIELD_NAME_USER_AGENT          = "User-Agent";
const char *HTTPStd_FIELD_NAME_VARY                = "Vary";
const char *HTTPStd_FIELD_NAME_VIA                 = "Via";
const char *HTTPStd_FIELD_NAME_WWW_AUTHENTICATE    = "WWW-Authenticate";
const char *HTTPStd_FIELD_NAME_WARNING             = "Warning";
const char *HTTPStd_FIELD_NAME_X_FORWARDED_FOR     = "X-Forwarded-For";
/*! @} */
/*! @} */
