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
#ifndef ti_net_http_HTTPCli__include
#define ti_net_http_HTTPCli__include

/*!
 *  @file ti/net/http/httpcli.h
 *
 *  @addtogroup ti_net_http_HTTPCli HTTP Client
 *
 *  @brief  HTTP/1.1 Client API
 *
 *  This module provides an HTTP client implementation of IETF standard for
 *  HTTP/1.1 - RFC 2616 and TLS support to interact with HTTP/1.1 servers.
 *
 *  ### Features
 *    - Supports GET, POST, PUT, HEAD, OPTIONS, DELETE methods, request and
 *  response headers/bodies, and redirection response handling.
 *
 *    - Using this module, connections can be opened/closed to HTTP servers.
 *  Each connection is an instance which can be configured with a set of
 *  repeated requests/responses parameters so that they can be used across
 *  multiple transactions.
 *
 *    - For supporting small memory devices, the module has been specially
 *  designed with split request/response APIs which enables sending requests and
 *  responses in smaller chunks, and static memory based APIs to eliminate the
 *  need for dynamic memory allocations.
 *
 *    - Supports security features which include TLS and communication
 *  through proxy.
 *
 *    - Supports two types of programming models: synchronous and
 *  asynchronous modes. In the default synchronous mode, an application uses
 *  APIs to make a request to the server and blocks until the server responds.
 *  With asynchronous mode, a thread to handle the response is created
 *  which invokes callbacks registered with the module.
 *
 *    - Supports TI-RTOS NDK, SimpleLink WiFi and Linux networking stacks. At
 *  IP level supports both IPv4 and IPv6.
 *
 *    - Supports conditional compilation of the module to include/exclude
 *  some of the features mentioned above.
 *
 *  ### Limitations
 *    - HTTP client on SimpleLink WiFi supports IPv4 only.
 *    - HTTP servers sending response header field name > 24 bytes are skipped.
 *  This length is controlled by the 'MAX_FIELD_NAME_LEN' macro in 'httpcli.c'.
 *  Modify the macro and rebuild the library if needed.
 *
 *  ### Deprecated Macros
 *  The macros HTTPCli_METHOD* and HTTPCli_FIELD_NAME* have been deprecated.
 *  Use HTTPStd_* macros.
 *
 *  For backward compatibility, the old macros can still be used if
 *  -Dti_net_http_HTTPCli__deprecated is added to the compile options.
 *
 *  ### Minimal HTTP Client library
 *  A minimal flavor of the library without asynchronous callback support which
 *  includes status handling, content handling and redirection handling can
 *  be built. The default library has support for these features.
 *
 *  To rebuild the library, add "-DHTTPCli_LIBTYPE_MIN" to the compile line
 *  and rebuild. The same option has to be added on application compile line
 *  also.
 *
 *  ### See also
 *    - @ref ti_net_TLS "TLS" for TLS APIs.
 *
 *  ### Examples
 *  The basic flow of managing an HTTP client is shown in the pseudo codes
 *  below. For clarity, return values are ignored - in a real application,
 *  these should be checked for errors.
 *
 *  - HTTP GET Example
 *  @code
 *  #include <ti/net/http/httpcli.h>
 *
 *  HTTPCli_Struct cli;
 *
 *  // Request fields
 *  HTTPCli_Field fields[2] = {
 *      { HTTPStd_FIELD_NAME_HOST,  "www.example.com" },
 *      { NULL, NULL }
 *  };
 *
 *  // Response field filters
 *  char respFields[2] = {
 *      HTTPStd_FIELD_NAME_CONTENT_LENGTH,
 *      NULL
 *  };
 *
 *  // Construct a static HTTP client instance
 *  HTTPCli_construct(&cli);
 *
 *  HTTPCli_setRequestFields(&cli, fields);
 *
 *  HTTPCli_setResponseFields(&cli, respFields);
 *
 *  // Connect to the HTTP Server
 *  HTTPCli_connect(&cli, &addr, 0, NULL);
 *
 *  // Make HTTP 1.1 GET request
 *  //
 *  // Send request to the server:
 *  //
 *  // GET /index.html HTTP/1.1
 *  // Host: www.example.com
 *  // <blank line>
 *  HTTPCli_sendRequest(&cli, HTTPStd_GET, "/index.html", false);
 *
 *  //
 *  // Get the processed response status
 *  //
 *  // HTTP/1.1 200 OK
 *  status = HTTPCli_getResponseStatus(&cli);
 *
 *  // Check the HTTP return status and process remaining response
 *  if (status == HTTPStd_OK) {
 *      do {
 *          //Filter the response headers and get the set response field
 *          //
 *          // ...
 *          // Content-type: text/xml; charset=utf-8\r\n
 *          // Content-length: 34
 *          // ...
 *          ret = HTTPCli_getResponseField(&cli, buf, sizeof(buf), &moreFlag);
 *
 *          //
 *          // Process data in buf if field is content length
 *          //
 *          // Zero is the index of Content length in respFields array
 *          if (ret == 0) {
 *              len = (int)strtoul(buf, NULL, 0);
 *          }
 *      } while (ret != HTTPCli_FIELD_ID_END);
 *
 *      while (len > 0) {
 *          len -= HTTPCli_readRawResponseBody(&cli, buf, sizeof(buf));
 *          // ... process buf data and save ...
 *      }
 *  }
 *
 *  HTTPCli_disconnect(&cli);
 *
 *  HTTPCli_destruct(&cli);
 *  @endcode
 *
 *  - HTTPS POST Example
 *  @code
 *  #include <ti/net/tls.h>
 *  #include <ti/net/http/httpcli.h>
 *
 *  TLS_Handle tls;
 *
 *  HTTPCli_Struct cli;
 *  HTTPCli_Params params;
 *
 *  tls = TLS_create(method);
 *  TLS_setCertFile(tls, TLS_CERT_TYPE_CA, TLS_CERT_FORMAT_DER, "./ca.der");
 *
 *  // Request fields
 *  HTTPCli_Field fields[2] = {
 *      { HTTPStd_FIELD_NAME_HOST,  "www.example.com" },
 *      { NULL, NULL }
 *  };
 *
 *  // Response field filters
 *  char respFields[2] = {
 *      HTTPStd_FIELD_NAME_CONTENT_LENGTH,
 *      NULL
 *  };
 *
 *  // Construct a static HTTP client instance
 *  HTTPCli_construct(&cli);
 *
 *  HTTPCli_setRequestFields(&cli, fields);
 *
 *  HTTPCli_setResponseFields(&cli, respFields);
 *
 *  //Set the TLS context
 *  HTTPCli_Params_init(&params);
 *  params.tls = tls;
 *
 *  // Connect securely to the HTTPS Server
 *  HTTPCli_connect(&cli, &addr, 0, &params);
 *
 *  // Make HTTP 1.1 POST request
 *  //
 *  // Send request to the server:
 *  //
 *  // POST /index.html HTTP/1.1
 *  // Host: www.example.com
 *  HTTPCli_sendRequest(&cli, HTTPStd_POST, "/index.html", true);
 *
 *  // Send additional fields
 *  //
 *  // Content-Length: <length>
 *  // <blank line>
 *  HTTPCli_sendField(&cli, HTTPStd_FIELD_NAME_CONTENT_LENGTH, len, true);
 *
 *  // Send request body
 *  //
 *  // <data>
 *  HTTPCli_sendRequestBody(&cli, data, strlen(data));
 *
 *  // Get the processed response status
 *  //
 *  // HTTP/1.1 200 OK
 *  status = HTTPCli_getResponseStatus(&cli);
 *
 *  // Check the HTTP return status and process remaining response
 *  if (status == HTTPStd_OK) {
 *      do {
 *          // Filter the response headers and get the set response field
 *          //
 *          //...
 *          // Content-type: text/xml; charset=utf-8\r\n
 *          // Content-length: 34
 *          //  ...
 *          ret = HTTPCli_getResponseField(&cli, buf, sizeof(buf), &moreFlag);
 *
 *          //  Process data in buf if field is content length
 *          //  Zero is the index of Content length in respFields array
 *          if (ret == 0) {
 *              len = (int)strtoul(buf, NULL, 0);
 *          }
 *
 *      } while (ret != HTTPCli_FIELD_ID_END);
 *
 *      while (len > 0) {
 *          len -= HTTPCli_readRawResponseBody(&cli, buf, sizeof(buf));
 *          // ... process buf data and save ...
 *      }
 *   }
 *
 *   HTTPCli_disconnect(&cli);
 *
 *   HTTPCli_destruct(&cli);
 *   @endcode
 */

