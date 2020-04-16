/******************************************************************************

 @file appHandler.c

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

/******************************************************************************
  Includes
*****************************************************************************/

#include "Utils/uart_term.h"
#include "pthread.h"
#include "mqueue.h"
#include "Common/commonDefs.h"
#include "collector.h"
#include "API_MAC/api_mac.h"
#include "config.h"
#include "appHandler.h"


#define EMBEDDED_GATEWAY

/******************************************************************************
 Typedefs
*****************************************************************************/



/******************************************************************************
 GLOBAL Variables
*****************************************************************************/


/*******************************************************************
 * LOCAL VARIABLES
 ********************************************************************/
static mqd_t *appHCliMq;

///*******************************************************************
// * LOCAL FUNCTIONS
// ********************************************************************/
//
//
///*!
// * @brief send a data confirm to the gateway
// * @param status - the status value to send
// */
//static void send_AppsrvTxDataCnf(int status)
//{
//#if EMBEDDED_GATEWAY
//    // return the list of connected device to the gateway task...
//    // dont implement
//
//
//#endif //EMBEDDED_GATEWAY
//}
//
//
///*!
// * @brief send a join confirm to the gateway
// */
//static void send_AppSrvJoinPermitCnf(int status)
//{
//#if EMBEDDED_GATEWAY
//    // return the list of connected device to the gateway task...
//    // dont implement
//
//
//#endif //EMBEDDED_GATEWAY
//}
//
///*!
// * @brief handle a data request from the gateway
// * @param pCONN - where the request came from
// * @param pIncommingMsg - the msg from the gateway
// */
//static void appsrv_processTxDataReq(struct appsrv_connection *pCONN,
//                                    struct mt_msg *pIncomingMsg)
//{
//#if EMBEDDED_GATEWAY
//    // return the list of connected device to the gateway task...
//    //implement
//
//
//#endif //EMBEDDED_GATEWAY
//}
//
///*!
// * @brief handle a join permit request from the gateway
// * @param pCONN - where the request came from
// * @param pIncommingMsg - the msg from the gateway
// */
//static void appsrv_processSetJoinPermitReq(struct appsrv_connection *pCONN,
//                                           struct mt_msg *pIncomingMsg)
//{
//
//#if EMBEDDED_GATEWAY
//    // return the list of connected device to the gateway task...
//    //implement
//
//
//#endif //EMBEDDED_GATEWAY
//}
//
///*!
// * @brief handle a getnetwork info request from the gateway
// * @param pCONN - where the request came from
// */
//static void appsrv_processGetNwkInfoReq(struct appsrv_connection *pCONN)
//{
//    struct mt_msg *pMsg;
//#if EMBEDDED_GATEWAY
//    // return the nwk information to the gateway task...
//    // dont implement
//
//
//#endif //EMBEDDED_GATEWAY
//}
//
///*!
// * @brief  Process incoming getDeviceArrayReq message
// *
// * @param pConn - the connection
// */
//static void appsrv_processGetDeviceArrayReq(struct appsrv_connection *pCONN)
//{
//
//#if EMBEDDED_GATEWAY
//    // return the list of connected device to the gateway task...
//    // dont implement
//
//
//#endif //EMBEDDED_GATEWAY
//}

/******************************************************************************
 Function Implementation
*****************************************************************************/

