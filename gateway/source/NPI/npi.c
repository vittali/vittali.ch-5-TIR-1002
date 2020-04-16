/******************************************************************************

 @file npi.c

 @brief

 Group: WCS LPC
 Target Device: CC13xx CC32xx

 ******************************************************************************

 Copyright (c) 2016-2017, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name:
 Release Date:
 *****************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <Common/commonDefs.h>
#include <Board.h>
#include <npiParse.h>
#include "mqueue.h"
#include "Transport/comTransport.h"

#if defined(NPI_USE_UART)
#define TRANSPORT_IFACE_PORT Board_UART1

#elif defined(NPI_USE_SPI)
#define TRANSPORT_IFACE_PORT Board_SPI0

#else
#error "Please define the transport interface to be used for the NPI"
#endif

static void * npiThread(void *pvParameters);

static uint8_t npiFrameBuffer[MT_MAX_LEN];
static mqd_t npiMqHandle = NULL;
static mqd_t appRegisterMq = NULL;
pthread_t npiThreadHandle = (pthread_t) NULL;


int npiInit(const char *npiMqName)
{
    uint8_t transportPort = TRANSPORT_IFACE_PORT;
    mq_attr attr;
    pthread_attr_t pAttrs;
    struct sched_param priParam;
    int ret = 0;
    /* mode is not implemented in TI-RTOS POSIX wrappers */
    /* modify mode as needed if running in a different platform */
    unsigned mode = 0;

    /* sync object for inter thread communication                             */
    attr.mq_maxmsg = 50;
    attr.mq_msgsize = sizeof(msgQueue_t);
    npiMqHandle = mq_open(npiMqName, O_CREAT, mode, &attr);

    pthread_attr_init(&pAttrs);
    priParam.sched_priority = NPI_TASK_PRI;
    pthread_attr_setschedparam(&pAttrs, &priParam);
    pthread_attr_setstacksize(&pAttrs, 1024);
    pthread_attr_setdetachstate(&pAttrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&npiThreadHandle, &pAttrs, npiThread, NULL);

    ret  = transportOpen(&transportPort);
    transportRegisterMq(&npiMqHandle);
    return ret;
}
//TODO:
// allow multiple threads to register with npi client to receive npi messages
// specify protocol/technology that you are registering for.
// for example one task registers for 15.4 stack and another one registers for BLE
void npiCliMqReg(const char *npiClientMq)
{
    appRegisterMq = mq_open(npiClientMq, O_RDWR); // set to read write so we can flush srsps
    mtRegisterSrspMq(MT_SRSP_MQ);
    mtRegisterClientMq(&appRegisterMq);
    mtRegisterServerMq(&npiMqHandle);

    //trigger an initial read
    transportRead(npiFrameBuffer, 1);

}

void * npiThread(void *pvParameters)
{
    msgQueue_t incomingMsg;
    uint32_t bytesToRead = 0;
    for(;;)
    {
        incomingMsg.event = 0xff;
        incomingMsg.msgPtr = NULL;
        incomingMsg.msgPtrLen = 0;
        // block here and wait for a msg from either transport or the application
        mq_receive(npiMqHandle, (char*)&incomingMsg, sizeof(msgQueue_t), NULL);
        switch (incomingMsg.event)
        {
        case NPIEvent_TRANSPRT_RX:
            bytesToRead = mtProcessInCmd((uint8_t*)incomingMsg.msgPtr, incomingMsg.msgPtrLen);
            transportRead(npiFrameBuffer, bytesToRead); // kick off a non-blocking read
            if(incomingMsg.msgPtr == npiFrameBuffer)
            {
                incomingMsg.msgPtr = NULL;
            }
            break;
        case NPIEvent_TRANSPRT_TX:
        {
            int32_t writenBytes = 0;
            if(incomingMsg.msgPtr != NULL)
            {
                writenBytes = transportWrite(incomingMsg.msgPtr, incomingMsg.msgPtrLen);
                if(writenBytes != incomingMsg.msgPtrLen)
                {
                    writenBytes = transportWrite(incomingMsg.msgPtr, incomingMsg.msgPtrLen);
                    //something went wrong, either report back to the application or retry uart write
                }
            }
        }
            break;
        }
        if(incomingMsg.msgPtr)
        {
            free(incomingMsg.msgPtr);
        }
    }
}




