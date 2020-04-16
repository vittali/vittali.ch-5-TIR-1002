/*
 * Copyright (c) 2014-2016, Texas Instruments Incorporated
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
#ifndef ti_net_http_HTTPStd__include
#define ti_net_http_HTTPStd__include
#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @file ti/net/http/httpstd.h
 *
 *  @brief Standard Definitions for HTTP Status Codes, Content Type and Fields
 *
 *  @ingroup ti_net_http_HTTPCli ti_net_http_HTTP2Cli
 *  @{
 */
/*!
 *  @name HTTP Standard Ports
 *  @{
 */
#define HTTPStd_PORT        80
#define HTTPStd_SECURE_PORT 443
/*! @} */

/*!
 *  @name HTTP Status Codes
 *  @{
 */
enum HTTPStd_STATUS_CODE {
    HTTPStd_CONTINUE                          = 100, /*!< Informational */
    HTTPStd_SWITCHING_PROTOCOLS               = 101, /*!< Informational */

    HTTPStd_OK                                = 200, /*!< Success */
    HTTPStd_CREATED                           = 201, /*!< Success */
    HTTPStd_ACCEPTED                          = 202, /*!< Success */
    HTTPStd_NON_AUTHORITATIVE_INFORMATION     = 203, /*!< Success */
    HTTPStd_NO_CONTENT                        = 204, /*!< Success */
    HTTPStd_RESET_CONTENT                     = 205, /*!< Success */
    HTTPStd_PARTIAL_CONTENT                   = 206, /*!< Success */
    HTTPStd_MULTI_STATUS                      = 207, /*!< Success */

    HTTPStd_MULTIPLE_CHOICES                  = 300, /*!< Redirection */
    HTTPStd_MOVED_PERMANENTLY                 = 301, /*!< Redirection */
    HTTPStd_FOUND                             = 302, /*!< Redirection */
    HTTPStd_SEE_OTHER                         = 303, /*!< Redirection */
    HTTPStd_NOT_MODIFIED                      = 304, /*!< Redirection */
    HTTPStd_USE_PROXY                         = 305, /*!< Redirection */
    HTTPStd_TEMPORARY_REDIRECT                = 307, /*!< Redirection */

    HTTPStd_BAD_REQUEST                       = 400, /*!< Client Error */
    HTTPStd_UNAUTHORIZED                      = 401, /*!< Client Error */
    HTTPStd_PAYMENT_REQUIRED                  = 402, /*!< Client Error */
    HTTPStd_FORBIDDEN                         = 403, /*!< Client Error */
    HTTPStd_NOT_FOUND                         = 404, /*!< Client Error */
    HTTPStd_METHOD_NOT_ALLOWED                = 405, /*!< Client Error */
    HTTPStd_NOT_ACCEPTABLE                    = 406, /*!< Client Error */
    HTTPStd_PROXY_AUTHENTICATION_REQUIRED     = 407, /*!< Client Error */
    HTTPStd_REQUEST_TIMEOUT                   = 408, /*!< Client Error */
    HTTPStd_CONFLICT                          = 409, /*!< Client Error */
    HTTPStd_GONE                              = 410, /*!< Client Error */
    HTTPStd_LENGTH_REQUIRED                   = 411, /*!< Client Error */
    HTTPStd_PRECONDITION_FAILED               = 412, /*!< Client Error */
    HTTPStd_REQUEST_ENTITY_TOO_LARGE          = 413, /*!< Client Error */
    HTTPStd_REQUEST_URI_TOO_LONG              = 414, /*!< Client Error */
    HTTPStd_UNSUPPORTED_MEDIA_TYPE            = 415, /*!< Client Error */
    HTTPStd_REQUESTED_RANGE_NOT_SATISFAIABLE  = 416, /*!< Client Error */
    HTTPStd_EXPECTATION_FAILED                = 417, /*!< Client Error */
    HTTPStd_UNPROCESSABLE_ENTITY              = 422, /*!< Client Error */
    HTTPStd_TOO_MANY_REQUESTS                 = 429, /*!< Client Error */

    HTTPStd_INTERNAL_SERVER_ERROR             = 500, /*!< Server Error */
    HTTPStd_NOT_IMPLEMENTED                   = 501, /*!< Server Error */
    HTTPStd_BAD_GATEWAY                       = 502, /*!< Server Error */
    HTTPStd_SERVICE_UNAVAILABLE               = 503, /*!< Server Error */
    HTTPStd_GATEWAY_TIMEOUT                   = 504, /*!< Server Error */
    HTTPStd_HTTP_VERSION_NOT_SUPPORTED        = 505, /*!< Server Error */

    HTTPStd_STATUS_CODE_END                   = 600
};
/*! @} */
/*!
 * @cond DONT_INCLUDE
 */
extern const char *HTTPStd_GET;
extern const char *HTTPStd_POST;
extern const char *HTTPStd_HEAD;
extern const char *HTTPStd_OPTIONS;
extern const char *HTTPStd_PUT;
extern const char *HTTPStd_DELETE;
extern const char *HTTPStd_CONNECT;
extern const char *HTTPStd_PATCH;