/*! @ingroup ti_net_http_HTTPCli */
/*@{*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ti/net/ssock.h>
#include <ti/net/tls.h>
#include <ti/net/http/httpstd.h>

/*!
 *  Modify the HTTP client object's internal buffer length and rebuild the
 *  library if needed.
 */
#ifndef HTTPCli_BUF_LEN
#define HTTPCli_BUF_LEN 128
#endif

/*! Minimum size required to hold content length string */
#define HTTPCli_RECV_BUFLEN 32

/*!
 *  @name HTTP Client Error Codes
 *  @{
 */
/*!
 *  @brief Socket create failed
 *
 *  @sa @ref HTTPCli_getSocketError()
 */
#define HTTPCli_ESOCKETFAIL        (-101)

/*!
 *  @brief Cannot connect to the remote host
 *
 *  @sa @ref HTTPCli_getSocketError()
 */
#define HTTPCli_ECONNECTFAIL       (-102)

/*!
 *  @brief Cannot send to the remote host
 *
 *  @sa @ref HTTPCli_getSocketError()
 */
#define HTTPCli_ESENDFAIL          (-103)

/*!
 *  @brief Cannot recieve data from the remote host
 *
 *  @sa @ref HTTPCli_getSocketError()
 */
#define HTTPCli_ERECVFAIL          (-104)

/*!
 *  @brief Failed to configure the socket with TLS parameters
 *
 *  @sa @ref HTTPCli_getSocketError()
 */
#define HTTPCli_ETLSFAIL           (-105)

