/******************************************************************************

 @file npiParse.c

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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <Common/commonDefs.h>
#include <Utils/uart_term.h>
#include "npiParse.h"

#define xNPI_DEBUG

#define MT_SRSP_SLEEP_TIMEOUT 50 // ms
#define MT_SRSP_RETRY_COUNT  100

static mqd_t *clientMq = NULL;
static mqd_t mtSrspMq;
static mqd_t *mtServerMq = NULL; // NPI queue from applications perspective

/*!----------------------------------------------------------------------------
 * \brief  Calculates FCS for MT Frame
 *
 * \param  msg_ptr   Pointer to message buffer.
 * \param  len       Length of message pointer pointed to by msg_ptr.
 *
 * \return     uint8_t   Calculated FCS.
 *---------------------------------------------------------------------------*/
static uint8_t mtCalcFCS(uint8_t *msg_ptr, uint8_t len);
static void Mt_bufToMsg(mtMsg_t *inMtMsg, uint8_t *pBuf);


void mtRegisterClientMq(mqd_t *mqHandle)
{
    clientMq = mqHandle;
}
void mtRegisterSrspMq(const char *mtMqName)
{
    mq_attr attr;
    attr.mq_maxmsg = 30;
    attr.mq_msgsize = sizeof(msgQueue_t);
    mtSrspMq = mq_open(mtMqName, O_CREAT | O_NONBLOCK, 0, &attr);
}
void mtRegisterServerMq(mqd_t *mqHandle)
{
    mtServerMq = mqHandle;
}
uint32_t mtProcessInCmd(uint8_t * data, uint32_t len)
{
    static uint8_t rxState = MT_WAITING_SOF;
    static uint8_t *currentMtPacket = NULL;
    uint32_t readBytes = MT_SOF_LEN;

    switch(rxState)
    {
        case MT_WAITING_SOF:
            if(len == MT_SOF_LEN && data[0] == MT_SOF)
            {
                readBytes = MT_HDR_LEN; // TODO: if supporting Unified NPI modify this length
                rxState = MT_WAITING_HEADER;
            }
        break;
        case MT_WAITING_HEADER:
            // header parser, check if there is SOF if not return 4 to read another header dont change state
            if(len == MT_HDR_LEN) // if for some reason this function is called with a different length then something went wrong go back to waiting for SOF
            {
                currentMtPacket = (uint8_t*)malloc((size_t)(MT_HDR_LEN + data[0]));
                memcpy(currentMtPacket, data, MT_HDR_LEN);
                //TODO: if supporting unified NPI then modify line below because unified uses 2 Byte length
                readBytes = (uint32_t)(data[0] + MT_FCS_LEN);
                rxState = MT_WAITING_DATA;
            }
            else // something went wrong go back to waiting for an SOF
            {
                rxState = MT_WAITING_SOF;
                readBytes = MT_SOF_LEN;
            }
        break;
        case MT_WAITING_DATA:
        {
            uint8_t currLen = 0;
            uint8_t calcFCS = 0;
            if(currentMtPacket != NULL)
            {
                currLen = currentMtPacket[0];
            }
            if(len == (currLen + MT_FCS_LEN))
            {
                memcpy((currentMtPacket + MT_HDR_LEN), data, currLen);
                calcFCS = mtCalcFCS(currentMtPacket, MT_HDR_LEN + currLen);
                if(calcFCS == data[len - 1])
                {
                    unsigned int prio = (currentMtPacket[1] & MT_CMD_TYPE_MASK) == MT_CMD_SRSP ? MQ_HIGH_PRIOR : MQ_LOW_PRIOR;
                    msgQueue_t clientReportMsg;
                    //TODO: modify below if supporting multiple clients
                    clientReportMsg.event = CollectorEvent_PROCESS_NPI_CMD;
                    clientReportMsg.msgPtr = currentMtPacket;
                    clientReportMsg.msgPtrLen = (int32_t)(MT_HDR_LEN + currLen);
#ifdef NPI_DEBUG
                    if(currentMtPacket[3] != 0 && clientReportMsg.msgPtrLen == 4)
                    {
                        UART_PRINT("Potential ERROR in packet below. Status = %02X\n\r", currentMtPacket[3]);
                    }
                    UART_PRINT("[NPI] IN ------> len: 0x%02X cmd0: 0x%02X cmd1: 0x%02X data: ", currentMtPacket[0], currentMtPacket[1], currentMtPacket[2]);
                    for(int i = 3; i < clientReportMsg.msgPtrLen; i++)
                    {
                        UART_PRINT("%02X ", currentMtPacket[i]);
                    }
                    UART_PRINT("\n\r");
#endif
                    if(prio == MQ_HIGH_PRIOR)
                    {
                        clientReportMsg.event = MtEvent_RECEIVED_SRSP;
                        mq_send(mtSrspMq, (char*)&clientReportMsg, sizeof(msgQueue_t), prio);
                    }
                    else
                    {
                        mq_send(*clientMq, (char*)&clientReportMsg, sizeof(msgQueue_t), prio);
                    }

                }
                else
                {
                    if(currentMtPacket != NULL)
                    {
                        free(currentMtPacket);
                        currentMtPacket = NULL;
                    }
                }

            }
            else
            {
                if(currentMtPacket != NULL)
                {
                    free(currentMtPacket);
                    currentMtPacket = NULL;
                }

                // TODO: something went wrong length does not match
                // this should never happen add log here
            }
            //there is no break in this one intentionally since we need to go back to SOF state
        }
        /* no break */
        default:
            rxState = MT_WAITING_SOF;
            readBytes = MT_SOF_LEN;
        break;
    }
    return readBytes;
}

