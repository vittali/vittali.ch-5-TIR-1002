/*
 * Copyright (c) 2014-2017, Texas Instruments Incorporated
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
 *  ======== sntp.c ========
 */
#include <string.h>

#include "sntp.h"

/*
 *  Time Base Conversion Macros
 *
 *  The NTP timebase is 00:00 Jan 1 1900.  The local
 *  time base is 00:00 Jan 1 1970.  Convert between
 *  these two by added or substracting 70 years
 *  worth of time.  Note that 17 of these years were
 *  leap years.
 */
#define TIME_BASEDIFF        ((((unsigned int)70 * 365 + 17) * 24 * 3600))
#define TIME_NTP_TO_LOCAL(t) ((t) - TIME_BASEDIFF)
#define TIME_LOCAL_TO_NTP(t) ((t) + TIME_BASEDIFF)

/* Time to wait for reply from server (seconds) */
#define SNTP_REPLY_WAIT_TIME 5

/* KOD error code: rate exceeded, server requesting NTP client to back off */
#define SNTP_KOD_RATE_STR "RATE"
#define SNTP_KOD_RATE_CODE 3

/* KOD error code: access denied, server requests client to end all comm */
#define SNTP_KOD_DENY_STR "DENY"
#define SNTP_KOD_DENY_CODE 2

/* KOD error code: access denied, server requests client to end all comm */
#define SNTP_KOD_RSTR_STR "RSTR"
#define SNTP_KOD_RSTR_CODE 1

/* Size of KOD error codes */
#define SNTP_KOD_ERROR_CODE_SIZE 4

/* Use NTP version 4 */
#define SNTP_VERSION 4

/* Flag value for unsync'ed leap indicator field, signifying server error */
#define SNTP_NOSYNC 3

/* NTP mode defined in RFC 4330 */
#define SNTP_MODE_CLIENT 3

/* SNTP Header (as specified in RFC 4330) */
typedef struct _SNTP_Header {
    /*
     *  'flags' stores three values:
     *
     *    - 2 bit Leap Indicator (LI)
     *    - 3 bit Version Number (VN)
     *    - 3 bit Mode.
     */
    unsigned char flags;
    unsigned char stratum;
    unsigned char poll;
    signed char   precision;
    int           rootDelay;
    unsigned int  rootDispersion;
    unsigned int  referenceID;

    /* NTP time stamps */
    unsigned int referenceTS[2];
    unsigned int originateTS[2];
    unsigned int receiveTS[2];
    unsigned int transmitTS[2];
} _SNTP_Header;

/* Function prototypes */
static int hasKissCode(char *str);
static int socketSetup(struct sockaddr *cs, int *s);

/*
 *  ======== hasKissCode ========
 *
 *  Utility function to check if a string contains a Kiss O' Death code.
 *
 *  Returns:
 *      SNTP_KOD_RATE_CODE - str contains "RATE" KOD code
 *
 *      SNTP_KOD_DENY_CODE - str contains "DENY" KOD code
 *
 *      SNTP_KOD_RSTR_CODE - str contains "RSTR" KOD code
 *
 *      0 - str does not contain any of the above KOD codes
 */
static int hasKissCode(char *str)
{
    if (strncmp((char *)SNTP_KOD_RATE_STR, str, SNTP_KOD_ERROR_CODE_SIZE)
            == 0) {

        return (SNTP_KOD_RATE_CODE);
    }
    else if (strncmp((char *)SNTP_KOD_DENY_STR, str, SNTP_KOD_ERROR_CODE_SIZE)
            == 0) {

        return (SNTP_KOD_DENY_CODE);
    }
    else if (strncmp((char *)SNTP_KOD_RSTR_STR, str, SNTP_KOD_ERROR_CODE_SIZE)
            == 0) {

        return (SNTP_KOD_RSTR_CODE);
    }
    else {
        return (0);
    }
}

/*
 *  ======== socketSetup ========
 *
 *  Utility function to create and connect the SNTP client socket.
 *  This socket can then be used to generically call send() and recv() for
 *  either IPv4 or IPv6.
 *
 *  Returns 0 on success, or an error code on failure.
 */