/*!
 *  @brief Failed to resolve host name
 */
#define HTTPCli_EHOSTNAME          (-106)

/*!
 *  @brief Internal send buffer is not big enough
 *
 *  Modify the SEND_BUFLEN macro in the httpcli.c and rebuild the library if
 *  needed.
 */
#define HTTPCli_ESENDBUFSMALL      (-107)

/*!
 *  @brief Receive buffer is not big enough
 *
 *  The length of buffer parameter in @ref HTTPCli_getResponseField() should
 *  be at least @ref HTTPCli_RECV_BUFLEN.
 */
#define HTTPCli_ERECVBUFSMALL      (-108)

/*!
 *  @brief @ref HTTPCli_getResponseStatus() should not be called in async mode
 */
#define HTTPCli_EASYNCMODE         (-109)

/*!
 *  @brief Thread create failed
 */
#define HTTPCli_ETHREADFAIL        (-110)

/*!
 *  @brief Failed to create an HTTP tunnel through proxy
 */
#define HTTPCli_EPROXYTUNNELFAIL   (-111)

/*!
 *  @brief Response recieved from the server is not a valid HTTP/1.1 response
 */
#define HTTPCli_ERESPONSEINVALID   (-112)

/*!
 *  @brief Content length returned is too large
 *
 *  The length of buffer parameter in @ref HTTPCli_getResponseField() is not
 *  sufficient
 */
#define HTTPCli_ECONTENTLENLARGE   (-114)

/*!
 *  @brief Redirection URI returned is too long to be read into the buffer
 *
 *  Modify the URI_BUFLEN macro in the httpcli.c and rebuild the library if
 *  needed.
 */
#define HTTPCli_EREDIRECTURILONG   (-115)

/*!
 *  @brief Content type returned is too long to be read into buffer
 */
#define HTTPCli_ECONTENTTYPELONG   (-116)

/*!
 *  @brief Received content type does not match any registered callback
 *
 *  Modify the CONTENT_BUFLEN macro in the httpcli.c and rebuild the library if
 *  needed.
 */
#define HTTPCli_ENOCONTENTCALLBACK (-117)

/*!
 *  @brief Data is not of chunked type
 */
#define HTTPCli_ENOTCHUNKDATA      (-118)

/*!
 *  @brief Operation could not be completed. Try again.
 */
#define HTTPCli_EINPROGRESS        (-119)

/*!
 *  @brief Internal instance buffer to send/recieve data is too small.
 */
#define HTTPCli_EINTERNALBUFSMALL  (-120)

/*!
 *  @brief Could not setup the notify callbacks
 */
#define HTTPCli_ESETNOTIFYFAIL     (-121)

/*!
 *  @brief Input domain name length is too long to be read into buffer.
 *
 *  Modify the DOMAIN_BUFLEN macro in the httpcli.c and rebuild the library if
 *  needed.
 */
#define HTTPCli_EURILENLONG        (-122)

/*!
 *  @brief Request Field 'Host' is not found in the set array.
 *
 *  'Host' should be set in the 'fields' parameter of @ref
 *  HTTPCli_setRequestFields(). This is used to set up an HTTP tunnel through
 *  proxy servers.
 */
#define HTTPCli_EHOSTFIELDNOTFOUND (-123)
/*! @} */

#ifdef ti_net_http_HTTPCli__deprecated
/*!
 * @cond DONT_INCLUDE
 */
/* Standard ports */
#define HTTP_PORT HTTPStd_PORT
#define HTTPS_PORT HTTPStd_SECURE_PORT

