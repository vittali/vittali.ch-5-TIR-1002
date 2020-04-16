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
 *
 */

/*****************************************************************************

 Application Name     -   MQTT Client
 Application Overview -   The device is running a MQTT client which is
                           connected to the online broker. The Red LED on the
                           device can be controlled from a web client by
                           publishing msg on appropriate topics. Similarly,
                           message are being published on pre-configured topics
                           every second.

 Application Details  - Refer to 'MQTT Client' README.html

 *****************************************************************************/
//*****************************************************************************
//
//! \addtogroup mqtt_server
//! @{
//
//*****************************************************************************
/* Standard includes                                                          */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>

/* Hardware includes                                                          */
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>

/* Driverlib includes                                                         */
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/timer.h>

/* TI-Driver includes                                                         */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

/* Simplelink includes                                                        */
#include <ti/drivers/net/wifi/simplelink.h>

/* MQTT Library includes                                                      */
//#include <ti/net/mqtt/mqtt_client.h>
#include <ti/net/mqtt/mqttclient.h>

//#include <ti/net/mqtt/mqtt_server.h>
#include <ti/net/mqtt/mqttserver.h>

/* Common interface includes                                                  */
#include <Utils/uart_term.h>
#include <Common/commonDefs.h>

/* Application includes                                                       */
#include <Board.h>
#include <CloudService/cloudJson.h>
#include "cloudServiceIBM.h"

//*****************************************************************************
//                          LOCAL DEFINES
//*****************************************************************************

#define CLIENT_INIT_STATE       (0x01)
#define SERVER_INIT_STATE       (0x02)
#define MQTT_INIT_STATE         (0x04)


/* Operate Lib in MQTT 3.1 mode.                                              */
#define MQTT_3_1_1              false
#define MQTT_3_1                true

#define WILL_TOPIC              "Client"
#define WILL_MSG                "Client Stopped"
#define WILL_QOS                MQTT_QOS_0
#define WILL_RETAIN             false


#define PORT_NUMBER             1883
#define SECURED_PORT_NUMBER     8883
#define LOOPBACK_PORT           1882

/* Clean session flag                                                         */
#define CLEAN_SESSION           true

/* Retain Flag. Used in publish message.                                      */
#define RETAIN                  1

/* Defining Number of topics                                                  */
#define SUB_TOPIC_COUNT         1

/* Defining Subscription Topic Values                                         */
#define TOPIC0                  "iot-2/type/+/id/+/cmd/+/fmt/json"

/* Spawn task priority and Task and Thread Stack Size                         */

#define RXTASKSIZE              4096

/* secured client requires time configuration, in order to verify server      */
/* certifcate validiy (date).                                                 */

/* Day of month (DD format) range 1-31                                        */
#define DAY                     8
/* Month (MM format) in the range of 1-12                                     */
#define MONTH                   2
/* Year (YYYY format)                                                         */
#define YEAR                    2017
/* Hours in the range of 0-23                                                 */
#define HOUR                    12
/* Minutes in the range of 0-59                                               */
#define MINUTES                 35
/* Seconds in the range of 0-59                                               */
#define SEC                     00

#define TOPICS_NUM 3
#define MAX_TOPIC_LEN 100
char topicList[TOPICS_NUM][MAX_TOPIC_LEN];
char *topicStrings[TOPICS_NUM] = {"nwkUpdate", "permitJoinCnf", "deviceUpdate"};
typedef enum
{
    MqttTopicIBM_NWK_UPDT,
    MqttTopicIBM_STATE_UPDT,
    MqttTopicIBM_DEV_UPDT,
}MqttTopic;

const char topicBaseStr[] = "iot-2/type/%s/id/%s/evt/%s/fmt/json";
const char clienIdBaseStr[] = "g:%s:%s:%s";
const char srvrBaseStr[] = "%s.messaging.internetofthings.ibmcloud.com";
char topicList[TOPICS_NUM][MAX_TOPIC_LEN];
#define CLIENTID_MAX_LEN 32
#define IBM_SRVR_ADDR_MAC_LEN 70
char ClientId[CLIENTID_MAX_LEN];
char IBMServerAddress[IBM_SRVR_ADDR_MAC_LEN];