/*!
  Csf module calls this function to inform the user/appClient
  that the application has either started/restored the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_networkUpdate(bool restored, Llc_netInfo_t *networkInfo)
{
    nwk_t *pNwk;
    msgQueue_t queueElement;

    pNwk = (nwk_t*)malloc(sizeof(nwk_t));

    pNwk->channel = networkInfo->channel;
    pNwk->panId = networkInfo->devInfo.panID;
    pNwk->shortAddr = networkInfo->devInfo.shortAddress;
    memcpy(pNwk->extAddr, networkInfo->devInfo.extAddress, APIMAC_SADDR_EXT_LEN);
    //TODO: Remove sec or modify api to receive sec
    pNwk->security_enable = CONFIG_SECURE;
    pNwk->mode = networkInfo->fh;


    queueElement.event = GatewayEvent_NWK_UPDATE;
    queueElement.msgPtr = pNwk;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}

/*!

  Csf module calls this function to inform the user/appClient
  that a device has joined the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceUpdate(Llc_deviceListItem_t *pDevListItem)
{

    dev_t *pDev;
    pDev = (dev_t*) malloc(sizeof(dev_t));

    pDev->shortAddr = pDevListItem->devInfo.shortAddress;
    memcpy(pDev->extAddr, pDevListItem->devInfo.extAddress, APIMAC_SADDR_EXT_LEN);
    pDev->topic = NULL;
    pDev->rssi = 0;
    pDev->active = true;
    pDev->objectCount = 0;

    msgQueue_t queueElement;
    queueElement.event = GatewayEvent_DEV_UPDATE;
    queueElement.msgPtr =  pDev;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}

/*!
  Csf module calls this function to inform the user/appClient
  that the device has responded to the configuration request

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                               Smsgs_configRspMsg_t *pRspMsg)
{
    dev_t *pDev;
    pDev = (dev_t*) malloc(sizeof(dev_t));

    if(pSrcAddr->addrMode == 2)
    {
        pDev->shortAddr = pSrcAddr->addr.shortAddr;
    }
    else if(pSrcAddr->addrMode == 3)
    {
        pDev->shortAddr = 0xFFFF;
        memcpy(pDev->extAddr, pSrcAddr->addr.extAddr, APIMAC_SADDR_EXT_LEN);
    }
    pDev->rssi = (signed int) rssi;

    msgQueue_t queueElement;
    queueElement.event = GatewayEvent_DEV_CNF_UPDATE;
    queueElement.msgPtr = pDev;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}
/*!
  Csf module calls this function to inform the user/appClient
  that a device is no longer active in the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceNotActiveUpdate(ApiMac_deviceDescriptor_t *pDevInfo,
                                  bool timeout)
{
    dev_t *pDev;
    pDev = (dev_t*) malloc(sizeof(dev_t));

    pDev->shortAddr = pDevInfo->shortAddress;

    memcpy(pDev->extAddr, pDevInfo->extAddress, APIMAC_SADDR_EXT_LEN);
    pDev->objectCount = 0;
    pDev->rssi = 0;
    pDev->topic = NULL;
    pDev->active = false;

    msgQueue_t queueElement;
    queueElement.event = GatewayEvent_DEV_NOT_ACTIVE;
    queueElement.msgPtr = pDev;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}

/*!
  Csf module calls this function to inform the user/appClient
  of the reported sensor data from a network device

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceSensorDataUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_sensorMsg_t *pSensorMsg)
{
    dev_t *pDev;
    uint16_t objIdx = 0;
    pDev = (dev_t*) malloc(sizeof(dev_t));

    if(pSrcAddr->addrMode == ApiMac_addrType_short)
    {
        pDev->shortAddr = pSrcAddr->addr.shortAddr;

    }
    else if(pSrcAddr->addrMode == ApiMac_addrType_extended)
    {
        pDev->shortAddr = 0xFFFF;
    }
    memcpy(pDev->extAddr, pSensorMsg->extAddress, APIMAC_SADDR_EXT_LEN);
    pDev->rssi = (signed int) rssi;
    pDev->object[0].typeId = LIGHT_TYPE_ID;//light type
    pDev->object[0].sensorVal = 0.0;
    pDev->object[0].unit[0] = '\0';
    pDev->object[0].type[0] = '\0';
    pDev->objectCount = 0;
    pDev->active = true;

    if(pSensorMsg->frameControl & Smsgs_dataFields_tempSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = TEMP_TYPE_ID;//temp type
        pDev->object[objIdx].sensorVal = (pSensorMsg->tempSensor.ambienceTemp);
        strcpy(pDev->object[objIdx].unit, "C");
        strcpy(pDev->object[objIdx].type, TEMP_TYPE);
    }
    if(pSensorMsg->frameControl & Smsgs_dataFields_lightSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = LIGHT_TYPE_ID;//light type
        pDev->object[objIdx].sensorVal = (pSensorMsg->lightSensor.rawData);
        strcpy(pDev->object[objIdx].unit, "Lumen");
        strcpy(pDev->object[objIdx].type, LIGHT_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_humiditySensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = HUM_TYPE_ID;//hum type
        pDev->object[objIdx].sensorVal = (pSensorMsg->humiditySensor.humidity);
        strcpy(pDev->object[objIdx].unit,"%");
        strcpy(pDev->object[objIdx].type, HUM_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_pressureSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = GEN_SENSOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->pressureSensor.pressureValue);
        strcpy(pDev->object[objIdx].unit,"P");
        strcpy(pDev->object[objIdx].type, PRESS_TYPE);

        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = TEMP_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->pressureSensor.tempValue);
        strcpy(pDev->object[objIdx].unit,"C");
        strcpy(pDev->object[objIdx].type, TEMP_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_motionSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = PRESENSE_TYPE_ID;
        pDev->object[objIdx].sensorVal = (int)(pSensorMsg->motionSensor.isMotion);
        strcpy(pDev->object[objIdx].unit, "-");
        strcpy(pDev->object[objIdx].type, MOTION_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_batterySensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = GEN_SENSOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->batterySensor.voltageValue);
        strcpy(pDev->object[objIdx].unit, "V");
        strcpy(pDev->object[objIdx].type, VOLTAGE_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_hallEffectSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = GEN_SENSOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->hallEffectSensor.isOpen);
        strcpy(pDev->object[objIdx].unit,"-");
        strcpy(pDev->object[objIdx].type, HALL_OPEN_TYPE);

        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = GEN_SENSOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->hallEffectSensor.isTampered);
        strcpy(pDev->object[objIdx].unit,"-");
        strcpy(pDev->object[objIdx].type, HALL_TMPR_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_fanSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = ACTUATOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->fanSensor.fanSpeed);
        strcpy(pDev->object[objIdx].unit,"%");
        strcpy(pDev->object[objIdx].type, FAN_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_doorLockSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = ACTUATOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (pSensorMsg->doorLockSensor.isLocked);
        strcpy(pDev->object[objIdx].unit, "-");
        strcpy(pDev->object[objIdx].type, DOOR_LOCK_TYPE);
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_waterleakSensor)
    {
        objIdx = pDev->objectCount++;
        pDev->object[objIdx].typeId = GEN_SENSOR_TYPE_ID;
        pDev->object[objIdx].sensorVal = (int)(pSensorMsg->waterleakSensor.status);
        strcpy(pDev->object[objIdx].unit, "Status");
        strcpy(pDev->object[objIdx].type, WATR_LEAK_TYPE);
    }


    msgQueue_t queueElement;
    queueElement.event = GatewayEvent_SENSOR_DATA_UPDATE;
    queueElement.msgPtr = pDev;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}

/*!
  TBD

  Public function defined in appsrv_Collector.h
*/
void appsrv_stateChangeUpdate(Cllc_states_t state)
{
    nwk_t *pNwk;
    pNwk = (nwk_t*) malloc(sizeof(nwk_t));
    pNwk->state = state;

    msgQueue_t queueElement;
    queueElement.event = GatewayEvent_NWK_STATE_CHANGE;
    queueElement.msgPtr = pNwk;
    mq_send(*appHCliMq, (char*) &queueElement, sizeof(msgQueue_t), 0);
}


void appCliMqReg(mqd_t *pAppCliMq)
{
     appHCliMq = pAppCliMq;

}