/* HTTP Status Codes */
#define HTTP_CONTINUE                      HTTPStd_CONTINUE
#define HTTP_SWITCHING_PROTOCOLS           HTTPStd_SWITCHING_PROTOCOLS
#define HTTP_OK                            HTTPStd_OK
#define HTTP_CREATED                       HTTPStd_CREATED
#define HTTP_ACCEPTED                      HTTPStd_ACCEPTED
#define HTTP_NON_AUTHORITATIVE_INFORMATION HTTPStd_NON_AUTHORITATIVE_INFORMATION
#define HTTP_NO_CONTENT                    HTTPStd_NO_CONTENT
#define HTTP_RESET_CONTENT                 HTTPStd_RESET_CONTENT
#define HTTP_PARTIAL_CONTENT               HTTPStd_PARTIAL_CONTENT
#define HTTP_MULTIPLE_CHOICES              HTTPStd_MULTIPLE_CHOICES
#define HTTP_MOVED_PERMANENTLY             HTTPStd_MOVED_PERMANENTLY
#define HTTP_FOUND                         HTTPStd_FOUND
#define HTTP_SEE_OTHER                     HTTPStd_SEE_OTHER
#define HTTP_NOT_MODIFIED                  HTTPStd_NOT_MODIFIED
#define HTTP_USE_PROXY                     HTTPStd_USE_PROXY
#define HTTP_TEMPORARY_REDIRECT            HTTPStd_TEMPORARY_REDIRECT
#define HTTP_BAD_REQUEST                   HTTPStd_BAD_REQUEST
#define HTTP_UNAUTHORIZED                  HTTPStd_UNAUTHORIZED
#define HTTP_PAYMENT_REQUIRED              HTTPStd_PAYMENT_REQUIRED
#define HTTP_FORBIDDEN                     HTTPStd_FORBIDDEN
#define HTTP_NOT_FOUND                     HTTPStd_NOT_FOUND
#define HTTP_METHOD_NOT_ALLOWED            HTTPStd_METHOD_NOT_ALLOWED
#define HTTP_NOT_ACCEPTABLE                HTTPStd_NOT_ACCEPTABLE
#define HTTP_PROXY_AUTHENTICATION_REQUIRED HTTPStd_PROXY_AUTHENTICATION_REQUIRED
#define HTTP_REQUEST_TIMEOUT               HTTPStd_REQUEST_TIMEOUT
#define HTTP_CONFLICT                      HTTPStd_CONFLICT
#define HTTP_GONE                          HTTPStd_GONE
#define HTTP_LENGTH_REQUIRED               HTTPStd_LENGTH_REQUIRED
#define HTTP_PRECONDITION_FAILED           HTTPStd_PRECONDITION_FAILED
#define HTTP_REQUEST_ENTITY_TOO_LARGE      HTTPStd_REQUEST_ENTITY_TOO_LARGE
#define HTTP_REQUEST_URI_TOO_LONG          HTTPStd_REQUEST_URI_TOO_LONG
#define HTTP_UNSUPPORTED_MEDIA_TYPE        HTTPStd_UNSUPPORTED_MEDIA_TYPE
#define HTTP_REQUESTED_RANGE_NOT_SATISFAIABLE \
        HTTPStd_REQUESTED_RANGE_NOT_SATISFAIABLE
#define HTTP_EXPECTATION_FAILED            HTTPStd_EXPECTATION_FAILED
#define HTTP_INTERNAL_SERVER_ERROR         HTTPStd_INTERNAL_SERVER_ERROR
#define HTTP_NOT_IMPLEMENTED               HTTPStd_NOT_IMPLEMENTED
#define HTTP_BAD_GATEWAY                   HTTPStd_BAD_GATEWAY
#define HTTP_SERVICE_UNAVAILABLE           HTTPStd_SERVICE_UNAVAILABLE
#define HTTP_GATEWAY_TIMEOUT               HTTPStd_GATEWAY_TIMEOUT
#define HTTP_HTTP_VERSION_NOT_SUPPORTED    HTTPStd_HTTP_VERSION_NOT_SUPPORTED

/* HTTP methods */
#define HTTPCli_METHOD_GET      HTTPStd_GET
#define HTTPCli_METHOD_POST     HTTPStd_POST
#define HTTPCli_METHOD_HEAD     HTTPStd_HEAD
#define HTTPCli_METHOD_OPTIONS  HTTPStd_OPTIONS
#define HTTPCli_METHOD_PUT      HTTPStd_PUT
#define HTTPCli_METHOD_DELETE   HTTPStd_DELETE
#define HTTPCli_METHOD_CONNECT  HTTPStd_CONNECT

/* HTTP Request Field Name */
#define HTTPCli_FIELD_NAME_ACCEPT  HTTPStd_FIELD_NAME_ACCEPT
#define HTTPCli_FIELD_NAME_ACCEPT_CHARSET  HTTPStd_FIELD_NAME_ACCEPT_CHARSET
#define HTTPCli_FIELD_NAME_ACCEPT_ENCODING  HTTPStd_FIELD_NAME_ACCEPT_ENCODING
#define HTTPCli_FIELD_NAME_ACCEPT_LANGUAGE  HTTPStd_FIELD_NAME_ACCEPT_LANGUAGE
#define HTTPCli_FIELD_NAME_ACCEPT_RANGES  HTTPStd_FIELD_NAME_ACCEPT_RANGES
#define HTTPCli_FIELD_NAME_AGE  HTTPStd_FIELD_NAME_AGE
#define HTTPCli_FIELD_NAME_ALLOW  HTTPStd_FIELD_NAME_ALLOW
#define HTTPCli_FIELD_NAME_AUTHORIZATION  HTTPStd_FIELD_NAME_AUTHORIZATION
#define HTTPCli_FIELD_NAME_CACHE_CONTROL  HTTPStd_FIELD_NAME_CACHE_CONTROL
#define HTTPCli_FIELD_NAME_CONNECTION  HTTPStd_FIELD_NAME_CONNECTION
#define HTTPCli_FIELD_NAME_CONTENT_ENCODING  HTTPStd_FIELD_NAME_CONTENT_ENCODING
#define HTTPCli_FIELD_NAME_CONTENT_LANGUAGE  HTTPStd_FIELD_NAME_CONTENT_LANGUAGE
#define HTTPCli_FIELD_NAME_CONTENT_LENGTH  HTTPStd_FIELD_NAME_CONTENT_LENGTH
#define HTTPCli_FIELD_NAME_CONTENT_LOCATION  HTTPStd_FIELD_NAME_CONTENT_LOCATION
#define HTTPCli_FIELD_NAME_CONTENT_MD5  HTTPStd_FIELD_NAME_CONTENT_MD5
#define HTTPCli_FIELD_NAME_CONTENT_RANGE  HTTPStd_FIELD_NAME_CONTENT_RANGE
#define HTTPCli_FIELD_NAME_CONTENT_TYPE  HTTPStd_FIELD_NAME_CONTENT_TYPE
#define HTTPCli_FIELD_NAME_COOKIE  HTTPStd_FIELD_NAME_COOKIE
#define HTTPCli_FIELD_NAME_DATE  HTTPStd_FIELD_NAME_DATE
#define HTTPCli_FIELD_NAME_ETAG  HTTPStd_FIELD_NAME_ETAG
#define HTTPCli_FIELD_NAME_EXPECT  HTTPStd_FIELD_NAME_EXPECT
#define HTTPCli_FIELD_NAME_EXPIRES  HTTPStd_FIELD_NAME_EXPIRES
#define HTTPCli_FIELD_NAME_FROM  HTTPStd_FIELD_NAME_FROM
#define HTTPCli_FIELD_NAME_HOST  HTTPStd_FIELD_NAME_HOST
#define HTTPCli_FIELD_NAME_IF_MATCH  HTTPStd_FIELD_NAME_IF_MATCH
#define HTTPCli_FIELD_NAME_IF_MODIFIED_SINCE  \
        HTTPStd_FIELD_NAME_IF_MODIFIED_SINCE