const char *incomingDevCmds[] =
{
 "updateFanSpeed",
 "sendToggle",
 "updateDoorLock",
 "leakBuzzOff"
};
const CmdTypes incomingDevCmdTypes[] =
{
 CmdType_FAN_DATA,
 CmdType_LED_DATA,
 CmdType_DOORLOCK_DATA,
 CmdType_LEAK_DATA
};

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************

static void cloudIBM_stop();
int32_t cloudIBM_startMqttCli();
static void cloudIBM_mqttClientCb(int32_t event , void * metaData , uint32_t metaDateLen , void *data , uint32_t dataLen);


//*****************************************************************************
//                 GLOBAL VARIABLES
//*****************************************************************************




static MQTTClient_Handle gMqttClient;
MQTTClient_Params  cloudIBM_mqttParams;

/* Receive task handle                                                        */
pthread_t g_rx_task_hndl = (pthread_t) NULL;

/* Client ID,User Name and Password                                           */
#define CLNT_USR_PWD

/* Subscription topics and qos values                                         */

MQTTClient_SubscribeParams subInfo[SUB_TOPIC_COUNT] = {{
    TOPIC0,
    NULL,
    MQTT_QOS_0,
    NULL}
};


/* Message Queue                                                              */
mqd_t cloudSrvrMq;
mqd_t disconnectedMq;
mqd_t *gatewayMq;
CloudIBM_Info_t *cloudConnectionInfo;
bool wlanConnected = false;
bool ibmCloudConnected = false;

char *IBMMqttUsername = "use-token-auth";



/* enables secured client                                                     */
//#define SECURE_CLIENT

#ifdef  SECURE_CLIENT

#define CLIENT_NUM_SECURE_FILES 4
char *Mqtt_Client_secure_files[CLIENT_NUM_SECURE_FILES] = {"/cert/index4.key.pem","/cert/index4.cert.pem","/cert/ca.cert.pem",NULL};

/* Initialization structure to be used with sl_ExtMqtt_Init API. In order to  */
/* use secured socket method, cipher, n_files and secure_files must be        */
/* configured. certifcates also must be programmed  ("ca-cert.pem").          */
MQTTClient_NetAppConnParams_t Mqtt_ClientCtx =
{
     MQTTCLIENT_NETCONN_SEC,
     SERVER_ADDRESS, //SERVER_ADDRESS,
     SECURED_PORT_NUMBER,//  PORT_NUMBER,
     SL_SO_SEC_METHOD_TLSV1,
     SL_SEC_MASK_SECURE_DEFAULT,
     0,
     NULL
};

void setTime()
{
    SlDateTime_t dateTime = {0};
    dateTime.tm_day = (uint32_t)DAY;
    dateTime.tm_mon = (uint32_t)MONTH;
    dateTime.tm_year = (uint32_t)YEAR;
    dateTime.tm_hour = (uint32_t)HOUR;
    dateTime.tm_min = (uint32_t)MINUTES;
    dateTime.tm_sec = (uint32_t)SEC;
    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME, sizeof(SlDateTime_t), (uint8_t *)(&dateTime));
}
#else


#endif

MQTTClient_Will will_param =
{
    WILL_TOPIC,
    WILL_MSG,
    WILL_QOS,
    WILL_RETAIN
};



void *cloudIBM_MqttClientThread(void * pvParameters)
{
    //msgQueue_t queueElement;

    MQTTClient_run((MQTTClient_Handle)pvParameters);

    //queueElement.event = CommonEvent_CLOUD_SERV_DISCONNECTED;
    //queueElement.msgPtr = NULL;
    UART_PRINT("[Cloud Service] MQTT CLIENT disconnected, exit pthread\n\r");
    /* write message indicating disconnect Broker message.                    */
    //mq_send(cloudSrvrMq, (char*) &queueElement, sizeof(msgQueue_t), 0);

    pthread_exit(0);

    return NULL;
}

/*!
 Register the message queue

 Public function defined in cloudService.h
 */
void cloudIBM_registerCliMq(mqd_t *pGatewayMq)
{
    gatewayMq = pGatewayMq;
}

/*!
 Initialize the IBM client

 Public function defined in cloudService.h
 */
