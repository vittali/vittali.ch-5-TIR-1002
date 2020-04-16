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
#ifndef ti_net_http_HTTP2Cli__include
#define ti_net_http_HTTP2Cli__include

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @file ti/net/http/http2cli.h
 *
 *  @addtogroup ti_net_http_HTTP2Cli HTTP/2 Client
 *
 *  @brief HTTP/2 Client API
 *
 *  This module provides an HTTP client implementation of IETF standard for
 *  HTTP/2 - RFC 7540.
 *  @{
 */
#include <stdbool.h>
#include <stdint.h>

#include <ti/net/ssock.h>
#include <ti/net/tls.h>

#include "httpstd.h"
#include "http2std.h"
#include "http2hdr.h"
#include "hpack.h"

/*!
 *  @name HTTP/2 client Error Codes
 *  @{
 */
/*!
 *  @brief Socket create failed
 *
 *  @sa @ref HTTP2Cli_getSocketError()
 */
#define HTTP2Cli_ESOCKETFAIL                   (-101)

/*!
 *  @brief TCP connection to the server failed
 *
 *  @sa @ref HTTP2Cli_getSocketError()
 */
#define HTTP2Cli_ECONNECTFAIL                  (-102)

/*!
 *  @brief Cannot send data to the server
 *
 *  @sa @ref HTTP2Cli_getSocketError()
 */
#define HTTP2Cli_ESENDFAIL                     (-103)

/*!
 *  @brief Cannot recieve data to the server
 *
 *  @sa @ref HTTP2Cli_getSocketError()
 */
#define HTTP2Cli_ERECVFAIL                     (-104)

/*!
 *  @brief TCP connection timed out when waiting for data to arrive
 */
#define HTTP2Cli_ERECVTIMEOUT                  (-105)

/*!
 *  @brief Failed to configure the socket with TLS parameters
 *
 *  @sa @ref HTTP2Cli_getSocketError()
 */
#define HTTP2Cli_ETLSFAIL                      (-106)

/*!
 *  @brief No streams available on this connection. Close this connection and
 *         open a new connection.
 */
#define HTTP2Cli_ESTREAMID                     (-107)

/*!
 *  @brief Memory allocation failed due to insufficient system heap
 */
#define HTTP2Cli_EINSUFFICIENTHEAP             (-108)

/*!
 *  @brief HTTP/2 protocol error caused by the server. Close this connection.
 */
#define HTTP2Cli_EPROTOCOL                     (-109)

/*!
 *  @brief HTTP/2 frame size error caused by the server. Close this connection.
 */
#define HTTP2Cli_EFRAMESIZE                    (-110)

/*!
 *  @brief The size of headers is too big than the size that the server is
 *         ready to accept.
 */
#define HTTP2Cli_EHEADERSLISTTOOBIG            (-111)

/*!
 *  @brief The server sent a RST Stream. Check @ref HTTP2Cli_Error for error
 *         code.
 */
#define HTTP2Cli_ERSTSTREAMRECV                (-112)

/*!
 *  @brief The server sent a GO AWAY. Check @ref HTTP2Cli_Error for error
 *         code.
 */
#define HTTP2Cli_EGOAWAYRECV                   (-113)

/*!
 *  @brief Invalid request parameters input
 */
#define HTTP2Cli_EINVALIDREQUESTPARAMS         (-114)

/*
 *  @brief Could not resolve the hostname from the input URI
 */
#define HTTP2Cli_EHOSTNAMERESOLVE              (-115)

/*
 *  @brief The received PING payload does not match the sent payload
 */
#define HTTP2Cli_EPINGPAYLOAD                  (-116)
/*! @} */

/*!
 *  @brief HTTP/2 client object type
 */
typedef struct HTTP2Cli_Struct HTTP2Cli_Struct;

/*!
 *  @brief HTTP/2 client instance type
 */
typedef HTTP2Cli_Struct* HTTP2Cli_Handle;

/*!
 *  @brief HTTP2Cli callback function for handling data
 *
 *  The @ref HTTP2Cli_processResponse() function calls the registered data
 *  callback function when a data frame arrives from the HTTP/2 server. This
 *  function is called repeatedly till the end of data from the stream which is
 *  indicated by the `endStream` flag. The `streamId` can be used to match
 *  the request and the response.
 *
 *  @param[in]  cli       Instance of the HTTP connection
 *
 *  @param[in]  arg       User supplied argument
 *
 *  @param[in]  streamId  Stream ID of the response
 *
 *  @param[in]  data      Response data buffer
 *
 *  @param[in]  dataLen   Length of the `data` buffer
 *
 *  @param[in]  endStream Set if there is no more data in the stream
 */
typedef void (*HTTP2Cli_responseDataFxn)(HTTP2Cli_Handle cli, uint32_t arg,
        uint32_t streamId, uint8_t *data, uint32_t len, bool endStream);

/*!
 *  @brief HTTP2Cli callback function for handling headers
 *
 *  The @ref HTTP2Cli_processResponse() function calls the registered header
 *  callback function when a header frame arrives from the HTTP/2 server. This
 *  function is called repeatedly till the end of headers from the stream. If
 *  no data frames follow headers, the `endStream` flag will be set. The
 *  `streamId` can be used to match the request and the response.
 *
 *  @param[in]  cli         Instance of the HTTP connection
 *
 *  @param[in]  arg         User supplied argument
 *
 *  @param[in]  streamId    Stream ID of the response
 *
 *  @param[in]  status      HTTP/2 response status code
 *
 *  @param[in]  headersList List of HTTP/2 response headers
 *
 *  @param[in]  len         Length of the `headersList`
 *
 *  @param[in]  endStream   Set if there is no more data in the stream
 */
typedef void (*HTTP2Cli_responseHeadersFxn)(HTTP2Cli_Handle cli, uint32_t arg,
        uint32_t streamId, uint16_t status, HTTP2Hdr_Field *headersList,
        uint32_t len, bool endStream);

/*
 *  @brief HTTP/2 client object
 */
struct HTTP2Cli_Struct {
   Ssock_Struct ssock;
   TLS_Handle tls;
   uint32_t nextStreamId;
   int sockError;
   HTTP2Std_Settings serverSettings;
   HTTP2Cli_responseHeadersFxn headersFxn;
   HTTP2Cli_responseDataFxn dataFxn;
   HPACK_Struct hpack;
   uint32_t arg;
};

/*!
 *  @brief HTTP/2 Error object
 */
typedef struct HTTP2Cli_Error {
   uint32_t streamId;   /*!< Terminated stream Id */
   uint32_t ecode;      /*!< HTTP/2 error code */
} HTTP2Cli_Error;

/*!
 *  @brief HTTP/2 client instance config parameters
 */
typedef struct HTTP2Cli_Params {
   HTTP2Cli_responseHeadersFxn headersFxn; /*!<User supplied headers callback */
   HTTP2Cli_responseDataFxn dataFxn; /*!< User supplied data callback */
   TLS_Handle tls; /*!< TLS instance from @ref ti_net_TLS "TLS module" */
   uint32_t arg; /*!< User supplied argument that will be passed to callbacks */
} HTTP2Cli_Params;

/*!
 *  @brief Connect to the HTTP/2 Server
 *
 *  Creates a secure TCP connection to the HTTP/2 Server and exchanges the
 *  HTTP/2 connection preface and settings.
 *
 *  @param[in]  cli    Instance of an HTTP/2 client object
 *
 *  @param[in]  addr   IP address of the server
 *
 *  @param[in]  flags  Reserved for future use
 *
 *  @return 0 on success or error code on failure
 */
extern int HTTP2Cli_connect(HTTP2Cli_Handle cli, const struct sockaddr *addr,
        int flags);

/*!
 *  @brief Initialize a new instance object inside the provided structure
 *
 *  @param[in]  cli     Pointer to @ref HTTP2Cli_Struct
 *
 *  @param[in]  params  Per-instance config parameters
 */
extern void HTTP2Cli_construct(HTTP2Cli_Struct *cli,
        const HTTP2Cli_Params *params);

/*!
 *  @brief Allocate and initialize a new instance object, and return it's handle
 *
 *  @param[in]  params  Per-instance config parameters
 *
 *  @return A new HTTP/2 client instance on success or NULL on failure
 */
extern HTTP2Cli_Handle HTTP2Cli_create(const HTTP2Cli_Params *params);

/*!
 *  @brief Destroy and free this previously allocated instance object, and set
 *         the reference handle to NULL
 *
 *  @param[in]  cli  Pointer to an HTTP/2 client instance
 */
extern void HTTP2Cli_delete(HTTP2Cli_Handle *cli);

/*!
 *  @brief Destroy the instance object inside the provided structure
 *
 *  @param[in]  cli  Pointer to @ref HTTP2Cli_Struct
 */
extern void HTTP2Cli_destruct(HTTP2Cli_Struct *cli);

/*!
 *  @brief Disconnect from the HTTP/2 server
 *
 *  Closes the secure TCP connection to the HTTP/2 server and destroys the
 *  HTTP/2 client instance object.
 *
 *  @param[in]  cli  Instance of an HTTP/2 client object
 */
extern void HTTP2Cli_disconnect(HTTP2Cli_Handle cli);

/*!
 *  @brief Get the error code from the socket/TLS layer
 *
 *  This function can be used to get the underlying socket/TLS layer error
 *  code when the other HTTP2Cli APIs fail with connect/send/recv/tls errors.
 *
 *  @param[in]  cli  Instance of an HTTP/2 client object
 *
 *  @return Error code from the socket/TLS layer or 0 when no error occured
 */
extern int HTTP2Cli_getSocketError(HTTP2Cli_Handle cli);

/*!
 *  @brief  Initialize the socket address structure for the given URI
 *
 *  The supported URI format is:
 *     - [https://]host_name[:port_number][/request_uri]
 *
 *  For cases where port is not provided, the default port number is set.
 *
 *  @param[in]  uri    A null terminated URI string
 *
 *  @param[in]  ipv6   Set if IPv6 address has to be retrieved for the given
 *                     URI
 *
 *  @param[out] addr   Pointer to the sockaddr structure to be filled
 *                     with IP information.
 *
 *  @return 0 on success or error code on failure.
 */
extern int HTTP2Cli_initSockAddr(const char *uri, bool ipv6,
        struct sockaddr *addr);

/*!
 *  @brief Initialize the config params with default values
 *
 *  @param[in]  params  Pointer to @ref HTTP2Cli_Params
 */
extern void HTTP2Cli_Params_init(HTTP2Cli_Params *params);

/*!
 *  @brief Process the response from HTTP/2 server
 *
 *  This function has to be called periodically to process the response/request
 *  from the server. The headers and data frames are processed and passed to
 *  to user supplied callback functions. The other frames are processed and
 *  appropriate response/ack are sent to the HTTP/2 server.
 *
 *  @remarks It is important to call this function periodically as ack/pings
 *           have to be responded to within a resonable time.
 *
 *  @param[in]  cli        Instance of an HTTP/2 client object
 *
 *  @param[in]  timeout_ms Max time (in millisecs) allowed for function
 *                         to return
 *
 *  @param[out] error      Stream Id and error code returned by the server for
 *                         stream errors (@ref HTTP2Cli_ERSTSTREAMRECV) and
 *                         connection errors (@ref HTTP2Cli_EGOAWAYRECV) will
 *                         be set in this object.
 *
 *  @return The received HTTP/2 frame type on success, or an error code on
 *          failure
 */
extern int HTTP2Cli_processResponse(HTTP2Cli_Handle cli, uint32_t timeout_ms,
        HTTP2Cli_Error *error);

/*!
 *  @brief Make an HTTP/2 request to the server
 *
 *  Creates an HTTP/2 request based on the arguments: HTTP/2 method, URI,
 *  optional additional headers and optional data, and sends it to the HTTP/2
 *  server.
 *
 *  This function by default sets-up ":method", ":path", ":scheme" and
 *  ":authority" headers for HTTP/2 request. Any additional headers can be
 *  sent through this function.
 *
 *  A stream is created for the HTTP/2 request and the ID will be returned in
 *  `streamId`. This ID can be used to determine the response for this request.
 *
 *  @param[in]  cli                Instance of the HTTP/2 client object
 *
 *  @param[in]  method             HTTP/2 method (for ex: @ref HTTPStd_GET)
 *
 *  @param[in]  uri                A NULL terminated URI string with format
 *                                 - [https://]host_name[:port][/request_uri]
 *
 *  @param[in]  additionalHeaders  (Optional) Additional headers apart from
 *                                 the headers described above
 *
 *  @param[in]  headersLen         Length of the `additionalHeaders` array
 *
 *  @param[in]  data               (Optional) Application data
 *
 *  @param[in]  dataLen            Length of `data` buffer
 *
 *  @param[out]  streamId          An ID will be set for the stream created
 *
 *  @return 0 on success or an error code on failure
 */
extern int HTTP2Cli_sendRequest(HTTP2Cli_Handle cli, const char *method,
        const char *uri, HTTP2Hdr_Field *additionalHeaders, uint8_t headersLen,
        uint8_t *data, uint32_t dataLen, uint32_t *streamId);

/*!
 *  @brief Send HTTP/2 request headers to the server
 *
 *  Creates an HTTP/2 request based on the arguments: HTTP/2 method, URI,
 *  and optional additional headers, and sends it to the HTTP/2 server.
 *
 *  This function is similar to @ref HTTP2Cli_sendRequest() with the exception
 *  that only headers are sent to the server. It can be followed by a call to
 *  @ref HTTP2Cli_sendRequestData() to send data if any.
 *
 *  This function by default sets-up ":method", ":path", ":scheme" and
 *  ":authority" headers for HTTP/2 request. Any additional headers can be
 *  sent through it.
 *
 *  A stream is created for the HTTP/2 request and the ID will be returned in
 *  `streamId`. This ID can be used to determine the response for this request.
 *
 *  @param[in]  cli                Instance of the HTTP/2 client object
 *
 *  @param[in]  method             HTTP/2 method (for ex: @ref HTTPStd_GET)
 *
 *  @param[in]  uri                A NULL terminated URI string with format
 *                                 - [https://]host_name[:port][/request_uri]
 *
 *  @param[in]  additionalHeaders  (Optional) Additional headers apart from
 *                                 the headers described above
 *
 *  @param[in]  headersLen         Length of the `additionalHeaders` array
 *
 *  @param[in]  endStream          Set this to `true` if there is no data
 *                                 to send to the server.
 *
 *  @param[out]  streamId          An ID will be set for the stream created
 *
 *  @return 0 on success or an error code on failure
 */
extern int HTTP2Cli_sendRequestHeaders(HTTP2Cli_Handle cli, const char *method,
        const char *uri, HTTP2Hdr_Field *additionalHeaders, uint8_t headersLen,
        bool endStream, uint32_t *streamId);

/*!
 *  @brief Send HTTP/2 data to the server
 *
 *  This function sends the input data in HTTP/2 data frames to the server. It
 *  can be called multiple times with the last data to be indicated by setting
 *  the 'endStream' flag.
 *
 *  @remarks This function should be called only after a call to
 *           @ref HTTP2Cli_sendRequestHeaders().
 *
 *  @param[in]  cli                Instance of the HTTP/2 client object
 *
 *  @param[in]  streamId           Stream Id received from
 *                                 @ref HTTP2Cli_sendRequestHeaders()
 *
 *  @param[in]  data               Application data
 *
 *  @param[in]  dataLen            Length of `data` buffer
 *
 *  @param[in]  endStream          Set this to `true` when sending the last data
 *
 *  @return 0 on success or an error code on failure
 */
extern int HTTP2Cli_sendRequestData(HTTP2Cli_Handle cli, uint32_t *streamId,
        uint8_t *data, uint32_t dataLen, bool endStream);

/*!
 *  @brief Get the handle to the underlying socket
 *
 *  @remarks The socket handle should be only used for select(). Any other
 *           usage of this handle will result unknown HTTP/2 failures.
 *
 *  @param[in]  cli  Instance of the HTTP/2 client object
 *
 *  @return Socket handle on success or 0 when socket is not created
 */
extern int HTTP2Cli_getSocket(HTTP2Cli_Handle cli);

/*!
 *  @brief Send a HTTP/2 PING to the HTTP/2 server
 *
 *  @remarks The application should call `HTTP2Cli_processResponse()` to
 *           check for PING response from the server and it is responsible
 *           for timing the PING request.
 *
 *  @param[in]  cli  Instance of the HTTP/2 client object
 *
 *  @return 0 on success or an error code on failure
 */
extern int HTTP2Cli_sendPing(HTTP2Cli_Handle cli);

/*@}*/
#ifdef __cplusplus
}
#endif
#endif