#define HTTPCli_FIELD_NAME_IF_NONE_MATCH  HTTPStd_FIELD_NAME_IF_NONE_MATCH
#define HTTPCli_FIELD_NAME_IF_RANGE  HTTPStd_FIELD_NAME_IF_RANGE
#define HTTPCli_FIELD_NAME_IF_UNMODIFIED_SINCE  \
        HTTPStd_FIELD_NAME_IF_UNMODIFIED_SINCE
#define HTTPCli_FIELD_NAME_LAST_MODIFIED  HTTPStd_FIELD_NAME_LAST_MODIFIED
#define HTTPCli_FIELD_NAME_LOCATION  HTTPStd_FIELD_NAME_LOCATION
#define HTTPCli_FIELD_NAME_MAX_FORWARDS  HTTPStd_FIELD_NAME_MAX_FORWARDS
#define HTTPCli_FIELD_NAME_ORIGIN  HTTPStd_FIELD_NAME_ORIGIN
#define HTTPCli_FIELD_NAME_PRAGMA  HTTPStd_FIELD_NAME_PRAGMA
#define HTTPCli_FIELD_NAME_PROXY_AUTHENTICATE  \
        HTTPStd_FIELD_NAME_PROXY_AUTHENTICATE
#define HTTPCli_FIELD_NAME_PROXY_AUTHORIZATION  \
        HTTPStd_FIELD_NAME_PROXY_AUTHORIZATION
#define HTTPCli_FIELD_NAME_RANGE  HTTPStd_FIELD_NAME_RANGE
#define HTTPCli_FIELD_NAME_REFERER  HTTPStd_FIELD_NAME_REFERER
#define HTTPCli_FIELD_NAME_RETRY_AFTER  HTTPStd_FIELD_NAME_RETRY_AFTER
#define HTTPCli_FIELD_NAME_SERVER  HTTPStd_FIELD_NAME_SERVER
#define HTTPCli_FIELD_NAME_TE  HTTPStd_FIELD_NAME_TE
#define HTTPCli_FIELD_NAME_TRAILER  HTTPStd_FIELD_NAME_TRAILER
#define HTTPCli_FIELD_NAME_TRANSFER_ENCODING  \
        HTTPStd_FIELD_NAME_TRANSFER_ENCODING
#define HTTPCli_FIELD_NAME_UPGRADE  HTTPStd_FIELD_NAME_UPGRADE
#define HTTPCli_FIELD_NAME_USER_AGENT  HTTPStd_FIELD_NAME_USER_AGENT
#define HTTPCli_FIELD_NAME_VARY  HTTPStd_FIELD_NAME_VARY
#define HTTPCli_FIELD_NAME_VIA  HTTPStd_FIELD_NAME_VIA
#define HTTPCli_FIELD_NAME_WWW_AUTHENTICATE  HTTPStd_FIELD_NAME_WWW_AUTHENTICATE
#define HTTPCli_FIELD_NAME_WARNING  HTTPStd_FIELD_NAME_WARNING
#define HTTPCli_FIELD_NAME_X_FORWARDED_FOR  HTTPStd_FIELD_NAME_X_FORWARDED_FOR
/*! @endcond */
#endif

/*!
 * @name HTTP Client Configuration Types
 * @{
 */
#define HTTPCli_TYPE_TLS   (0x02)
#define HTTPCli_TYPE_IPV6  (0x04)
/*! @} */

/*!
 * @name HTTP Client Field ID
 * Special return codes for HTTPCli_getResponseField().
 * @{
 */
#define HTTPCli_FIELD_ID_DUMMY   (-11)
#define HTTPCli_FIELD_ID_END     (-12)
/*! @} */

/*!
 *  @brief HTTPCli request header field
 */