void cloudIBM_init(const char *cloudSrvrMqName)
{
    mq_attr attr;
    unsigned mode = 0;

    attr.mq_maxmsg = 20;
    attr.mq_msgsize = sizeof(msgQueue_t);
    disconnectedMq = mq_open("disconnectedMQ", O_CREAT | O_NONBLOCK, mode, &attr);
    cloudSrvrMq = mq_open(cloudSrvrMqName, O_WRONLY );//| O_NONBLOCK);

    if (cloudSrvrMq == NULL)
    {
        UART_PRINT("[Cloud Service] MQTT Message Queue create fail\n\r");
    }
}

/*!
 Register the cloud Info

 Public function defined in cloudService.h
 */
void cloudIBM_registerCloudInfo(CloudIBM_Info_t *cInfo)
{
    if(cloudConnectionInfo)
    {
        free(cloudConnectionInfo->devId);
        free(cloudConnectionInfo->devType);
        free(cloudConnectionInfo->orgId);
        free(cloudConnectionInfo->securityToken);
        free(cloudConnectionInfo);
    }
    cloudConnectionInfo = cInfo;
}

/*!
 Start the cloud client

 Public function defined in cloudService.h
 */
void cloudIBM_start()
{
    if(wlanConnected && !ibmCloudConnected && cloudConnectionInfo)
    {
        ibmCloudConnected = (cloudIBM_startMqttCli() == 0);
        if(ibmCloudConnected)
        {
            msgQueue_t mqEvt = {CommonEvent_CLOUD_SERV_CONNECTED, NULL, 0};
            mq_send(cloudSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
        }
    }
}


/*!
 Start the IBM mqtt client

 Public function defined in cloudService.h
 */
int32_t cloudIBM_startMqttCli()
{
    int32_t lRetVal = -1;
    int32_t threadArg = 100;
    pthread_attr_t pAttrs;
    struct sched_param priParam;

    UART_PRINT("\n\r");
    for(uint16_t topicIdx = 0; topicIdx < TOPICS_NUM; topicIdx++)
    {
        snprintf(topicList[topicIdx], MAX_TOPIC_LEN - 1, topicBaseStr,
                 cloudConnectionInfo->devType,
                 cloudConnectionInfo->devId,
                 topicStrings[topicIdx]);
        UART_PRINT("\n\r[Cloud Service] TOPIC# %d: %s\n\r", topicIdx, topicList[topicIdx]);
    }

    snprintf(ClientId, CLIENTID_MAX_LEN,clienIdBaseStr,
             cloudConnectionInfo->orgId,
             cloudConnectionInfo->devType,
             cloudConnectionInfo->devId);

    snprintf(IBMServerAddress, IBM_SRVR_ADDR_MAC_LEN, srvrBaseStr, cloudConnectionInfo->orgId);
    UART_PRINT("[Cloud Service] CLIENT ID: %s\n\r", ClientId);
    UART_PRINT("[Cloud Service] SERVER ADDRESS: %s\n\n\r", IBMServerAddress);

    MQTTClient_ConnParams Mqtt_ClientCtx =
    {
        MQTTCLIENT_NETCONN_URL,
        IBMServerAddress,
        PORT_NUMBER, 0, 0, 0,
        NULL
    };

    /* Initialze MQTT client lib                                              */
    cloudIBM_mqttParams.clientId = ClientId;
    cloudIBM_mqttParams.connParams = &Mqtt_ClientCtx;
    cloudIBM_mqttParams.mqttMode31 = MQTT_3_1;
    cloudIBM_mqttParams.blockingSend = true;

    gMqttClient = MQTTClient_create(cloudIBM_mqttClientCb, &cloudIBM_mqttParams);

    if( gMqttClient == NULL )
    {
        /* lib initialization failed                                          */
        return -1;
    }

    /* Open Client Receive Thread start the receive task. Set priority and    */
    /* stack size attributes                                                  */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 2;
    lRetVal = pthread_attr_setschedparam(&pAttrs, &priParam);
    lRetVal |= pthread_attr_setstacksize(&pAttrs, RXTASKSIZE);
    lRetVal |= pthread_attr_setdetachstate(&pAttrs, PTHREAD_CREATE_DETACHED);
    lRetVal |= pthread_create(&g_rx_task_hndl, &pAttrs, cloudIBM_MqttClientThread, (void *) &threadArg);

    if (lRetVal != 0)
    {
        UART_PRINT("[Cloud Service] Client Thread Create Failed failed\n\r");
        return -1;
    }

/*setting date and time for secure client                                 */
#ifdef SECURE_CLIENT
    setTime();
#endif

/* setting will parameters                                                */
#ifdef WILL_PARAM
    MQTTClient_set(gMqttClient, MQTT_Client_WILL_PARAM, &will_param, sizeof(will_param));
#endif

#ifdef CLNT_USR_PWD
    /* Set username for client connection                                     */
    MQTTClient_set(gMqttClient, MQTTClient_USER_NAME, IBMMqttUsername, strlen((char*)IBMMqttUsername));

    /* Set password                                                           */
    MQTTClient_set(gMqttClient, MQTTClient_PASSWORD, cloudConnectionInfo->securityToken, strlen(cloudConnectionInfo->securityToken));
#endif
    /* Initiate MQTT Connect                                                  */
    {
#if CLEAN_SESSION == true
        bool clean = CLEAN_SESSION;
        lRetVal |= MQTTClient_set(gMqttClient, MQTTClient_CLEAN_CONNECT, &clean, sizeof(bool));
#endif

        lRetVal |= MQTTClient_connect(gMqttClient);

        if (lRetVal != 0)
        {
            /* lib initialization failed                                      */
            UART_PRINT("[Cloud Service] Connection to broker failed\n\r, ERRORNUM: %d", lRetVal);
            return (-1);
        }
        /* Subscribe to topics                                                */
        if (MQTTClient_subscribe(gMqttClient , subInfo, SUB_TOPIC_COUNT) < 0)
        {
            UART_PRINT("\n\r [Cloud Service] Subscription Error \n\r");
            MQTTClient_disconnect(gMqttClient);
            return -1;
        }
    }

    return 0;
}

/*!
 Stop the IBM Client

 Public function defined in cloudService.h
 */
void cloudIBM_stop()
{
    int16_t ret = -1;
    MQTTClient_UnsubscribeParams unsubInfo[SUB_TOPIC_COUNT];
    if(ibmCloudConnected)
    {
        for(uint8_t unsubIdx = 0; unsubIdx < SUB_TOPIC_COUNT; unsubIdx++ )
        {
            unsubInfo[unsubIdx].topic = subInfo[unsubIdx].topic;
        }

        ret = MQTTClient_unsubscribe(gMqttClient , unsubInfo, SUB_TOPIC_COUNT);

        if(ret != 0)
        {
            UART_PRINT("[Cloud Service] ERROR UNSUBSCRIBING FROM TOPICS\n\r");
        }
        /* exiting the Client library                                             */
        ret = MQTTClient_delete(gMqttClient);
        if(ret != 0)
        {
            UART_PRINT("[Cloud Service] ERROR DELETING CONTEXT\n\r");
        }
        ibmCloudConnected = false;
    }


}


void CloudIBM_handleWlanDisconnect()
{
    wlanConnected = false;
    cloudIBM_stop();
}
void CloudIBM_handleWlanConnect()
{
    wlanConnected = true;
    cloudIBM_start();
}
void CloudIBM_handleCloudDisconnect()
{
    cloudIBM_stop();
    cloudIBM_start();
}
void CloudIBM_handleCloudConnect(void)
{
    uint8_t timeout = 20;
    msgQueue_t queueElemRecv;
    // flush all queued msgs accumulated while cloud was disconnected
    while(mq_receive(disconnectedMq, (char*) &queueElemRecv, sizeof(msgQueue_t), NULL) > 0 && timeout--)
    {
        CloudIBM_handleGatewayEvt(&queueElemRecv);
        if(queueElemRecv.msgPtr)
        {
            free(queueElemRecv.msgPtr);
            queueElemRecv.msgPtr = NULL;
        }
    }
}
void CloudIBM_handleGatewayEvt(msgQueue_t *inEvtMsg)
{
    if(ibmCloudConnected)
    {
        char *tmpBuff;
        char *tmpTopic = NULL;
        tmpBuff = (char*) inEvtMsg->msgPtr;
        switch(inEvtMsg->event)
        {
        case CloudServiceEvt_NWK_UPDATE:
            tmpTopic = topicList[MqttTopicIBM_NWK_UPDT];
            break;

        case CloudServiceEvt_DEV_UPDATE:
            tmpTopic = topicList[MqttTopicIBM_DEV_UPDT];
            break;

        case CloudServiceEvt_STATE_CNF_EVT:
            tmpTopic = topicList[MqttTopicIBM_NWK_UPDT];
            MQTTClient_publish(gMqttClient, tmpTopic,
                                     strlen(tmpTopic),
                                     tmpBuff,
                                     strlen(tmpBuff),
                                     MQTT_QOS_0 | MQTT_PUBLISH_RETAIN );

            UART_PRINT("\n\r [Cloud Service] CC3200 Publishes the following message \n\r");
            UART_PRINT("\tTopic: %s\n\r", tmpTopic);
            UART_PRINT("\tData: %s\n\r", tmpBuff);
            tmpTopic = topicList[MqttTopicIBM_STATE_UPDT];
           break;
        }
        if(tmpTopic)
        {
            MQTTClient_publish(gMqttClient, tmpTopic,
                                     strlen(tmpTopic),
                                     tmpBuff,
                                     strlen(tmpBuff),
                                     MQTT_QOS_0 | MQTT_PUBLISH_RETAIN );

            UART_PRINT("\n\r [Cloud Service] CC3200 Publishes the following message \n\r");
            UART_PRINT("\tTopic: %s\n\r", tmpTopic);
            UART_PRINT("\tData: %s\n\r", tmpBuff);
        }
    }
    else
    {
        char *buf = (char*)malloc(inEvtMsg->msgPtrLen);
        memcpy(buf, inEvtMsg->msgPtr, inEvtMsg->msgPtrLen);
        msgQueue_t mqEvt = {inEvtMsg->event, buf, inEvtMsg->msgPtrLen};
        if(mq_send(disconnectedMq, (char*) &mqEvt, sizeof(msgQueue_t), 0) != 0)
        {
            free(buf);
        }
    }
}


/*!
 Parse the received message from the cloud

 Public function defined in cloudService.h
 */
void CloudIBM_handleCloudEvt(msgQueue_t *inEvtMsg)
{
    char *extractedJson;
    char *tmpBuff;
    deviceCmd_t *inCommand;
    msgQueue_t queueElementSend;
    inCommand = malloc(sizeof(deviceCmd_t));
    inCommand->cmdType =   0xFFFF;
    inCommand->shortAddr = 0x0000;
    tmpBuff = (char *)inEvtMsg->msgPtr;
    UART_PRINT(" [CS_IBM] InCmd: %s\n\r", tmpBuff);

    extractedJson = jsonParseIn(tmpBuff, "action");
    if(extractedJson)
    {
        UART_PRINT(" [CS_IBM] ExtractedJson: %s\n\r", extractedJson);
        queueElementSend.event = GatewayEvent_PERMIT_JOIN;
        inCommand->cmdType = CmdType_PERMIT_JOIN;
        if(strncmp(extractedJson, "open", 5) == 0)
        {
            inCommand->data = 1;
        }
        else if(strncmp(extractedJson, "close", 5) == 0)
        {
            inCommand->data = 0;
        }
        free(extractedJson);
    }

    extractedJson = jsonParseIn(tmpBuff, "dstAddr");
    if(!extractedJson)
    {
        extractedJson = jsonParseIn(tmpBuff, "ext_addr");
        inCommand->shortAddr = 0xFFFF;
    }


    if(extractedJson)
    {
        UART_PRINT(" [CS_IBM] ExtractedJson: %s\n\r", extractedJson);
        int shortAddr;
        inCommand->cmdType = CmdType_DEVICE_DATA;
        queueElementSend.event = GatewayEvent_DEVICE_CMD;
        if(inCommand->shortAddr == 0x0000)
        {
            sscanf(extractedJson, "%x", &shortAddr);
            inCommand->shortAddr = shortAddr;
        }
        else if(inCommand->shortAddr == 0xFFFF)
        {
            *(uint64_t*)inCommand->extAddr = strtoull (extractedJson, NULL, 16);
        }
        free(extractedJson);

        extractedJson = jsonParseIn(tmpBuff, "devActionType");
        if(extractedJson)
        {
            for(uint8_t cmds = 0; cmds < sizeof(incomingDevCmds); cmds++)
            {
                if(strncmp(extractedJson, incomingDevCmds[cmds], strlen(incomingDevCmds[cmds])) == 0)
                {
                    inCommand->cmdType = incomingDevCmdTypes[cmds];
                    break;
                }
            }
            free(extractedJson);
        }



        extractedJson = jsonParseIn(tmpBuff, "value");
        if(extractedJson)
        {
            inCommand->data = atoi(extractedJson);
            free(extractedJson);
        }
//        extractedJson = jsonParseIn(tmpBuff, "toggleLED");
//        if(extractedJson)
//        {
//            inCommand->cmdType = CmdType_LED_DATA;
//            free(extractedJson);
//        }
    }
    queueElementSend.msgPtr = inCommand;

    /* send message to gateway task for processing */
    if(mq_send(*gatewayMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0) != 0)
    {
        free(inCommand);
    }
}




//****************************************************************************
//
//! Callback in case of various event (for clients connection with remote
//! broker)
//!
//! \param[in]  app_hndl    - is the ctx handl for the connection
//! \param[in]  evt         - is a event occured
//! \param[in]  buff        - is the pointer to the buffer for data
//!                           (for this event)
//! \param[in]  len         - is the length of the buffer data
//!
//! return none
//
//****************************************************************************
void cloudIBM_mqttClientCb(int32_t event , void * metaData , uint32_t metaDateLen , void *data , uint32_t dataLen)
{
    switch((MQTTClient_EventCB)event)
    {
        case MQTTClient_OPERATION_CB_EVENT:
        {
            switch (((MQTTClient_OperationMetaDataCB*)metaData)->messageType)
            {
                case MQTTCLIENT_OPERATION_EVT_PUBACK:
                    UART_PRINT("PubAck:\n\r");
                    UART_PRINT("%s\n\r", data);
                    break;

                case MQTTCLIENT_OPERATION_SUBACK:
                    UART_PRINT("Sub Ack:\n\r");
                    break;
                case MQTTCLIENT_OPERATION_UNSUBACK:
                    UART_PRINT("UnSub Ack \n\r");
                    UART_PRINT("%s\n\r", data);
                    break;

                default:
                    break;
            }
        }
            break;
        case MQTTClient_RECV_CB_EVENT:
        {
            MQTTClient_RecvMetaDataCB *recvMetaData =  (MQTTClient_RecvMetaDataCB *)metaData;
            uint32_t bufSizeReqd = 0;

            char *pubBuff = NULL;

            bufSizeReqd += dataLen + 1;
            pubBuff = (char *) malloc(bufSizeReqd);

            if (pubBuff == NULL)
            {
                UART_PRINT("malloc failed: recv_cb\n\r");
                return;
            }

            memcpy((void*) (pubBuff), (const void*) data, dataLen);
            memset((void*) (pubBuff + dataLen), '\0', 1);

            UART_PRINT("\n\rMsg Recvd. by client\n\r");
            UART_PRINT("TOPIC: %s\n\r", recvMetaData->topic);
            UART_PRINT("PAYLOAD: %s\n\r", pubBuff);
            UART_PRINT("QOS: %d\n\r", recvMetaData->qos);

            if (recvMetaData->retain)
            {
                UART_PRINT("Retained\n\r");
            }

            if (recvMetaData->dup)
            {
                UART_PRINT("Duplicate\n\r");
            }

            msgQueue_t mqEvt = {CloudServiceEvt_CLOUD_IN_MQTT, pubBuff, bufSizeReqd};
            mq_send(cloudSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
        }
            break;
        case MQTTClient_DISCONNECT_CB_EVENT:
        {
            msgQueue_t mqEvt = {CommonEvent_CLOUD_SERV_DISCONNECTED, NULL, 0};
            mq_send(cloudSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
            UART_PRINT("BRIDGE DISCONNECTION\n\r");
        }
            break;
    }

}



//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
