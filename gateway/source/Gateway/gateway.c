/******************************************************************************

 @file gateway.c

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <Common/commonDefs.h>
#include <Utils/util.h>
#include <Board.h>
#include <Utils/uart_term.h>
#include <CloudService/cloud_service.h>
#include <NPI/npi.h>
#include <Collector/collector.h>
#include "gtwayJson.h"
#include "provisioning.h"
#include "gateway.h"



//*****************************************************************************
//                 Banner VARIABLES
//*****************************************************************************
char lineBreak[]                = "\n\r";

#define APPLICATION_NAME        "TI-15.4 Stack Sensor to Cloud Gateway"
#define APPLICATION_VERSION     "1.0"
#define DEF_TIME                "Mon Jan 01 00:00:00 2018"

#define SPAWN_TASK_PRIORITY     9

void gatewayStartSlTask(void);
int listDevUpdate(dev_t *newDev);


static mqd_t gatewayMq;
static mqd_t gatewayCollectorMq;
static mqd_t gatewayCloudMq;
SlWlanSecParams_t SecurityParams = { 0 };
bool ntpStarted = false;
char *currentTimeStr;
static nwk_t nwkInfo;
static dev_t devList[MAX_NUM_OF_DEVICES];

void gatewayInit()
{
    UART_Handle tUartHndl;
    mq_attr attr;
    unsigned mode = 0;

    // create mqueue for gateway
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 19;
    attr.mq_msgsize = sizeof(msgQueue_t);
    gatewayMq = mq_open(GATEWAY_MQ, O_CREAT, mode, &attr);

    nwkInfo.mode = true;
    nwkInfo.security_enable = false;
    nwkInfo.state = Cllc_states_joiningNotAllowed;
    nwkInfo.devCount = 0;

    Board_initSPI();
    /* Configure the UART                                                     */
    npiInit(NPI_MQ);
    tUartHndl = InitTerm();
    /* remove uart receive from LPDS dependency                               */
    UART_control(tUartHndl, UART_CMD_RXDISABLE, NULL);

    gatewayStartSlTask();

    /* Run MQTT Main Thread (it will open the Client and Server)          */

    provisioningInit(GATEWAY_MQ);
    cloudServiceInit(CLOUDSERVICE_MQ);
    cloudServiceCliMqReg(GATEWAY_MQ);
    collectorInit(COLLECTOR_MQ);


    npiCliMqReg(COLLECTOR_MQ);
    collectorClientRegister(NPI_MQ, GATEWAY_MQ);

    gatewayCollectorMq = mq_open(COLLECTOR_MQ, O_WRONLY);
    gatewayCloudMq = mq_open(CLOUDSERVICE_MQ, O_WRONLY);

}

void gatewayStartSlTask()
{
    pthread_t spawn_thread = (pthread_t) NULL;
    pthread_attr_t pAttrs_spawn;
    struct sched_param priParam;
    int32_t retc = 0;
    /* Create the sl_Task                                                     */
    pthread_attr_init(&pAttrs_spawn);
    priParam.sched_priority = SL_TASK_PRI;
    retc = pthread_attr_setschedparam(&pAttrs_spawn, &priParam);
    retc |= pthread_attr_setstacksize(&pAttrs_spawn, TASKSTACKSIZE);
    retc |= pthread_attr_setdetachstate(&pAttrs_spawn, PTHREAD_CREATE_DETACHED);

    retc = pthread_create(&spawn_thread, &pAttrs_spawn, sl_Task, NULL);
    if (retc != 0)
    {
        UART_PRINT("[Gateway Task] could not create simplelink task\n\r");
        while (1);
    }
}


void printExtAddr(uint8_t *extA)
{
    uint32_t loBytes = Util_buildUint32(*extA++, *extA++,*extA++,*extA++);
    uint32_t hiBytes = Util_buildUint32(*extA++, *extA++,*extA++,*extA);
    UART_PRINT("0x%08X%08X", hiBytes, loBytes);
}

bool extAddrCmp(uint8_t *extA, uint8_t *extB)
{
    bool extCmp = (memcmp(extA, extB, APIMAC_SADDR_EXT_LEN) == 0);
    return extCmp;
}

int devSearchExt(uint8_t *pAddr)
{
    for(int i = 0; i < nwkInfo.devCount; i++)
    {
        if(extAddrCmp(devList[i].extAddr, pAddr))
        {
            printExtAddr(pAddr);
            UART_PRINT("[Gateway Task] Extended address found in device list\n\r");
            return i;
        }
    }
    UART_PRINT("[Gateway Task] Extended address NOT found in device list\n\r");
    return -1;
}
int devSearchShort(uint16_t sAddr)
{
    for(int i = 0; i < nwkInfo.devCount; i++)
    {
        if(devList[i].shortAddr == sAddr)
        {
            return i;
        }
    }
    return -1;
}

int objSearch(int index, int typeId)
{
    for(int i = 0; i < devList[index].objectCount; i++)
    {
        if(devList[index].object[i].typeId == typeId)
        {
            return i;
        }
    }
    return -1;
}