typedef struct HTTPCli_Field {
    const char *name;      /*!< Field name,  ex: "Accept" */
    const char *value;     /*!< Field value, ex: "text/plain" */
} HTTPCli_Field;

/*!
 *  @brief HTTPCli callback function for status handling
 *
 *  @param[in]  cli     Instance of the HTTP connection
 *
 *  @param[in]  status  Response status code
 */
typedef void (*HTTPCli_StatusCallback)(void *cli, int status);

/*!
 *  @brief HTTPCli response status code handler type
 */
typedef struct HTTPCli_StatusHandler {
    HTTPCli_StatusCallback  handle1xx;  /*!< 1xx status code callback */
    HTTPCli_StatusCallback  handle2xx;  /*!< 2xx status code callback */
    HTTPCli_StatusCallback  handle4xx;  /*!< 4xx/5xx status code callback */
} HTTPCli_StatusHandler;

/*!
 *  @brief HTTPCli callback function for content handling
 *
 *  @param[in]  cli      Instance of the HTTP connection
 *
 *  @param[in]  status   Response status code
 *
 *  @param[in]  body     Data from the response body
 *
 *  @param[in]  len      Length of response body buffer
 *
 *  @param[in]  moreFlag Set if more response data is available
 *
 *  @return 1 to continue or 0 to stop further processing.
 */
typedef int (*HTTPCli_ContentCallback)(void *cli, int status, char *body,
        int len, bool moreFlag);

/*!
 *  @brief HTTPCli content handler type
 */
typedef struct HTTPCli_ContentHandler {
    char *contentType;               /*!< ex: application/json */
    HTTPCli_ContentCallback handle;  /*!< Callback for content Type */
} HTTPCli_ContentHandler;

/*!
 *  @brief HTTPCli callback function prototype for redirection handling
 *
 *  @param[in]  cli     Instance of the HTTP connection
 *
 *  @param[in]  status  Response status code
 *
 *  @param[in]  uri     The new URI string
 */
typedef void (*HTTPCli_RedirectCallback)(void *cli, int status, char *uri);

/*!
 *  @cond DONT_INCLUDE
 *
 *  @brief HTTPCli callback function prototype for asynchronous notify
 *         Supported for 6LoWPAN stack only.
 *
 *  @param[in]  skt  socket handle
 *
 *  @param[in]  cli  Instance of the HTTP connection
 */
typedef void (*HTTPCli_Notify)(long skt, void *cli);
/*! @endcond */

/*!
 *  @brief HTTPCli instance type
 *
 *  @remarks This doxygen does not document all data fields. See
 *           <a href="httpcli_8h_source.html"><b>httpcli.h</b></a> for complete
 *           details.
 */
typedef struct HTTPCli_Struct {
    char **respFields;
    int sockerr;
    unsigned int state;
    unsigned long clen;
    Ssock_Struct ssock;
    HTTPCli_Field *fields;

    char buf[HTTPCli_BUF_LEN]; /*!<see @ref HTTPCli_BUF_LEN */
    unsigned int buflen;
    char *bufptr;

    TLS_Handle tls;
#ifndef HTTPCli_LIBTYPE_MIN
    HTTPCli_StatusHandler *shandle;
    HTTPCli_ContentHandler *chandle;
    HTTPCli_RedirectCallback rhandle;
#ifndef __linux__
    unsigned int stackSize;
    unsigned int priority;
#endif
#endif

} HTTPCli_Struct;

/*!
 *  @brief HTTPCli instance paramaters
 *
 *  @remarks This doxygen does not document all data fields. See
 *           <a href="httpcli_8h_source.html"><b>httpcli.h</b></a> for complete
 *           details.
 */
typedef struct HTTPCli_Params {

    TLS_Handle tls;
#ifndef HTTPCli_LIBTYPE_MIN
    HTTPCli_StatusHandler *shandle;
    HTTPCli_ContentHandler *chandle;
    HTTPCli_RedirectCallback rhandle;
#ifndef __linux__
    unsigned int stackSize; /*!< Async thread stack size. 0 for default */
    unsigned int priority;  /*!< Async thread priority. 0 for default */
#endif
#endif

#ifdef NET_SLP
    HTTPCli_Notify rnotify;   /*!< Async read notify handle (6LoWPAN) */
    HTTPCli_Notify wnotify;   /*!< Async write notify handle (6LoWPAN) */
    HTTPCli_Notify enotify;   /*!< Async exception notify handle (6LowPAN)*/
#endif

#ifndef NET_SLP
    int timeout;
    /*!< Timeout value (in seconds) for socket. Set 0 for default value */
#endif

} HTTPCli_Params;

typedef HTTPCli_Struct* HTTPCli_Handle;

/*!
 *  @brief  Initialize the HTTPCli Params structure to default values
 *
 *  @param[in]  params A pointer to the HTTPCli_Params struct
 */
extern void HTTPCli_Params_init(HTTPCli_Params *params);