void Mt_bufToMsg(mtMsg_t *inMtMsg, uint8_t *pBuf)
{
    uint8_t *temppBuf = pBuf;
    if(pBuf)
    {
        inMtMsg->len = *temppBuf;
        temppBuf++;
        inMtMsg->cmd0 = *temppBuf;
        temppBuf++;
        inMtMsg->cmd1 = *temppBuf;
        temppBuf++;
        inMtMsg->attrs = (uint8_t*)malloc(inMtMsg->len);
        memcpy(inMtMsg->attrs, temppBuf, inMtMsg->len);
    }
}
void Mt_sendCmd(mtMsg_t *cmdDesc)
{
    int32_t outCmdLen = cmdDesc->len + MT_HDR_LEN + MT_SOF_LEN + MT_FCS_LEN;
    uint8_t *cmdBuf = malloc(outCmdLen);
    msgQueue_t serverReportMsg;
    cmdBuf[0] = MT_SOF;
    cmdBuf[1] = cmdDesc->len;
    cmdBuf[2] = cmdDesc->cmd0;
    cmdBuf[3] = cmdDesc->cmd1;
    memcpy(&cmdBuf[4], cmdDesc->attrs, cmdDesc->len);
    cmdBuf[outCmdLen - 1] = mtCalcFCS(&cmdBuf[1], cmdDesc->len + MT_HDR_LEN);
    serverReportMsg.event = NPIEvent_TRANSPRT_TX;
    serverReportMsg.msgPtr = cmdBuf;
    serverReportMsg.msgPtrLen = (int32_t)(outCmdLen);
#ifdef NPI_DEBUG
    UART_PRINT("[NPI] OUT----> len: 0x%02X cmd0: 0x%02X cmd1: 0x%02X data: ", cmdBuf[1], cmdBuf[2], cmdBuf[3]);
    for(int i = 4; i < outCmdLen; i++)
    {
        UART_PRINT("%02X ", cmdBuf[i]);
    }
    UART_PRINT("\n\r");
#endif
    //send message to NPI task
    mq_send(*mtServerMq, (char*)&serverReportMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}


uint8_t Mt_rcvSrsp(mtMsg_t *cmdDesc)
{
    mtMsg_t tempCmd;
    uint8_t status = MT_FAIL;
    //uint8_t expectedCmd0 = cmdDesc->cmd0;
    uint8_t expectedCmd1 = cmdDesc->cmd1;
    msgQueue_t incomingMsg;
    tempCmd.attrs = NULL;

    for(uint8_t retries = 0; retries < MT_SRSP_RETRY_COUNT; retries++)
    {
        incomingMsg.event = 0xff;
        incomingMsg.msgPtr = NULL;
        usleep(MT_SRSP_SLEEP_TIMEOUT);
        mq_receive(mtSrspMq, (char*)&incomingMsg, sizeof(msgQueue_t), NULL);
        if(incomingMsg.event == MtEvent_RECEIVED_SRSP)
        {
            Mt_bufToMsg(&tempCmd, incomingMsg.msgPtr);
            if(incomingMsg.msgPtr)
            {
                free(incomingMsg.msgPtr);
                incomingMsg.msgPtr = NULL;
            }
            if(tempCmd.cmd1 == expectedCmd1)
            {
                cmdDesc->len = tempCmd.len;
                cmdDesc->attrs = tempCmd.attrs;
                status = MT_SUCCESS;
                break;
            }
            if(tempCmd.attrs)
            {
                free(tempCmd.attrs);
                tempCmd.attrs = NULL;
            }
        }
        if(incomingMsg.msgPtr)
        {
            free(incomingMsg.msgPtr);
            incomingMsg.msgPtr = NULL;
        }


        // not the msg we are looking for, put it back on the queue
        //usleep(MT_SRSP_SLEEP_TIMEOUT);
        //mq_send(*clientMq, (char*)&incomingMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);

    }
    return status;
}

void Mt_parseCmd(uint8_t *inCmd, int32_t cmdLen)
{
    mtMsg_t parsedCmd;

    parsedCmd.len = inCmd[0];
    parsedCmd.cmd0 = inCmd[1];
    parsedCmd.cmd1 = inCmd[2];

    if(parsedCmd.len > 0)
    {
        parsedCmd.attrs = &inCmd[3];
    }

    if((parsedCmd.cmd0 & MT_CMD_TYPE_MASK) == MT_CMD_SRSP)
    {
        UART_PRINT("[npiParse] Error: SRSP sent to Wrong queue\n\r");
    }
    else
    {
        switch(parsedCmd.cmd0 & MT_SUBSYSTEM_MASK)
        {
        case MT_SYS:
            MtSys_ProcessCmd(&parsedCmd);
            break;
        case MT_MAC:
            MtMac_ProcessCmd(&parsedCmd);
            break;
        case MT_UTIL:
            break;

        default:
            break;
        }
    }




}



static uint8_t mtCalcFCS(uint8_t *msg_ptr, uint8_t len)
{
    uint8_t x;
    uint8_t xorResult;

    xorResult = 0;

    for (x = 0; x < len; x++, msg_ptr++)
    {
        xorResult = xorResult ^ *msg_ptr;
    }

    return (xorResult);
}
