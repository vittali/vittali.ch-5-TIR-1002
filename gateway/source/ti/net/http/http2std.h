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
#ifndef ti_net_http_HTTP2Std__include
#define ti_net_http_HTTP2Std__include
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/*!
 *  @name HTTP/2 Frame types
 *  @{
 */
#define HTTP2Std_FRAME_TYPE_DATA           (0x0)
#define HTTP2Std_FRAME_TYPE_HEADERS        (0x1)
#define HTTP2Std_FRAME_TYPE_PRIORITY       (0x2)
#define HTTP2Std_FRAME_TYPE_RST_STREAM     (0x3)
#define HTTP2Std_FRAME_TYPE_SETTINGS       (0x4)
#define HTTP2Std_FRAME_TYPE_PUSH_PROMISE   (0x5)
#define HTTP2Std_FRAME_TYPE_PING           (0x6)
#define HTTP2Std_FRAME_TYPE_GOAWAY         (0x7)
#define HTTP2Std_FRAME_TYPE_WINDOW_UPDATE  (0x8)
#define HTTP2Std_FRAME_TYPE_CONTINUATION   (0x9)
/*! @} */

/*!
 *  @name HTTP/2 error codes
 *  @{
 */
#define HTTP2Std_ECODE_NO_ERROR                    (0x0)
#define HTTP2Std_ECODE_PROTOCOL_ERROR              (0x1)
#define HTTP2Std_ECODE_INTERNAL_ERROR              (0x2)
#define HTTP2Std_ECODE_FLOW_CONTROL_ERROR          (0x3)
#define HTTP2Std_ECODE_SETTINGS_TIMEOUT            (0x4)
#define HTTP2Std_ECODE_STREAM_CLOSED               (0x5)
#define HTTP2Std_ECODE_FRAME_SIZE_ERROR            (0x6)
#define HTTP2Std_ECODE_REFUSED_STREAM              (0x7)
#define HTTP2Std_ECODE_CANCEL                      (0x8)
#define HTTP2Std_ECODE_COMPRESSION_ERROR           (0x9)
#define HTTP2Std_ECODE_CONNECT_ERROR               (0xa)
#define HTTP2Std_ECODE_ENHANCE_YOUR_CALM           (0xb)
#define HTTP2Std_ECODE_INADEQUATE_SECURITY         (0xc)
#define HTTP2Std_ECODE_HTTP_1_1_REQUIRED           (0xd)
/*! @} */

/*!
 * @cond INTERNAL
 */
/* HTTP/2 Frame Header length (in Octets) */
#define HTTP2Std_FRAME_HEADER_LEN          (9)

/* HTTP/2 Frame Header Field lengths (in Octets)*/
#define HTTP2Std_FRAME_HEADER_FIELD_LENGTH_LEN    (3)
#define HTTP2Std_FRAME_HEADER_FIELD_TYPE_LEN      (1)
#define HTTP2Std_FRAME_HEADER_FIELD_FLAGS_LEN     (1)
#define HTTP2Std_FRAME_HEADER_FIELD_STREAM_ID_LEN (4)

/* HTTP/2 flags */
#define HTTP2Std_FLAGS_ACK         (0x1)
#define HTTP2Std_FLAGS_END_STREAM  (0x1)
#define HTTP2Std_FLAGS_END_HEADERS (0x4)
#define HTTP2Std_FLAGS_PADDED      (0x8)
#define HTTP2Std_FLAGS_PRIORITY    (0x20)

/* HTTP/2 stream id values */
#define HTTP2Std_STREAM_ID_MAX                       (0x7FFFFFFF)
#define HTTP2Std_STREAM_ID_CLIENT_DEFAULT            (1)
#define HTTP2Std_STREAM_ID_DEFAULT                   (0)

/* HTTP/2 Settings payload length (in Octets) */
#define HTTP2Std_SETTINGS_PAYLOAD_LEN (36)

/* HTTP/2 Settings field lengths (in Octets) */
#define HTTP2Std_SETTINGS_PARAMS_LEN       (6)
#define HTTP2Std_SETTINGS_FIELD_ID_LEN     (2)
#define HTTP2Std_SETTINGS_FIELD_VALUE_LEN  (4)

/* HTTP/2 Settings identifier */
#define HTTP2Std_SETTINGS_HEADER_TABLE_SIZE      (0x1)
#define HTTP2Std_SETTINGS_ENABLE_PUSH            (0x2)
#define HTTP2Std_SETTINGS_MAX_CONCURRENT_STREAMS (0x3)
#define HTTP2Std_SETTINGS_INITIAL_WINDOW_SIZE    (0x4)
#define HTTP2Std_SETTINGS_MAX_FRAME_SIZE         (0x5)
#define HTTP2Std_SETTINGS_MAX_HEADER_LIST_SIZE   (0x6)

/* HTTP/2 Settings values (in Octets) */
#define HTTP2Std_SETTINGS_HEADER_TABLE_SIZE_DEFAULT  (4096)
#define HTTP2Std_SETTINGS_CONCURRENT_STREAMS_DEFAULT (100)
#define HTTP2Std_SETTINGS_WINDOW_SIZE_DEFAULT        (65535)
#define HTTP2Std_SETTINGS_FRAME_SIZE_MAX             (16777215)
#define HTTP2Std_SETTINGS_FRAME_SIZE_DEFAULT         (16384)
#define HTTP2Std_SETTINGS_HEADER_LIST_SIZE_DEFAULT   (1024)

/* HTTP/2 Header field lengths (in Octets) */
#define HTTP2Std_HEADER_FIELD_PAD_LEN         (1)
#define HTTP2Std_HEADER_STREAM_DEPENDENCY_LEN (4)
#define HTTP2Std_HEADER_WEIGHT_LEN            (1)

/* HTTP/2 Data field lengths (in Octets) */
#define HTTP2Std_DATA_FIELD_PAD_LEN         (1)

/* HTTP/2 GO AWAY payload length (in Octets) */
#define HTTP2Std_GOAWAY_PAYLOAD_LEN   (8)

/* HTTP/2 GO AWAY payload field length (in Octets) */
#define HTTP2Std_GOAWAY_PAYLOAD_FIELD_STREAM_ID_LEN  (4)
#define HTTP2Std_GOAWAY_PAYLOAD_FIELD_ERROR_CODE_LEN (4)

/* HTTP/2 payload lengths (in Octets) */
#define HTTP2Std_RST_PAYLOAD_LEN      (4)

/* HTTP/2 payload lengths (in Octets) */
#define HTTP2Std_PING_PAYLOAD_LEN     (8)

/* HTTP/2 pseudo headers */
#define HTTP2Std_PSEUDO_HEADER_METHOD    ":method"
#define HTTP2Std_PSEUDO_HEADER_PATH      ":path"
#define HTTP2Std_PSEUDO_HEADER_SCHEME    ":scheme"
#define HTTP2Std_PSEUDO_HEADER_AUTHORITY ":authority"
#define HTTP2Std_PSEUDO_HEADER_STATUS    ":status"

/* HTTP/2 Connection Preface */
#define HTTP2Std_CLIENT_CONNECTION_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

/*
 *  @brief HTTP/2 Peer Settings
 */
typedef struct HTTP2Std_Settings {
    uint32_t headerTableSize;
    uint32_t concurrentStreams;
    uint32_t windowSize;
    uint32_t frameSize;
    uint32_t headersListSize;
} HTTP2Std_Settings;
/*! @endcond */

#ifdef __cplusplus
}
#endif
#endif