/*!
 *  @brief  Initialize the socket address structure for the given URI
 *
 *  The supported URI format is:
 *     - [http[s]://]host_name[:port_number][/request_uri]
 *
 *  For cases where port is not provided, the default port number is set.
 *
 *  @param[out] addr   Handle to the sockaddr structure
 *
 *  @param[in]  uri    A null terminated URI string
 *
 *  @param[in]  flags  Set @ref HTTPCli_TYPE_IPV6 for IPv6 addresses.
 *
 *  @remarks SimpleLink WiFi stack does not support IPv6.
 *
 *  @return 0 on success or error code on failure.
 */
extern int HTTPCli_initSockAddr(struct sockaddr *addr, const char *uri,
        int flags);

/*!
 *  @brief  Create a new instance object in the provided structure
 *
 *  @param[out] cli    Instance of an HTTP client
 */
extern void HTTPCli_construct(HTTPCli_Struct *cli);

/*!
 *  @brief  Allocate and initialize a new instance object and return its handle
 *
 *  @return handle of the HTTP client instance on success or NULL on failure.
 */
extern HTTPCli_Handle HTTPCli_create();

/*!
 *  @brief  Open a connection to an HTTP server
 *
 *  @param[in]  cli    Instance of an HTTP client
 *
 *  @param[in]  addr   IP address of the server. Can be NULL when proxy is set.
 *
 *  @param[in]  flags  Reserved for future use
 *
 *  @param[in]  params Per-instance config params, or NULL for default values
 *
 *  @return 0 on success or error code on failure.
 */
extern int HTTPCli_connect(HTTPCli_Handle cli, const struct sockaddr *addr,
        int flags, const HTTPCli_Params *params);

/*!
 *  @brief  Destroy the HTTP client instance and free the previously allocated
 *          instance object.
 *
 *  @param[in]  cli Pointer to the HTTP client instance
 */
extern void HTTPCli_delete(HTTPCli_Handle *cli);

/*!
 *  @brief  Destroy the HTTP client instance
 *
 *  @param[in]  cli Instance of the HTTP client
 */
extern void HTTPCli_destruct(HTTPCli_Struct *cli);

/*!
 *  @brief  Disconnect from the HTTP server and destroy the HTTP client instance
 *
 *  @param[in]  cli Instance of the HTTP client
 */
extern void HTTPCli_disconnect(HTTPCli_Handle cli);

/*!
 *  @brief  Set an array of header fields to be sent for every HTTP request
 *
 *  @param[in]  cli     Instance of an HTTP client
 *
 *  @param[in]  fields  An array of HTTP request header fields terminated by
 *                      an object with NULL fields, or NULL to get
 *                      previously set array.
 *
 *  @remarks The array should be persistant for lifetime of the HTTP instance.
 *
 *  @return previously set array
 */
extern HTTPCli_Field *HTTPCli_setRequestFields(HTTPCli_Handle cli,
        const HTTPCli_Field *fields);

/*!
 *  @brief  Set the header fields to filter the response headers
 *
 *  @param[in]  cli      Instance of an HTTP client
 *
 *  @param[in]  fields   An array of HTTP response header field strings
 *                       terminated by a NULL, or NULL to get previously set
 *                       array.
 *
 *  @remarks The array should be persistant for lifetime of the HTTP instance.
 *
 *  @return previously set array
 */
extern char **HTTPCli_setResponseFields(HTTPCli_Handle cli,
        const char *fields[]);

/*!
 *  @brief  Make an HTTP 1.1 request to the HTTP server
 *
 *  Sends an HTTP 1.1 request-line and the header fields from the user set array
 *  (see @ref HTTPCli_setRequestFields()) to the server.
 *
 *  Additionally, more fields apart from the user set array of header fields
 *  can be sent to the server. To send more fields, set the `moreFlag` when
 *  calling this function and then call @ref HTTPCli_sendField() with more
 *  fields.
 *
 *  @param[in]  cli        Instance of an HTTP client
 *
 *  @param[in]  method     HTTP 1.1 method (ex: @ref HTTPStd_GET)
 *
 *  @param[in]  requestURI the path on the server to open and any CGI
 *                         parameters
 *
 *  @param[in]  moreFlag   Set this flag when more fields will sent to the
 *                         server
 *
 *  @return 0 on success or error code on failure
 */
extern int HTTPCli_sendRequest(HTTPCli_Handle cli, const char *method,
        const char *requestURI, bool moreFlag);

/*!
 *  @brief  Send an header field to the HTTP server
 *
 *  This is a complementary function to @ref HTTPCli_sendRequest() when more
 *  header fields are to be sent to the server.
 *
 *  @param[in]  cli      Instance of an HTTP client
 *
 *  @param[in]  name     HTTP 1.1 request header field
 *                       (ex: @ref HTTPStd_FIELD_NAME_HOST)
 *
 *  @param[in]  value    HTTP 1.1 request header field value
 *
 *  @param[in]  lastFlag Set this flag when sending the last header field
 *
 *  @return 0 on success or error code on failure
 */
extern int HTTPCli_sendField(HTTPCli_Handle cli, const char *name,
        const char *value, bool lastFlag);