static int socketSetup(struct sockaddr *cs, int *s)
{
    int status;
    struct timeval to;

    /* If socket already exists close it */
    if (*s != -1) {
        close(*s);
    }

    /* Create a UDP socket to communicate with NTP server */
    *s = socket(cs->sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (*s == -1) {
        return (SNTP_ESOCKETFAIL);
    }

    /* Get size of the current server struct (re-use variable to save space) */
    if (cs->sa_family == AF_INET) {
        status = sizeof(struct sockaddr_in);
    }
    else if (cs->sa_family == AF_INET6) {
        status = sizeof(struct sockaddr_in6);
    }
    else {
        return (SNTP_EINVALIDFAMILY);
    }

    /*
     *  Connect our UDP socket. We only want to recv replies from the NTP
     *  server on this socket:
     */
    status = connect(*s, cs, status);
    if (status == -1) {
        return (SNTP_ECONNECTFAIL);
    }

    /* Set a timeout for server response */
    to.tv_sec  = SNTP_REPLY_WAIT_TIME;
    to.tv_usec = 0;
    if (setsockopt(*s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(struct timeval))
            == -1) {
        return (SNTP_ESOCKETOPTFAIL);
    }

    /* Socket creation success */
    return (0);
}

/*
 *  ======== SNTP_getTime ========
 */
int SNTP_getTime(struct sockaddr *server, uint32_t (*get)(void),
        void (*set)(uint32_t newtime))
{
    int sntpSocket = -1;
    _SNTP_Header sntpPkt;
    int ret = 0;

    if (!server || !get || !set) {
        ret = SNTP_EINVALIDARGS;
        goto sntpFail;
    }

    ret = socketSetup(server, &sntpSocket);
    if (ret < 0) {
        goto sntpFail;
    }

    /* Initialize the SNTP packet, setting version and mode = client */
    memset(&sntpPkt, 0, sizeof(_SNTP_Header));
    sntpPkt.flags = SNTP_VERSION << 3;
    sntpPkt.flags |= SNTP_MODE_CLIENT;

    /* Set packet's transmit time to the current time on our clock */
    sntpPkt.transmitTS[0] = htonl(TIME_LOCAL_TO_NTP(get()));

    /* Send out our SNTP request to the current server */
    ret = send(sntpSocket, (void *)&sntpPkt, sizeof(_SNTP_Header), 0);
    if (ret < 0) {
        ret = SNTP_ESENDFAIL;
        goto sntpFail;
    }

    memset(&sntpPkt, 0, sizeof(_SNTP_Header));

    /* Retrieve the NTP packet from the socket and update our time. */
    ret = recv(sntpSocket, &sntpPkt, sizeof(_SNTP_Header), 0);
    if ((ret < 0) || (ret != sizeof(_SNTP_Header))) {
        ret = SNTP_ERECVFAIL;
        goto sntpFail;
    }

    /* Check for errors in server response */
    if (sntpPkt.stratum == 0) {

        /* Per RFC5905, we MUST handle Kiss O' Death packet */
        if ((sntpPkt.flags >> 6) == SNTP_NOSYNC) {

            /* KOD recv'd. Inspect kiss code & handle accordingly */
            ret = hasKissCode((char *)&sntpPkt.referenceID);

            if (ret == SNTP_KOD_RATE_CODE) {
                ret = SNTP_ERATEBACKOFF;
                goto sntpFail;
            }
            /* Check for fatal kiss codes */
            else if ((ret == SNTP_KOD_DENY_CODE)
                    || (ret == SNTP_KOD_RSTR_CODE)) {
                ret = SNTP_EFATALNORETRY;
                goto sntpFail;
            }
            /* Per RFC5905, other kiss codes are ignored */
        }
        else {
            /*
             *  A server response with stratum == 0, with no kiss
             *  code, is a fatal error. Mark server as invalid
             */
            ret = SNTP_EFATALNORETRY;
            goto sntpFail;
        }
    }

    /* Use server's transmit time to update our clock */
    set(TIME_NTP_TO_LOCAL(ntohl(sntpPkt.transmitTS[0])));

    ret = 0;

sntpFail:
    if (sntpSocket != -1) {
        close(sntpSocket);
    }

    return (ret);
}
