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
#ifndef ti_net_TLS__include
#define ti_net_TLS__include

#ifdef __cplusplus
extern "C" {
#endif
/*!
 *  @file ti/net/tls.h
 *
 *  @addtogroup ti_net_TLS TLS
 *
 *  @brief TLS Abstraction Layer
 *
 *  This module provides a simple portable interface to create and delete TLS
 *  contexts for various TLS layers like SimpleLink WiFi TLS and WolfSSL. These
 *  contexts can be shared with supported networking protocols like HTTP, MQTT
 *  and other protocols which require TLS and are connecting to the same host
 *  server.
 *
 *  The certificates can be provided either as a buffer input or as a string
 *  containing the certificate paths on the file system.
 *
 *  The file system based approach is supported only for TI-RTOS/SimpleLink WiFi
 *  and Linux/Sitara devices. The certificates can be set by providing the
 *  certificate file path using @ref TLS_setCertFile().
 *
 *  The buffer based approach is supported only for TI-RTOS/NDK and
 *  Linux/Sitara devices. The certificates can be set by providing the
 *  certificate buffers using @ref TLS_setCertBuf().
 *
 *  Additionally, for TI-RTOS/SimpleLink WiFi and Linux/Sitara devices
 *  certificate buffers can be written to file system using
 *  @ref TLS_writeDerFile() and set using @ref TLS_setCertFile().
 *
 *  A brief usage of TLS APIs is shown below as pseudo codes for different
 *  supported platforms
 *
 *  TI-RTOS/SimpleLink WiFi and Linux/Sitara (make sure file path is
 *  accessible):
 *  @code
 *  #include <ti/net/tls.h>
 *
 *  tls = TLS_create(TLS_METHOD_CLIENT_TLSV1_2);
 *
 *  //Create and flash the root CA PEM data (stored in a buffer) to a .der file.
 *  //If the *.der files are already flashed, you don't need to do this step.
 *  TLS_writeDerFile(caBuf, caBufLen, TLS_CERT_FORMAT_PEM, "/certs/ca.der");
 *
 *  TLS_setCertFile(tls, TLS_CERT_TYPE_CA, TLS_CERT_FORMAT_DER,
 *          "/certs/ca.der");
 *
 *  TLS_delete(&tls);
 *  @endcode
 *
 *  TI-RTOS/NDK and Linux/Sitara:
 *  @code
 *  #include <ti/net/tls.h>
 *
 *  tls = TLS_create(TLS_METHOD_CLIENT_TLSV1_2);
 *
 *  TLS_setCertBuf(tls, TLS_CERT_TYPE_CA, TLS_CERT_FORMAT_PEM, caBuf, caBufLen);
 *
 *  TLS_delete(&tls);
 *  @endcode
 *
 */
/*! @ingroup ti_net_TLS */
/*@{*/
#include <stdint.h>

/*!
 *  @name TLS Error Codes
 *  @{
 */
/*!
 *  @brief Input parameters are invalid
 */
#define TLS_EINVALIDPARAMS   (-11)

/*!
 *  @brief This feature is not supported on the network stack
 */
#define TLS_ENOTSUPPORTED    (-12)

/*!
 *  @brief Not enough heap to allocate required memory
 */
#define TLS_EALLOCFAIL       (-13)

/*!
 *  @brief Loading the certificate on SSL stack failed
 */
#define TLS_ECERTLOADFAIL    (-14)

/*!
 *  @brief Writing the certificate to filesystem failed
 */
#define TLS_ECERTWRITEFAIL   (-15)
/*! @} */

/*!
 *  @name TLS Method
 *  @{
 */
typedef enum TLS_Method {
   TLS_METHOD_CLIENT_TLSV1 = 1, /*!< TLS v1 Client */
   TLS_METHOD_CLIENT_TLSV1_1,   /*!< TLS v1.1 Client */
   TLS_METHOD_CLIENT_TLSV1_2,   /*!< TLS v1.2 Client */
   TLS_METHOD_SERVER_TLSV1,     /*!< TLS v1 Server */
   TLS_METHOD_SERVER_TLSV1_1,   /*!< TLS v1.1 Server */
   TLS_METHOD_SERVER_TLSV1_2,   /*!< TLS v1.2 Server */
} TLS_Method;
/*! @} */

/*!
 *  @name TLS Certificate Type
 *  @{
 */
typedef enum TLS_Cert_Type {
   TLS_CERT_TYPE_CA = 1,     /*!< Root CA certificate */
   TLS_CERT_TYPE_CERT,       /*!< Client/Server certificate */
   TLS_CERT_TYPE_KEY,        /*!< Client/Server private key */
   TLS_CERT_TYPE_DH_KEY      /*!< Diffie-Hellman key (SimpleLink WiFi only) */
} TLS_Cert_Type;
/*! @} */

/*!
 *  @name TLS Certificate Format
 *  @{
 */
typedef enum TLS_Cert_Format {
   TLS_CERT_FORMAT_DER = 1, /*!< DER encoded certificates */
   TLS_CERT_FORMAT_PEM      /*!< PEM encoded certificates */
} TLS_Cert_Format;
/*! @} */

typedef void *TLS_Handle;

/*!
 *  @brief  Allocate and initialize a new TLS context and return its handle
 *
 *  @param[in] method    TLS version (see @ref TLS_Method)
 *
 *  @return a TLS_Handle on success or NULL on failure
 */
extern TLS_Handle TLS_create(TLS_Method method);

/*!
 *  @brief  Destroy the TLS context instance and free the previously allocated
 *          instance object.
 *
 *  @param[in] tls    Pointer to the TLS context instance
 */
extern void TLS_delete(TLS_Handle *tls);

/*!
 *  @brief  Set the certificate files required for TLS handshake
 *
 *  It takes the path to a valid certificate on the file system as input.
 *
 *  @remarks This function is not supported for NDK/WolfSSL.
 *
 *  @param[in] tls       TLS context instance
 *
 *  @param[in] type      Certificate type as defined in @ref TLS_Cert_Type
 *
 *  @param[in] format    Certificate format as defined in @ref TLS_Cert_Format
 *
 *  @param[in] filePath  Path to the certificate on the file system. Note, the
 *                       string has to be persistent throughout the life-cycle
 *                       of the TLS context.
 *
 *  @return 0 on success or an error code on failure
 *                          TLS_EINVALIDPARAMS,
 *                          TLS_ECERTLOADFAIL,
 *                          TLS_ENOTSUPPORTED.
 */
extern int TLS_setCertFile(TLS_Handle tls, TLS_Cert_Type type,
        TLS_Cert_Format format, const char *filePath);

/*!
 *  @brief  Set the certificate buffers required for TLS handshake
 *
 *  It takes a valid certificate buffer as input and loads it on the
 *  TLS context.
 *
 *  @remarks This function is not supported for SimpleLink WiFi. Instead, use
 *           @ref TLS_writeDerFile() to write the certificate buffer to file
 *           and @ref TLS_setCertFile() to load the file from file system.
 *
 *  @param[in] tls      TLS context instance
 *
 *  @param[in] type     Certificate type as defined in @ref TLS_Cert_Type
 *
 *  @param[in] format   Certificate format as defined in @ref TLS_Cert_Format
 *
 *  @param[in] buf      Certificate buffer
 *
 *  @param[in] buflen   Length of 'buf' buffer
 *
 *  @return 0 on success or an error code on failure
 *                          TLS_EALLOCFAIL,
 *                          TLS_ECERTLOAFAIL,
 *                          TLS_ENOTSUPPORTED.
 */
extern int TLS_setCertBuf(TLS_Handle tls, TLS_Cert_Type type,
        TLS_Cert_Format format, uint8_t *buf, uint32_t buflen);

/*!
 *  @brief Convert and write DER encoded certificate buffers to file system
 *
 *  It takes a valid certificate buffer as input, converts it to DER encoding
 *  if the input buffer is in PEM encoding and writes to file system to the
 *  location provided as input.
 *
 *  @remarks This function is not supported for NDK/WolfSSL.
 *
 *  @remarks The files written to the file system are not deleted by this TLS
 *           module. It is the user's responsibility to delete the files if
 *           needed.
 *
 *  @param[in] buf       Certificate buffer
 *
 *  @param[in] buflen    Length of 'buf' buffer
 *
 *  @param[in] format    Certificate format as defined in @ref TLS_Cert_Format
 *
 *  @param[in] filePath  Path to write the certificate on the file system with
 *                       ".der" extension. Note, the string has to be
 *                       persistent throughout the life-cycle of the TLS
 *                       context.
 *
 *  @return 0 on success or an error code on failure
 *                          TLS_EALLOCFAIL,
 *                          TLS_ECERTWRITEFAIL,
 *                          TLS_ENOTSUPPORTED.
 */
extern int TLS_writeDerFile(uint8_t *buf, uint32_t buflen,
        TLS_Cert_Format format, const char *filePath);

/*!
 *  @brief write PEM encoded certificate buffers to file system
 *
 *  It takes a valid certificate buffer in PEM format as input, and writes to
 *  file system to the location provided as input.
 *
 *  The PEM certificate buffer should include headers and footers. For example,
 *  the certificates should begin and end with:
 *
 *      -----BEGIN CERTIFICATE-----\n
 *      -----END CERTIFICATE-----\n
 *
 *  Each of the header, certificate data and footer line in the buffer should be
 *  terminated by a newline character.
 *
 *  @remarks This function is not supported for NDK/WolfSSL.
 *
 *  @remarks The files written to the file system are not deleted by this TLS
 *           module. It is the user's responsibility to delete the files if
 *           needed.
 *
 *  @param[in] buf       Certificate buffer
 *
 *  @param[in] buflen    Length of 'buf' buffer
 *
 *  @param[in] type      Certificate type as defined in @ref TLS_Cert_Type
 *
 *  @param[in] format    Certificate format as defined in @ref TLS_Cert_Format
 *
 *  @param[in] filePath  Path to write the certificate on the file system with
 *                       ".pem" extension. Note, the string has to be
 *                       persistent throughout the life-cycle of the TLS
 *                       context.
 *
 *  @return 0 on success or an error code on failure
 *                          TLS_ECERTWRITEFAIL,
 *                          TLS_ENOTSUPPORTED.
 */
extern int TLS_writePemFile(uint8_t *buf, uint32_t buflen,
        TLS_Cert_Type type, TLS_Cert_Format format, const char *filePath);

/*! @} */
#ifdef __cplusplus
}
#endif
#endif