void *gatewayMainThread(void *pvParameters)
{

    msgQueue_t incomingMsg;
    msgQueue_t queueElementSend;

    char *tmpBuff;

    nwk_t *tempNwk;
    dev_t *tempDev;

    deviceCmd_t *tempDevCmd;
    permitJoinCmd_t *tempPermitJoinCmd;
    time_t ts;

    gatewayInit();
    currentTimeStr = DEF_TIME;

    for (;;)
    {
        incomingMsg.event = CommonEvent_INVALID_EVENT;
        incomingMsg.msgPtr = NULL;
        incomingMsg.msgPtrLen = 0;
        /* waiting for signals                                                */
        mq_receive(gatewayMq, (char*) &incomingMsg, sizeof(msgQueue_t), NULL);

        //if(ntpStarted)
        {
            ts = time(NULL);
            currentTimeStr = ctime(&ts);
        }

        if(incomingMsg.event <= GatewayEvent_MAX)
        {
            provisioning_processQMsg(&incomingMsg);
            continue;
        }

        switch (incomingMsg.event)
        {
        case GatewayEvent_INIT_COMPLETE:
            queueElementSend.event = CollectorEvent_START_COP;
            queueElementSend.msgPtr = NULL;
            queueElementSend.msgPtrLen = 0;
            mq_send(gatewayCollectorMq, (char*)&queueElementSend, sizeof(msgQueue_t), MQ_LOW_PRIOR);
        break;
        case CommonEvent_WLAN_CONNECTED:
            /* Use NTP to get the current time, as needed for SSL authentication */
            startNTP();
            ntpStarted = true;
            ts = time(NULL);
            currentTimeStr = ctime(&ts);
            queueElementSend.event = CommonEvent_WLAN_CONNECTED;
            queueElementSend.msgPtr = NULL;
            queueElementSend.msgPtrLen = 0;
            mq_send(gatewayCloudMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);
            queueElementSend.event = CollectorEvent_START_COP;
            queueElementSend.msgPtr = NULL;
            queueElementSend.msgPtrLen = 0;
            mq_send(gatewayCollectorMq, (char*)&queueElementSend, sizeof(msgQueue_t), MQ_LOW_PRIOR);
            break;
        case CommonEvent_WLAN_DISCONNECTED:
            queueElementSend.event = CommonEvent_WLAN_DISCONNECTED;
            queueElementSend.msgPtr = NULL;
            queueElementSend.msgPtrLen = 0;
            mq_send(gatewayCloudMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);
            break;
        case GatewayEvent_NWK_UPDATE:
        {
            tempNwk = (nwk_t*) incomingMsg.msgPtr;
            UART_PRINT("\n\r%s\r[Gateway Task] GatewayEvent_NWK_UPDATE Received\n\r", currentTimeStr);
            UART_PRINT("[Gateway Task] Data: Channel: %d, PanID: %04x, shortAddr:%04x, extAddr:",
                       tempNwk->channel, tempNwk->panId, tempNwk->shortAddr);
            printExtAddr(tempNwk->extAddr);
            UART_PRINT("\n\r");

            snprintf(nwkInfo.name, 7, "0x%04x", tempNwk->shortAddr);
            nwkInfo.security_enable = tempNwk->security_enable;
            nwkInfo.channel = tempNwk->channel;
            nwkInfo.panId = tempNwk->panId;
            nwkInfo.shortAddr = tempNwk->shortAddr;
            memcpy(nwkInfo.extAddr, tempNwk->extAddr, APIMAC_SADDR_EXT_LEN);
            nwkInfo.mode = tempNwk->mode;
            uint8_t isValid = 0;
            for(uint8_t extIdx = 0; extIdx < 8; extIdx++)
            {
                if(nwkInfo.extAddr[extIdx] != 0)
                {
                    isValid = 1;
                    break;
                }
            }
            if(isValid)
            {
                tmpBuff = formatNwkJson(&nwkInfo, devList);

                 //SEND DATA TO CLOUD SERVICE TASK
                queueElementSend.event = CloudServiceEvt_NWK_UPDATE;
                queueElementSend.msgPtr = tmpBuff;
                queueElementSend.msgPtrLen = strlen(tmpBuff) + 1;
                mq_send(gatewayCloudMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);
            }
        }


            break;
        case GatewayEvent_SENSOR_DATA_UPDATE:
        case GatewayEvent_DEV_NOT_ACTIVE:
        case GatewayEvent_DEV_UPDATE:
            tempDev = (dev_t*) incomingMsg.msgPtr;

            UART_PRINT("\n\r%s\r[Gateway Task] GatewayEvent_DEV_UPDATE Received\n\r", currentTimeStr);
            UART_PRINT("[Gateway Task] Data: ShortAddr:%d, EXTAddr:", tempDev->shortAddr);
            printExtAddr(tempDev->extAddr);
            UART_PRINT("  ObjectCount: %d", tempDev->objectCount);

            for(uint8_t objIdx = 0; objIdx < tempDev->objectCount; objIdx++)
            {
                UART_PRINT("\n\r[Gateway Task] ObjectData: Type:%s, TypeId:%d, SensotVal:%d, Unit:%s",
                            tempDev->object[objIdx].type, tempDev->object[objIdx].typeId,
                             tempDev->object[objIdx].sensorVal, tempDev->object[objIdx].unit);
            }
            UART_PRINT("\n\r");
            sprintf(tempDev->name, "0x%04x", tempDev->shortAddr);

            tmpBuff =  formatDevJson(&devList[listDevUpdate(tempDev)], currentTimeStr);
            //SEND DATA TO CLOUD TASK
            queueElementSend.event = CloudServiceEvt_DEV_UPDATE;
            queueElementSend.msgPtr = tmpBuff;
            queueElementSend.msgPtrLen = strlen(tmpBuff) + 1;
            mq_send(gatewayCloudMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);
            break;

        case GatewayEvent_DEV_CNF_UPDATE:
            tempDev = (dev_t*) incomingMsg.msgPtr;
            UART_PRINT("\n\r%s\r[Gateway Task] GatewayEvent_DEV_CNF_UPDATE Received\n\r", currentTimeStr);
            UART_PRINT("[Gateway Task] Data: ShortAddr: 0x%04X", tempDev->shortAddr);
            UART_PRINT("\n\r");

            break;

        case GatewayEvent_NWK_STATE_CHANGE:
        {
            tempNwk = (nwk_t*) incomingMsg.msgPtr;
            UART_PRINT("\n\r%s\r[Gateway Task] GatewayEvent_NWK_STATE_CHANGE Received\n\r",currentTimeStr);
            nwkInfo.state = tempNwk->state;
            UART_PRINT("[Gateway Task] New State:      0x%02X\n\r", tempNwk->state);
            uint8_t isValid = 0;
            for(uint8_t extIdx = 0; extIdx < 8; extIdx++)
            {
                if(nwkInfo.extAddr[extIdx] != 0)
                {
                    isValid = 1;
                    break;
                }
            }
            if(isValid)
            {
                tmpBuff = formatNwkJson(&nwkInfo, devList);
                //SEND DATA TO CLOUD TASK
                queueElementSend.event = CloudServiceEvt_STATE_CNF_EVT;
                queueElementSend.msgPtr = tmpBuff;
                queueElementSend.msgPtrLen = strlen(tmpBuff) + 1;

                mq_send(gatewayCloudMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);
            }
        }
            break;

        case GatewayEvent_PERMIT_JOIN:
            tempPermitJoinCmd= (permitJoinCmd_t*) malloc(sizeof(permitJoinCmd_t));
            if(((deviceCmd_t*)incomingMsg.msgPtr)->data)
            {
                tempPermitJoinCmd->permitJoin = true;
            }
            else
            {
                tempPermitJoinCmd->permitJoin = false;
            }

            queueElementSend.event = CollectorEvent_PERMIT_JOIN;
            queueElementSend.msgPtr = tempPermitJoinCmd;
            mq_send(gatewayCollectorMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);

            break;

        case GatewayEvent_DEVICE_CMD:
            tempDevCmd = (deviceCmd_t*) malloc(sizeof(deviceCmd_t));

            tempDevCmd->shortAddr = ((deviceCmd_t*)incomingMsg.msgPtr)->shortAddr;
            if(tempDevCmd->shortAddr == 0xFFFF)
            {
                int devIdx = devSearchExt(((deviceCmd_t*)incomingMsg.msgPtr)->extAddr);
                if(devIdx != -1)
                {
                    tempDevCmd->shortAddr = devList[devIdx].shortAddr;
                }
            }
            tempDevCmd->data = ((deviceCmd_t*)incomingMsg.msgPtr)->data;
            tempDevCmd->cmdType = ((deviceCmd_t*)incomingMsg.msgPtr)->cmdType;
            queueElementSend.event = CollectorEvent_SEND_SNSR_CMD;
            queueElementSend.msgPtr = tempDevCmd;
            mq_send(gatewayCollectorMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);

            break;
        default:

            break;
        }

        if(incomingMsg.msgPtr)
        {
            free(incomingMsg.msgPtr);
        }
    }
}

int listDevUpdate(dev_t *newDev)
{
    int devIdx = devSearchShort(newDev->shortAddr);
    if(devIdx != -1)
    {
        devList[devIdx].active = newDev->active;
        memcpy(devList[devIdx].object, newDev->object, sizeof(smartObject_t)*newDev->objectCount);
        devList[devIdx].objectCount = newDev->objectCount;
        devList[devIdx].rssi = newDev->rssi;
        return devIdx;
    }
    devIdx = devSearchExt(newDev->extAddr);
    if(devIdx != -1)
    {
        memcpy(&devList[devIdx], newDev, sizeof(dev_t));
        return devIdx;
    }
    devIdx = nwkInfo.devCount++;
    memcpy(&devList[devIdx], newDev, sizeof(dev_t));
    return devIdx;
}

