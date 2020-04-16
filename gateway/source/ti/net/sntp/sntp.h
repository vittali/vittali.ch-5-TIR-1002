/*
 * Copyright (c) 2013-2017, Texas Instruments Incorporated
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
 *  ======== sntp.h ========
 */
/**
 *  @file  ti/net/sntp/sntp.h
 *
 *  @addtogroup ti_net_sntp_SNTP SNTP Client
 *
 *  @brief SNTP Client API
 *
 *  The SNTP client provides an API to synchronize the local system clock with
 *  a server that provides time synchronization services using Network Time
 *  Protocol (NTP).
 *
 *  # Prerequisites For Use #
 *
 *  To use the SNTP client service, the user is required to provide two
 *  system time keeping functions, as well as an NTP server's IP address.
 *
 *  Furthermore, the SNTP client has been written to conform to the BSD
 *  sockets support layer. As such, it is required that the file containing any
 *  calls to the SNTP client API conform to the BSD layer requirements.
 *
 *  # SNTP Client Time Functions #
 *
 *  The required time keeping functions are called by the SNTP client in order
 *  to get or set the current time in the system ("gettime" and "settime").
 *  For example, these could be APIs which set or get the value of a Real Time
 *  Clock (RTC), such as the TI-RTOS Seconds module's Seconds_get() and
 *  Seconds_set() APIs. But, any equivalent API can be used.
 *
 *  The "gettime" and "settime" functions should have the following format:
 *  @code
 *  uint32_t (*GetTimeFxn)(void);
 *  void (*SetTimeFxn)(uint32_t newtime);
 *  @endcode
 *
 *  The "gettime" function should return the number of seconds since the Epoch
 *  (January 1, 1970). The "settime" function sets the time to the value
 *  that is passed to it. The SNTP client uses the "gettime" function to write
 *  the current system time into a request to an NTP server. The "settime"
 *  function is used to set the system time to a new value received from the NTP
 *  server.  The SNTP client handles the conversion of NTP time (time since
 *  January 1, 1900) to the local system's time (number of seconds since January
 *  1, 1970).
 *
 *  Both IPv4 and IPv6 unicast server addresses are supported (host names are
 *  not supported). Structures of type 'struct sockaddr_in' must be used for
 *  IPv4 server addresses and of type 'struct sockaddr_in6' for IPv6 server
 *  addresses.
 *
 *  # Important note on NTP protocol #
 *
 *  The NTP protocol clearly specifies that an SNTP client must never send
 *  requests to an NTP server in intervals less than 15 seconds.  It is
 *  important to respect this NTP requirement and it is the user's responsibilty
 *  to ensure that SNTP_getTime() is not called more than once in any 15
 *  second time period.
 *
 *  # Example Usage #
 *
 *  SNTP uses BSD Socket APIs by including <ti/net/socket.h> which
 *  conditionally includes the BSD socket headers for the
 *  various networking layers.  It is necessary to ensure that the
 *  following define is passed to the compiler in the application build to
 *  include the correct network layer BSD headers:
 *
 *  For SimpleLink WiFi networking layer:
 *      -DNET_SL
 *
 *  For NDK networking layer:
 *      -DNET_NDK
 *
 *  To use the SNTP client APIs, the application should include its header file
 *  as follows:
 *  @code
 *  #include <ti/net/sntp/sntp.h>
 *  @endcode
 *
 *  And, to the link line add the following SNTP library:
 *
 *  For SimpleLink WiFi networking layer:
 *      -l{NS_INSTALL_DIR}/source/ti/net/sntp/lib/sntp_sl.a{target}
 *
 *  For NDK networking layer:
 *      -l{NS_INSTALL_DIR}/source/ti/net/sntp/lib/sntp_ndk.a{target}
 *
 *  ============================================================================
 */

#ifndef ti_net_sntp_SNTP__include
#define ti_net_sntp_SNTP__include

/*! @ingroup ti_net_sntp_SNTP */
/*@{*/

#include <ti/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @name SNTP Error Codes
 *  @{
 */
/*!
 *  @brief Input arguments are invalid
 */
#define SNTP_EINVALIDARGS   (-101)

/*!
 *  @brief Failed to create a socket
 */
#define SNTP_ESOCKETFAIL    (-102)

/*!
 *  @brief The input socket address is not of AF_INET (IPv4) or AF_INET6
 *         (IPv6) family type
 */
#define SNTP_EINVALIDFAMILY (-103)

/*!
 *  @brief Failed to set receive timeout on socket
 */
#define SNTP_ESOCKETOPTFAIL (-104)

/*!
 *  @brief Failed to connect to the NTP server
 */
#define SNTP_ECONNECTFAIL   (-105)

/*!
 *  @brief Failed to send a time request to the NTP server
 */
#define SNTP_ESENDFAIL      (-106)

/*!
 *  @brief Failed to recieve the new time from the NTP server
 */
#define SNTP_ERECVFAIL      (-107)

/*!
 *  @brief NTP Server requests to reduce the update rate (RFC 5905 kiss code
 *         RATE)
 */
#define SNTP_ERATEBACKOFF   (-108)

/*!
 *  @brief NTP Server invalid or server requests to end all communications (RFC
 *         5905 kiss code DENY or RSTR)
 */
#define SNTP_EFATALNORETRY  (-109)
/*! @} */

/*!
 *  @brief Get time from NTP server
 *
 *  This function can be used to set the local time by providing the
 *  setters and getters of current time on system clock. It is the user's
 *  responsibility to periodically call this function to sync the time with
 *  NTP server. But note, the function should not be called more than once
 *  in a 15 second period to respect the NTP requirement.
 *
 *  @param  server A pointer to a sockaddr_in or sockaddr_in6 structure
 *                 containing NTP server address.
 *
 *  @param  get    A pointer to a function that returns the number of seconds
 *                 since January 1, 1970.
 *
 *  @param  set    A pointer to a function that sets the time to the supplied
 *                 value.
 *
 *  @return 0 on success, or an error code on failure
 */
extern int SNTP_getTime(struct sockaddr *server, uint32_t (*get)(void),
        void (*set)(uint32_t newtime));

/*! @} */
#ifdef __cplusplus
}
#endif

#endif