extern const char *HTTPStd_CONTENT_TYPE_APPLET;
extern const char *HTTPStd_CONTENT_TYPE_AU;
extern const char *HTTPStd_CONTENT_TYPE_CSS;
extern const char *HTTPStd_CONTENT_TYPE_DOC;
extern const char *HTTPStd_CONTENT_TYPE_GIF;
extern const char *HTTPStd_CONTENT_TYPE_HTML;
extern const char *HTTPStd_CONTENT_TYPE_JPG;
extern const char *HTTPStd_CONTENT_TYPE_MPEG;
extern const char *HTTPStd_CONTENT_TYPE_PDF;
extern const char *HTTPStd_CONTENT_TYPE_WAV;
extern const char *HTTPStd_CONTENT_TYPE_ZIP;
extern const char *HTTPStd_CONTENT_TYPE_PLAIN;

extern const char *HTTPStd_VER;

extern const char *HTTPStd_FIELD_NAME_ACCEPT;
extern const char *HTTPStd_FIELD_NAME_ACCEPT_CHARSET;
extern const char *HTTPStd_FIELD_NAME_ACCEPT_ENCODING;
extern const char *HTTPStd_FIELD_NAME_ACCEPT_LANGUAGE;
extern const char *HTTPStd_FIELD_NAME_ACCEPT_RANGES;
extern const char *HTTPStd_FIELD_NAME_AGE;
extern const char *HTTPStd_FIELD_NAME_ALLOW;
extern const char *HTTPStd_FIELD_NAME_AUTHORIZATION;
extern const char *HTTPStd_FIELD_NAME_CACHE_CONTROL;
extern const char *HTTPStd_FIELD_NAME_CONNECTION;
extern const char *HTTPStd_FIELD_NAME_CONTENT_ENCODING;
extern const char *HTTPStd_FIELD_NAME_CONTENT_LANGUAGE;
extern const char *HTTPStd_FIELD_NAME_CONTENT_LENGTH;
extern const char *HTTPStd_FIELD_NAME_CONTENT_LOCATION;
extern const char *HTTPStd_FIELD_NAME_CONTENT_MD5;
extern const char *HTTPStd_FIELD_NAME_CONTENT_RANGE;
extern const char *HTTPStd_FIELD_NAME_CONTENT_TYPE;
extern const char *HTTPStd_FIELD_NAME_COOKIE;
extern const char *HTTPStd_FIELD_NAME_DATE;
extern const char *HTTPStd_FIELD_NAME_ETAG;
extern const char *HTTPStd_FIELD_NAME_EXPECT;
extern const char *HTTPStd_FIELD_NAME_EXPIRES;
extern const char *HTTPStd_FIELD_NAME_FROM;
extern const char *HTTPStd_FIELD_NAME_HOST;
extern const char *HTTPStd_FIELD_NAME_IF_MATCH;
extern const char *HTTPStd_FIELD_NAME_IF_MODIFIED_SINCE;
extern const char *HTTPStd_FIELD_NAME_IF_NONE_MATCH;
extern const char *HTTPStd_FIELD_NAME_IF_RANGE;
extern const char *HTTPStd_FIELD_NAME_IF_UNMODIFIED_SINCE;
extern const char *HTTPStd_FIELD_NAME_LAST_MODIFIED;
extern const char *HTTPStd_FIELD_NAME_LOCATION;
extern const char *HTTPStd_FIELD_NAME_MAX_FORWARDS;
extern const char *HTTPStd_FIELD_NAME_ORIGIN;
extern const char *HTTPStd_FIELD_NAME_PRAGMA;
extern const char *HTTPStd_FIELD_NAME_PROXY_AUTHENTICATE;
extern const char *HTTPStd_FIELD_NAME_PROXY_AUTHORIZATION;
extern const char *HTTPStd_FIELD_NAME_RANGE;
extern const char *HTTPStd_FIELD_NAME_REFERER;
extern const char *HTTPStd_FIELD_NAME_RETRY_AFTER;
extern const char *HTTPStd_FIELD_NAME_SERVER;
extern const char *HTTPStd_FIELD_NAME_TE;
extern const char *HTTPStd_FIELD_NAME_TRAILER;
extern const char *HTTPStd_FIELD_NAME_TRANSFER_ENCODING;
extern const char *HTTPStd_FIELD_NAME_UPGRADE;
extern const char *HTTPStd_FIELD_NAME_USER_AGENT;
extern const char *HTTPStd_FIELD_NAME_VARY;
extern const char *HTTPStd_FIELD_NAME_VIA;
extern const char *HTTPStd_FIELD_NAME_WWW_AUTHENTICATE;
extern const char *HTTPStd_FIELD_NAME_WARNING;
extern const char *HTTPStd_FIELD_NAME_X_FORWARDED_FOR;

/*! @endcond */
/*! @} */
#ifdef __cplusplus
}
#endif
#endif