/*!
 *  @brief  Send the request message body to the HTTP server
 *
 *  Make a call to this function after @ref HTTPCli_sendRequest() (always)
 *  and @ref HTTPCli_sendField() (if applicable).
 *
 *  @param[in]  cli   Instance of an HTTP client
 *
 *  @param[in]  body  Request body buffer
 *
 *  @param[in]  len   Length of the request body buffer
 *
 *  @return 0 on success or error code on failure
 */
extern int HTTPCli_sendRequestBody(HTTPCli_Handle cli, const char *body,
        int len);

/*!
 *  @brief  Process the response header from the HTTP server and return status
 *
 *  @remarks  Do not call in asyncronous mode. This function will return
 *            @ref HTTPCli_EASYNCMODE.
 *
 *  @param[in]  cli    Instance of an HTTP client
 *
 *  @return  On success, @ref HTTPStd_STATUS_CODE "status code" from the server
 *           or error code on failure.
 */
extern int HTTPCli_getResponseStatus(HTTPCli_Handle cli);

/*!
 *  @brief  Process a response header from the HTTP server and return field
 *
 *  Filters the response headers based on the array of fields (see
 *  @ref HTTPCli_setResponseFields()).
 *
 *  Repeatedly call this function until @ref HTTPCli_FIELD_ID_END is returned.
 *
 *  @param[in]  cli       Instance of an HTTP client
 *
 *  @param[out] value     Field value string.
 *
 *  @param[in]  len       Length of field value string
 *
 *  @param[out] moreFlag  Flag set if the field value could not be completely
 *                        read into `value`. A subsequent call to this function
 *                        will read the remaining field value into `value` and
 *                        will return @ref HTTPCli_FIELD_ID_DUMMY.
 *
 *  @return On Success, the index of the field set in the
 *          @ref HTTPCli_setResponseFields() or @ref HTTPCli_FIELD_ID_END or
 *          @ref HTTPCli_FIELD_ID_DUMMY, or error code on failure.
 */
extern int HTTPCli_getResponseField(HTTPCli_Handle cli, char *value,
        int len, bool *moreFlag);

/*!
 *  @brief  Read a response header from the HTTP server
 *
 *  Repeatedly call this function until zero is returned.
 *
 *  @param[in]  cli       Instance of an HTTP client
 *
 *  @param[out] header    Buffer to hold Response Header string
 *
 *  @param[in]  len       Length of the buffer
 *
 *  @param[out] moreFlag  Flag set if the field value could not be completely
 *                        read into `header`. A subsequent call to this function
 *                        will read the remaining field value into `header`
 *
 *  @return The number of characters read (including NULL character) on
 *          success or error code on failure
 */
extern int HTTPCli_readResponseHeader(HTTPCli_Handle cli, char *header,
        int len, bool *moreFlag);

/*!
 *  @brief  Read the parsed response body data from the HTTP server
 *
 *  This function parses the response body if the content type is chunked
 *  transfer encoding or if the content length field is returned by the HTTP
 *  server.
 *
 *  Make a call to this function only after the call to
 *  @ref HTTPCli_getResponseStatus() and @ref HTTPCli_getResponseField().
 *
 *  @param[in]  cli       Instance of an HTTP client
 *
 *  @param[out] body      Response body buffer
 *
 *  @param[in]  len       Length of response body buffer
 *
 *  @param[out] moreFlag  Set if more data is available
 *
 *  @return The number of characters read on success or error code on failure
 */
extern int HTTPCli_readResponseBody(HTTPCli_Handle cli, char *body,
        int len, bool *moreFlag);

/*!
 *  @brief  Read the raw response message body from the HTTP server
 *
 *  Make a call to this function only after the call to
 *  @ref HTTPCli_getResponseStatus() and @ref HTTPCli_getResponseField().
 *
 *  Repeatedly call this function until entire response message is read.
 *
 *  @param[in]  cli   Instance of an HTTP client
 *
 *  @param[out] body  Response body buffer
 *
 *  @param[in]  len   Length of response body buffer
 *
 *  @return The number of characters read on success or error code on failure
 */
extern int HTTPCli_readRawResponseBody(HTTPCli_Handle cli, char *body, int len);

/*!
 *  @brief Set the proxy address
 *
 *  @param[in]  addr IP address of the proxy server
 */
extern void HTTPCli_setProxy(const struct sockaddr *addr);

/*!
 *  @brief Get the error code from the socket/TLS layer
 *
 *  If the returned error from the HTTPCli APIs is either a socket failure code
 *  (@ref HTTPCli_ESOCKETFAIL or @ref HTTPCli_ECONNECTFAIL or @ref
 *  HTTPCli_ESENDFAIL or @ref HTTPCli_ERECVFAIL) or a
 *  TLS failure code (@ref HTTPCli_ETLSFAIL), a call to this function may
 *  provide the socket/TLS layer error code.
 *
 *  @param[in]  cli   Instance of an HTTP client
 *
 *  @return The error code from the socket/TLS layer or zero.
 *
 *  @remarks errno set by the network stacks are not returned by this function.
 *  The application has to check errno for such stacks.
 */
extern int HTTPCli_getSocketError(HTTPCli_Handle cli);

#ifdef __cplusplus
}
#endif

/*@}*/
#endif
