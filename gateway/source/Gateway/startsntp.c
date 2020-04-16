/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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

#include <time.h>
#include <unistd.h>

#include <semaphore.h>

#include <ti/display/Display.h>
#include <ti/net/sntp/sntp.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/device.h>
#include <Utils/uart_term.h>

#define NTP_PORT          123
#define NTP_SERVERS       1
#define NTP_SERVERS_SIZE  (NTP_SERVERS * sizeof(struct sockaddr_in))
#define NTP_TIMEOUT       30000

/* Must wait at least 15 sec to retry NTP server (RFC 4330) */
#define NTP_POLL_TIME     15

char NTP_HOSTNAME[] = "pool.ntp.org";

//extern Display_Handle AWSIOT_display;

void startNTP(void);

/*
 *  ======== setTime ========
 */
void setTime(uint32_t t)
{
    SlDateTime_t dt;
    struct tm tm;
    struct timespec tspec;
    time_t ts;

    tspec.tv_sec = t;
    clock_settime(CLOCK_REALTIME, &tspec);

    time(&ts);
    tm = *localtime(&ts);

    /* Set system clock on network processor to validate certificate */
    dt.tm_day  = tm.tm_mday;
    /* tm.tm_mon is the month since January, so add 1 to get the actual month */
    dt.tm_mon  = tm.tm_mon + 1;
    /* tm.tm_year is the year since 1900, so add 1900 to get the actual year */
    dt.tm_year = tm.tm_year + 1900;
    dt.tm_hour = tm.tm_hour;
    dt.tm_min  = tm.tm_min;
    dt.tm_sec  = tm.tm_sec;
    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME,
            sizeof(SlDateTime_t), (unsigned char *)(&dt));
}

/*
 *  ======== getTime ========
 */
uint32_t getTime(void)
{
    struct timespec tspec;

    clock_gettime(CLOCK_REALTIME, &tspec);

    return (tspec.tv_sec);
}

/*
 *  ======== startNTP ========
 */
void startNTP(void)
{
    int ret;
    int ip;
    int retryCnt = 4;
    time_t ts;
    struct sockaddr_in ntpAddr = {0};
    struct hostent *dnsEntry;
    struct in_addr **addr_list;

    dnsEntry = gethostbyname(NTP_HOSTNAME);
    if (dnsEntry == NULL) {
        UART_PRINT(" [NTP] startNTP: NTP host cannot be resolved!\n\r");
        return;
    }

    /* Get the first IP address returned from DNS */
    addr_list = (struct in_addr **)dnsEntry->h_addr_list;
    ip = (*addr_list[0]).s_addr;

    ntpAddr.sin_addr.s_addr = htonl(ip);
    ntpAddr.sin_port = htons(NTP_PORT);
    ntpAddr.sin_family = AF_INET;

    do {
        ret = SNTP_getTime((struct sockaddr *)&ntpAddr, getTime, setTime);
        if (ret != 0)
        {
            UART_PRINT("startNTP: couldn't get time (%d), will retry in %d secs ...",
                ret, NTP_POLL_TIME);
            sleep(NTP_POLL_TIME);
            UART_PRINT("startNTP: retrying ...");
        }
    } while (ret != 0 && retryCnt--);

    ts = time(NULL);
    UART_PRINT(" [NTP] Current time: %s\n\r", ctime(&ts));
}
