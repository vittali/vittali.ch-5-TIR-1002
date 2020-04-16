/******************************************************************************

 @file collector.c

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
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <Utils/util.h>
#include <Common/commonDefs.h>

#include <Utils/uart_term.h>
#include <NPI/npiParse.h>
#include <NPIcmds/mtSys.h>
#include <API_MAC/api_mac.h>
#include "config.h"
#include "LinkController/llc.h"
#include "LinkController/cllc.h"
#include "smsgs.h"
#include "csf.h"
#include "appHandler.h"
#include "collector.h"



/******************************************************************************
 Constants and definitions
 *****************************************************************************/

#if !defined(STATIC)
/* make local */
#define STATIC static
#endif

#if !defined(CONFIG_AUTO_START)
#if defined(AUTO_START)
#define CONFIG_AUTO_START 1
#else
#define CONFIG_AUTO_START 0
#endif
#endif

/* beacon order for non beacon network */
#define NON_BEACON_ORDER      15

/* MAC Indirect Persistent Timeout */
#define INDIRECT_PERSISTENT_TIME 750

/* default MSDU Handle rollover */
#define MSDU_HANDLE_MAX 0x3F

/* App marker in MSDU handle */
#define APP_MARKER_MSDU_HANDLE 0x80

/* App Config request marker for the MSDU handle */
#define APP_CONFIG_MSDU_HANDLE 0x40

/* Default configuration frame control */
#define CONFIG_FRAME_CONTROL (Smsgs_dataFields_tempSensor | \
                              Smsgs_dataFields_lightSensor | \
                              Smsgs_dataFields_humiditySensor | \
                              Smsgs_dataFields_msgStats | \
                              Smsgs_dataFields_configSettings | \
                              Smsgs_dataFields_pressureSensor | \
                              Smsgs_dataFields_motionSensor | \
                              Smsgs_dataFields_batterySensor | \
                              Smsgs_dataFields_hallEffectSensor | \
                              Smsgs_dataFields_fanSensor | \
                              Smsgs_dataFields_doorLockSensor | \
                              Smsgs_dataFields_waterleakSensor)

/* Default configuration reporting interval, in milliseconds */
#define CONFIG_REPORTING_INTERVAL 90000


/* Delay for config request retry in busy network */
#define CONFIG_DELAY 1000
#define CONFIG_RESPONSE_DELAY 3*CONFIG_DELAY
/* Tracking timeouts */
#define TRACKING_CNF_DELAY_TIME 2000 /* in milliseconds */
#define TRACKING_DELAY_TIME 60000 /* in milliseconds */
#define TRACKING_TIMEOUT_TIME (CONFIG_POLLING_INTERVAL * 2) /*in milliseconds*/

/* Assoc Table (CLLC) status settings */
#define ASSOC_CONFIG_SENT       0x0100    /* Config Req sent */
#define ASSOC_CONFIG_RSP        0x0200    /* Config Rsp received */
#define ASSOC_CONFIG_MASK       0x0300    /* Config mask */
#define ASSOC_TRACKING_SENT     0x1000    /* Tracking Req sent */
#define ASSOC_TRACKING_RSP      0x2000    /* Tracking Rsp received */
#define ASSOC_TRACKING_RETRY    0x4000    /* Tracking Req retried */
#define ASSOC_TRACKING_ERROR    0x8000    /* Tracking Req error */
#define ASSOC_TRACKING_MASK     0xF000    /* Tracking mask  */
/******************************************************************************
 Global variables
 *****************************************************************************/

/* Task pending events */
uint16_t Collector_events = 0;

 static bool collectorStarted = false;
 static bool copInitialized = false;

/*! Collector statistics */
Collector_statistics_t Collector_statistics;

/******************************************************************************
 Local variables
 *****************************************************************************/


/*! true if the device was restarted */
static bool restarted = false;

/*! CLLC State */
STATIC Cllc_states_t cllcState = Cllc_states_initWaiting;

/*! Device's PAN ID */
STATIC uint16_t devicePanId = 0xFFFF;

/*! Device's Outgoing MSDU Handle values */
STATIC uint8_t deviceTxMsduHandle = 0;

STATIC bool fhEnabled = false;

Llc_netInfo_t coordInfo;

/******************************************************************************
 Local function prototypes
 *****************************************************************************/
static void * collectorThread(void *pvParameters);

static void initializeClocks(void);
static void cllcStartedCB(Llc_netInfo_t *pStartedInfo);
static ApiMac_assocStatus_t cllcDeviceJoiningCB(
                ApiMac_deviceDescriptor_t *pDevInfo,
                ApiMac_capabilityInfo_t *pCapInfo);
static void cllcStateChangedCB(Cllc_states_t state);
static void dataCnfCB(ApiMac_mcpsDataCnf_t *pDataCnf);
static void dataIndCB(ApiMac_mcpsDataInd_t *pDataInd);
static void processStartEvent(void);
static void processConfigResponse(ApiMac_mcpsDataInd_t *pDataInd);
static void processTrackingResponse(ApiMac_mcpsDataInd_t *pDataInd);
static void processToggleLedResponse(ApiMac_mcpsDataInd_t *pDataInd);
static void processSensorData(ApiMac_mcpsDataInd_t *pDataInd);
static Cllc_associated_devices_t *findDevice(ApiMac_sAddr_t *pAddr);
static Cllc_associated_devices_t *findDeviceStatusBit(uint16_t mask, uint16_t statusBit);
static uint8_t getMsduHandle(Smsgs_cmdIds_t msgType);
static bool sendMsg(Smsgs_cmdIds_t type, uint16_t dstShortAddr, bool rxOnIdle,
                    uint16_t len,
                    uint8_t *pData);
static void generateConfigRequests(void);
static void generateTrackingRequests(void);
static void sendTrackingRequest(Cllc_associated_devices_t *pDev);
static void commStatusIndCB(ApiMac_mlmeCommStatusInd_t *pCommStatusInd);
static void pollIndCB(ApiMac_mlmePollInd_t *pPollInd);
static void processDataRetry(ApiMac_sAddr_t *pAddr);
static void processConfigRetry(void);

/******************************************************************************
 Callback tables
 *****************************************************************************/

/*! API MAC Callback table */
ApiMac_callbacks_t Collector_macCallbacks =
    {
      /*! Associate Indicated callback */
      NULL,
      /*! Associate Confirmation callback */
      NULL,
      /*! Disassociate Indication callback */
      NULL,
      /*! Disassociate Confirmation callback */
      NULL,
      /*! Beacon Notify Indication callback */
      NULL,
      /*! Orphan Indication callback */
      NULL,
      /*! Scan Confirmation callback */
      NULL,
      /*! Start Confirmation callback */
      NULL,
      /*! Sync Loss Indication callback */
      NULL,
      /*! Poll Confirm callback */
      NULL,
      /*! Comm Status Indication callback */
      commStatusIndCB,
      /*! Poll Indication Callback */
      pollIndCB,
      /*! Data Confirmation callback */
      dataCnfCB,
      /*! Data Indication callback */
      dataIndCB,
      /*! Purge Confirm callback */
      NULL,
      /*! WiSUN Async Indication callback */
      NULL,
      /*! WiSUN Async Confirmation callback */
      NULL,
      /*! Unprocessed message callback */
      NULL
    };

STATIC Cllc_callbacks_t cllcCallbacks =
    {
      /*! Coordinator Started Indication callback */
      cllcStartedCB,
      /*! Device joining callback */
      cllcDeviceJoiningCB,
      /*! The state has changed callback */
      cllcStateChangedCB
    };
void mtsysCoPResetInd(MtSys_resetInd_t *pResetInd);
static MtSys_callbacks_t mysysResetCbs =
{
 mtsysCoPResetInd
};


static mqd_t collectorMq = NULL;
mqd_t regGatewayMq = NULL;
pthread_t collThreadH = (pthread_t) NULL;

extern void triggerCollectorEvt(uint16_t evt)
{
    msgQueue_t mqEvt = {COLLECTOR_PROCESS_EVT, NULL, 0};
    Util_setEvent(&Collector_events, evt);
    mq_send(collectorMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}
extern void triggerCllcEvt(uint16_t evt)
{
    msgQueue_t mqEvt = {CLLC_PROCESS_EVT, NULL, 0};
    Util_setEvent(&Cllc_events, evt);
    mq_send(collectorMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}
extern void triggerCsfEvt(uint16_t evt)
{
    msgQueue_t mqEvt = {CSF_PROCESS_EVT, NULL, 0};
    Util_setEvent(&Csf_events, evt);
    mq_send(collectorMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}

void mtsysCoPResetInd(MtSys_resetInd_t *pResetInd)
{
    msgQueue_t initMsg;
    cllcState = Cllc_states_initWaiting;
    initMsg.event = CollectorEvent_INIT_COP;
    initMsg.msgPtr = NULL;
    initMsg.msgPtrLen = 0;
    mq_send(collectorMq, (char*)&initMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}

void Collector_initCop(void)
{
//    /* Initialize the collector's statistics */
    memset(&Collector_statistics, 0, sizeof(Collector_statistics_t));
//
//    /* Initialize the MAC */
    ApiMac_init(CONFIG_FH_ENABLE);
//
//    /* Initialize the Coordinator Logical Link Controller */
    Cllc_init(&Collector_macCallbacks, &cllcCallbacks);
//
//    /* Register the MAC Callbacks */
    ApiMac_registerCallbacks(&Collector_macCallbacks);
//
//    /* Initialize the platform specific functions */
    Csf_init();
//
//    /* Set the indirect persistent timeout */
    ApiMac_mlmeSetReqUint16(ApiMac_attribute_transactionPersistenceTime,
                            INDIRECT_PERSISTENT_TIME);
    ApiMac_mlmeSetReqUint8(ApiMac_attribute_phyTransmitPowerSigned,
                           (uint8_t)CONFIG_TRANSMIT_POWER);
//
//    /* Initialize the app clocks */
    initializeClocks();

    if(CONFIG_AUTO_START)
    {
        /* Start the device */
        triggerCollectorEvt(COLLECTOR_START_EVT);
    }
}

void collectorInit(const char *collectorMqName)
{

    pthread_attr_t pAttrs;
    mq_attr attr;
    unsigned mode = 0;
    struct sched_param priParam;
    attr.mq_maxmsg = 51;
    attr.mq_msgsize = sizeof(msgQueue_t);
    collectorMq = mq_open(collectorMqName, O_CREAT, mode, &attr);

    pthread_attr_init(&pAttrs);
    priParam.sched_priority = COLLECTOR_TASK_PRI;
    pthread_attr_setschedparam(&pAttrs, &priParam);
    pthread_attr_setstacksize(&pAttrs, TASKSTACKSIZE);
    pthread_attr_setdetachstate(&pAttrs, PTHREAD_CREATE_DETACHED);

    pthread_create(&collThreadH, &pAttrs, collectorThread, NULL);

    MtSys_RegisterCbs(&mysysResetCbs);
}
void collectorClientRegister(const char *npiMq, const char *gatewayMq)
{
    //regNpiMq = mq_open(npiMq, O_WRONLY);
    regGatewayMq = mq_open(gatewayMq, O_WRONLY);
    appCliMqReg(&regGatewayMq);

    //if(restarted)
    {
//        usleep(2000);
//        msgQueue_t initMsg = {CollectorEvent_START_COP, NULL, 0};
//        mq_send(collectorMq, (char*)&initMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }
}

/*!
 Application task processing.

 Public function defined in collector.h
 */
void Collector_process(void)
{
    /* Start the collector device in the network */
    if(Collector_events & COLLECTOR_START_EVT)
    {
        if(cllcState == Cllc_states_initWaiting)
        {
            processStartEvent();
        }
        copInitialized = true;
        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_START_EVT);

    }

    /* Is it time to send the next tracking message? */
    if(Collector_events & COLLECTOR_TRACKING_TIMEOUT_EVT)
    {
        /* Process Tracking Event */
        generateTrackingRequests();

        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_TRACKING_TIMEOUT_EVT);
    }

    /*
     The generate a config request for all associated devices that need one
     */
    if(Collector_events & COLLECTOR_CONFIG_EVT)
    {
        generateConfigRequests();

        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_CONFIG_EVT);
    }
    /*
     Don't process ApiMac messages until all of the collector events
     are processed.
     */
    if(Collector_events == 0)
    {
        /* Wait for response message or events */
        ApiMac_processIncoming();
    }
}

void * collectorThread(void *pvParameters)
{

    msgQueue_t incomingMsg;
    for(;;)
    {
        incomingMsg.event = CommonEvent_INVALID_EVENT;
        incomingMsg.msgPtr = NULL;
        incomingMsg.msgPtrLen = 0;

        mq_receive(collectorMq, (char*)&incomingMsg, sizeof(msgQueue_t), NULL);

        switch (incomingMsg.event)
        {
        case CollectorEvent_START_COP:
        {
            if(!copInitialized)
            {
                msgQueue_t initMsg;
                initMsg.event = CollectorEvent_RESET_COP;
                initMsg.msgPtr = NULL;
                initMsg.msgPtrLen = 0;
                mq_send(collectorMq, (char*)&initMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);
            }
        }
            break;

        case CollectorEvent_RESET_COP:
        {
            MtSys_resetReq_t restReq;
            restReq.Type = 0; //soft reset
            MtSys_resetReq(&restReq);
        }
            break;
        case CollectorEvent_INIT_COP:
            Collector_initCop();
            break;
        case CollectorEvent_PROCESS_NPI_CMD:
            Mt_parseCmd(incomingMsg.msgPtr, incomingMsg.msgPtrLen);
            break;
        case CollectorEvent_SEND_SNSR_CMD:
            //Collector_sendSnsrCmd();
            if(incomingMsg.msgPtr)
            {
                deviceCmd_t *inCmd = (deviceCmd_t*)incomingMsg.msgPtr;
                uint8_t *devMsgBuf;
                uint8_t msgLen = 0;
                switch(inCmd->cmdType)
                {
                case CmdType_FAN_DATA:
                    msgLen = SMSGS_SENSOR_FAN_LEN;
                    devMsgBuf = (uint8_t*)malloc(msgLen);
                    devMsgBuf[0] = Smsgs_cmdIds_fanSpeedChg;
                    devMsgBuf[1] = (uint8_t)inCmd->data;
                    break;
                case CmdType_DOORLOCK_DATA:
                    msgLen = SMSGS_SENSOR_DOORLOCK_LEN;
                    devMsgBuf = (uint8_t*)malloc(msgLen);
                    devMsgBuf[0] = Smsgs_cmdIds_doorlockChg;
                    devMsgBuf[1] = (uint8_t)inCmd->data;
                    break;
                case CmdType_LED_DATA:
                    msgLen = SMSGS_TOGGLE_LED_REQUEST_MSG_LEN;
                    devMsgBuf = (uint8_t*)malloc(msgLen);
                    devMsgBuf[0] = Smsgs_cmdIds_toggleLedReq;
                    break;
                case CmdType_LEAK_DATA:
                    msgLen = SMSGS_BUZZER_CTRL_REQUEST_MSG_LEN;
                    devMsgBuf = (uint8_t*)malloc(msgLen);
                    devMsgBuf[0] = (uint8_t)Smsgs_cmdIds_buzzerCtrlReq;
                    break;
                default:
                    msgLen = SMSGS_SENSOR_DOORLOCK_LEN;
                    devMsgBuf = (uint8_t*)malloc(msgLen);
                    devMsgBuf[0] = Smsgs_cmdIds_sensorData;
                    devMsgBuf[1] = (uint8_t)inCmd->data;
                    break;
                }

                sendMsg((Smsgs_cmdIds_t)devMsgBuf[0], inCmd->shortAddr,
                                        false,
                                        (msgLen),
                                        devMsgBuf);
                free(devMsgBuf);
            }
            break;
        case CollectorEvent_PERMIT_JOIN:
        {
            permitJoinCmd_t *tempPermJoin = (permitJoinCmd_t*) incomingMsg.msgPtr;
            Cllc_states_t permJoin = tempPermJoin->permitJoin ? Cllc_states_joiningAllowed : Cllc_states_joiningNotAllowed ;
            ApiMac_mlmeSetReqBool(ApiMac_attribute_associatePermit, tempPermJoin->permitJoin);
            appsrv_stateChangeUpdate(permJoin);
        }
            break;
        case COLLECTOR_PROCESS_EVT:
            Collector_process();
            break;
        case CLLC_PROCESS_EVT:
            /* Process LLC Events */
            Cllc_process();
            break;
        case CSF_PROCESS_EVT:
            /* Allow the Specific functions to process */
            Csf_processEvents();
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

/*!
 Build and send the configuration message to a device.

 Public function defined in collector.h
 */
Collector_status_t Collector_sendConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint32_t reportingInterval,
                                               uint32_t pollingInterval)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        //Llc_deviceListItem_t item;

        /* Is the device a known device? */
        //if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_CONFIG_REQUEST_MSG_LENGTH];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_configReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = Util_breakUint32(reportingInterval, 0);
            *pBuf++ = Util_breakUint32(reportingInterval, 1);
            *pBuf++ = Util_breakUint32(reportingInterval, 2);
            *pBuf++ = Util_breakUint32(reportingInterval, 3);
            *pBuf++ = Util_breakUint32(pollingInterval, 0);
            *pBuf++ = Util_breakUint32(pollingInterval, 1);
            *pBuf++ = Util_breakUint32(pollingInterval, 2);
            *pBuf = Util_breakUint32(pollingInterval, 3);

            if((sendMsg(Smsgs_cmdIds_configReq, pDstAddr->addr.shortAddr,
                        false,
                        (SMSGS_CONFIG_REQUEST_MSG_LENGTH),
                         buffer)) == true)
            {
                status = Collector_status_success;
                Collector_statistics.configRequestAttempts++;
                /* set timer for retry in case response is not received */
                Csf_setConfigClock(CONFIG_DELAY);
            }
            else
            {
                processConfigRetry();
            }
        }
    }

    return (status);
}

/*!
 Update the collector statistics

 Public function defined in collector.h
 */
void Collector_updateStats( void )
{
    /* update the stats from the MAC */
    ApiMac_mlmeGetReqUint32(ApiMac_attribute_diagRxSecureFail,
                            &Collector_statistics.rxDecryptFailures);

    ApiMac_mlmeGetReqUint32(ApiMac_attribute_diagTxSecureFail,
                            &Collector_statistics.txEncryptFailures);
}

/*!
 Build and send the toggle led message to a device.

 Public function defined in collector.h
 */
Collector_status_t Collector_sendToggleLedRequest(ApiMac_sAddr_t *pDstAddr)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_TOGGLE_LED_REQUEST_MSG_LEN];

            /* Build the message */
            buffer[0] = (uint8_t)Smsgs_cmdIds_toggleLedReq;

            sendMsg(Smsgs_cmdIds_toggleLedReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    SMSGS_TOGGLE_LED_REQUEST_MSG_LEN,
                    buffer);

            status = Collector_status_success;
        }
        else
        {
            status = Collector_status_deviceNotFound;
        }
    }

    return(status);
}



/******************************************************************************
 Local Functions
 *****************************************************************************/

/*!
 * @brief       Initialize the clocks.
 */
static void initializeClocks(void)
{
    /* Initialize the tracking clock */
    Csf_initializeTrackingClock();
    Csf_initializeConfigClock();
}

/*!
 * @brief      CLLC Started callback.
 *
 * @param      pStartedInfo - pointer to network information
 */
static void cllcStartedCB(Llc_netInfo_t *pStartedInfo)
{
    devicePanId = pStartedInfo->devInfo.panID;
    if(pStartedInfo->fh == true)
    {
        fhEnabled = true;
    }

    coordInfo.channel = pStartedInfo->channel;
    memcpy(&coordInfo.devInfo, &pStartedInfo->devInfo, sizeof(ApiMac_deviceDescriptor_t));
    coordInfo.fh = pStartedInfo->fh;
    /* updated the user */
    Csf_networkUpdate(restarted, pStartedInfo);

    /* Start the tracking clock */
    Csf_setTrackingClock(TRACKING_DELAY_TIME);
}

/*!
 * @brief      Device Joining callback from the CLLC module (ref.
 *             Cllc_deviceJoiningFp_t in cllc.h).  This function basically
 *             gives permission that the device can join with the return
 *             value.
 *
 * @param      pDevInfo - device information
 * @param      capInfo - device's capability information
 *
 * @return     ApiMac_assocStatus_t
 */
static ApiMac_assocStatus_t cllcDeviceJoiningCB(
                ApiMac_deviceDescriptor_t *pDevInfo,
                ApiMac_capabilityInfo_t *pCapInfo)
{
    ApiMac_assocStatus_t status;

    /* Make sure the device is in our PAN */
    if(pDevInfo->panID == devicePanId)
    {
        ApiMac_sAddr_t pDstAddr;
        pDstAddr.addrMode = ApiMac_addrType_short;
        pDstAddr.addr.shortAddr = pDevInfo->shortAddress;

        /* Update the user that a device is joining */
        status = Csf_deviceUpdate(pDevInfo, pCapInfo);
        /* Send the Config Request */
        Collector_sendConfigRequest(
                        &pDstAddr, (CONFIG_FRAME_CONTROL),
                        (CONFIG_REPORTING_INTERVAL),
                        (CONFIG_POLLING_INTERVAL));
        appsrv_networkUpdate(false, &coordInfo);
        if(status==ApiMac_assocStatus_success)
        {
#ifdef FEATURE_MAC_SECURITY
            /* Add device to security device table */
            Cllc_addSecDevice(pDevInfo->panID,
                              pDevInfo->shortAddress,
                              &pDevInfo->extAddress, 0);
#endif /* FEATURE_MAC_SECURITY */

            triggerCollectorEvt(COLLECTOR_CONFIG_EVT);

        }
    }
    else
    {
        status = ApiMac_assocStatus_panAccessDenied;
    }
    return (status);
}

/*!
 * @brief     CLLC State Changed callback.
 *
 * @param     state - CLLC new state
 */
static void cllcStateChangedCB(Cllc_states_t state)
{
    /* Save the state */
    cllcState = state;
    if(cllcState == Cllc_states_started && !collectorStarted)
    {
        collectorStarted = true;
        msgQueue_t initMsg = {CollectorEvent_START_COP, NULL, 0};
        mq_send(collectorMq, (char*)&initMsg, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }
    /* Notify the user interface */
    Csf_stateChangeUpdate(cllcState);
}

/*!
 * @brief      MAC Data Confirm callback.
 *
 * @param      pDataCnf - pointer to the data confirm information
 */
static void dataCnfCB(ApiMac_mcpsDataCnf_t *pDataCnf)
{
    /* Record statistics */
    if(pDataCnf->status == ApiMac_status_channelAccessFailure)
    {
        Collector_statistics.channelAccessFailures++;
    }
    else if(pDataCnf->status == ApiMac_status_noAck)
    {
        Collector_statistics.ackFailures++;
    }
    else if(pDataCnf->status == ApiMac_status_transactionExpired)
    {
        Collector_statistics.txTransactionExpired++;
    }
    else if(pDataCnf->status == ApiMac_status_transactionOverflow)
    {
        Collector_statistics.txTransactionOverflow++;
    }
    else if(pDataCnf->status == ApiMac_status_success)
    {
        Csf_updateFrameCounter(NULL, pDataCnf->frameCntr);
    }
    else if(pDataCnf->status != ApiMac_status_success)
    {
        Collector_statistics.otherTxFailures++;
    }

    /* Make sure the message came from the app */
    if(pDataCnf->msduHandle & APP_MARKER_MSDU_HANDLE)
    {
        /* What message type was the original request? */
        if(pDataCnf->msduHandle & APP_CONFIG_MSDU_HANDLE)
        {
            /* Config Request */
            Cllc_associated_devices_t *pDev;
            pDev = findDeviceStatusBit(ASSOC_CONFIG_MASK, ASSOC_CONFIG_SENT);
            if(pDev != NULL)
            {
                if(pDataCnf->status != ApiMac_status_success)
                {
                    /* Try to send again */
                    pDev->status &= ~ASSOC_CONFIG_SENT;
                    Csf_setConfigClock(CONFIG_DELAY);
                }
                else
                {
                    pDev->status |= ASSOC_CONFIG_SENT;
                    pDev->status |= ASSOC_CONFIG_RSP;
                    pDev->status |= CLLC_ASSOC_STATUS_ALIVE;
                    Csf_setConfigClock(CONFIG_RESPONSE_DELAY);
                }
            }

            /* Update stats */
            if(pDataCnf->status == ApiMac_status_success)
            {
                Collector_statistics.configReqRequestSent++;
            }
        }
        else
        {
            /* Tracking Request */
            Cllc_associated_devices_t *pDev;
            pDev = findDeviceStatusBit(ASSOC_TRACKING_SENT,
                                       ASSOC_TRACKING_SENT);
            if(pDev != NULL)
            {
                if(pDataCnf->status == ApiMac_status_success)
                {
                    /* Make sure the retry is clear */
                    pDev->status &= ~ASSOC_TRACKING_RETRY;
                }
                else
                {
                    if(pDev->status & ASSOC_TRACKING_RETRY)
                    {
                        /* We already tried to resend */
                        pDev->status &= ~ASSOC_TRACKING_RETRY;
                        pDev->status |= ASSOC_TRACKING_ERROR;
                    }
                    else
                    {
                        /* Go ahead and retry */
                        pDev->status |= ASSOC_TRACKING_RETRY;
                    }

                    pDev->status &= ~ASSOC_TRACKING_SENT;

                    /* Try to send again or another */
                    Csf_setTrackingClock(TRACKING_CNF_DELAY_TIME);
                }
            }

            /* Update stats */
            if(pDataCnf->status == ApiMac_status_success)
            {
                Collector_statistics.trackingReqRequestSent++;
            }
        }
    }
}

/*!
 * @brief      MAC Data Indication callback.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void dataIndCB(ApiMac_mcpsDataInd_t *pDataInd)
{
    if((pDataInd != NULL) && (pDataInd->msdu.p != NULL)
       && (pDataInd->msdu.len > 0))
    {
        Smsgs_cmdIds_t cmdId = (Smsgs_cmdIds_t)*(pDataInd->msdu.p);

#ifdef FEATURE_MAC_SECURITY
        if(Cllc_securityCheck(&(pDataInd->sec)) == false)
        {
            /* reject the message */
            return;
        }
#endif /* FEATURE_MAC_SECURITY */

        if(pDataInd->srcAddr.addrMode == ApiMac_addrType_extended)
        {
            uint16_t shortAddr = Csf_getDeviceShort(
                            &pDataInd->srcAddr.addr.extAddr);
            if(shortAddr != INVALID_SHORT_ADDR)
            {
                /* Switch to the short address for internal tracking */
                pDataInd->srcAddr.addrMode = ApiMac_addrType_short;
                pDataInd->srcAddr.addr.shortAddr = shortAddr;
            }
            else
            {
                /* Can't accept the message - ignore it */
                return;
            }
        }

        switch(cmdId)
        {
            case Smsgs_cmdIds_configRsp:
                processConfigResponse(pDataInd);
                break;

            case Smsgs_cmdIds_trackingRsp:
                processTrackingResponse(pDataInd);
                break;

            case Smsgs_cmdIds_toggleLedRsp:
                processToggleLedResponse(pDataInd);
                break;
            case Smsgs_cmdIds_doorlockChg:
            case Smsgs_cmdIds_fanSpeedChg:
            case Smsgs_cmdIds_sensorData:
                processSensorData(pDataInd);
                break;



            default:
                /* Should not receive other messages */
                break;
        }
    }
}

/*!
 * @brief      Process the start event
 */
static void processStartEvent(void)
{
    Llc_netInfo_t netInfo;
    uint32_t frameCounter = 0;

    Csf_getFrameCounter(NULL, &frameCounter);
    /* See if there is existing network information */
    if(Csf_getNetworkInformation(&netInfo))
    {
        Llc_deviceListItem_t *pDevList = NULL;
        uint16_t numDevices = 0;

#ifdef FEATURE_MAC_SECURITY
        /* Initialize the MAC Security */
        Cllc_securityInit(frameCounter);
#endif /* FEATURE_MAC_SECURITY */

        numDevices = Csf_getNumDeviceListEntries();
        if (numDevices > 0)
        {
            /* Allocate enough memory for all know devices */
            pDevList = (Llc_deviceListItem_t *)Csf_malloc(
                            sizeof(Llc_deviceListItem_t) * numDevices);
            if(pDevList)
            {
                uint8_t i = 0;

                /* Use a temp pointer to cycle through the list */
                Llc_deviceListItem_t *pItem = pDevList;
                for(i = 0; i < numDevices; i++, pItem++)
                {
                    Csf_getDeviceItem(i, pItem);

#ifdef FEATURE_MAC_SECURITY
                    /* Add device to security device table */
                    Cllc_addSecDevice(pItem->devInfo.panID,
                                      pItem->devInfo.shortAddress,
                                      &pItem->devInfo.extAddress,
                                      pItem->rxFrameCounter);
#endif /* FEATURE_MAC_SECURITY */
                }
            }
            else
            {
                numDevices = 0;
            }
        }

        /* Restore with the network and device information */
        Cllc_restoreNetwork(&netInfo, (uint8_t)numDevices, pDevList);

        if (pDevList)
        {
            Csf_free(pDevList);
        }

        restarted = true;
    }
    else
    {
        restarted = false;

#ifdef FEATURE_MAC_SECURITY
        /* Initialize the MAC Security */
        Cllc_securityInit(frameCounter);
#endif /* FEATURE_MAC_SECURITY */

        /* Start a new netork */
        Cllc_startNetwork();
    }
}

/*!
 * @brief      Process the Config Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processConfigResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_CONFIG_RESPONSE_MSG_LENGTH)
    {
        Cllc_associated_devices_t *pDev;
        Smsgs_configRspMsg_t configRsp;
        uint8_t *pBuf = pDataInd->msdu.p;

        /* Parse the message */
        configRsp.cmdId = (Smsgs_cmdIds_t)*pBuf++;

        configRsp.status = (Smsgs_statusValues_t)Util_buildUint16(pBuf[0],
                                                                  pBuf[1]);
        pBuf += 2;

        configRsp.frameControl = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;

        configRsp.reportingInterval = Util_buildUint32(pBuf[0], pBuf[1],
                                                       pBuf[2],
                                                       pBuf[3]);
        pBuf += 4;

        configRsp.pollingInterval = Util_buildUint32(pBuf[0], pBuf[1], pBuf[2],
                                                     pBuf[3]);

        pDev = findDevice(&pDataInd->srcAddr);
        if(pDev != NULL)
        {
            /* Clear the sent flag and set the response flag */
            pDev->status &= ~ASSOC_CONFIG_SENT;
            pDev->status |= ASSOC_CONFIG_RSP;
        }

        /* report the config response */
        Csf_deviceConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi,
                               &configRsp);

        triggerCollectorEvt(COLLECTOR_CONFIG_EVT);

        Collector_statistics.configResponseReceived++;
    }
}

/*!
 * @brief      Process the Tracking Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processTrackingResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_TRACKING_RESPONSE_MSG_LENGTH)
    {
        Cllc_associated_devices_t *pDev;

        pDev = findDevice(&pDataInd->srcAddr);
        if(pDev != NULL)
        {
            if(pDev->status & ASSOC_TRACKING_SENT)
            {
                pDev->status &= ~ASSOC_TRACKING_SENT;
                pDev->status |= ASSOC_TRACKING_RSP;

                /* Setup for next tracking */
                Csf_setTrackingClock( TRACKING_DELAY_TIME);

                /* retry config request */
                processConfigRetry();
            }
        }

        /* Update stats */
        Collector_statistics.trackingResponseReceived++;
    }
}

/*!
 * @brief      Process the Toggle Led Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processToggleLedResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_TOGGLE_LED_RESPONSE_MSG_LEN)
    {
        bool ledState;
        uint8_t *pBuf = pDataInd->msdu.p;

        /* Skip past the command ID */
        pBuf++;

        ledState = (bool)*pBuf;

        /* Notify the user */
        Csf_toggleResponseReceived(&pDataInd->srcAddr, ledState);
    }
}

/*!
 * @brief      Process the Sensor Data message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processSensorData(ApiMac_mcpsDataInd_t *pDataInd)
{
    Smsgs_sensorMsg_t sensorData;
    uint8_t *pBuf = pDataInd->msdu.p;

    memset(&sensorData, 0, sizeof(Smsgs_sensorMsg_t));

    /* Parse the message */
    sensorData.cmdId = (Smsgs_cmdIds_t)*pBuf++;

    memcpy(sensorData.extAddress, pBuf, SMGS_SENSOR_EXTADDR_LEN);
    pBuf += SMGS_SENSOR_EXTADDR_LEN;

    sensorData.frameControl = Util_buildUint16(pBuf[0], pBuf[1]);
    pBuf += 2;

    if(sensorData.frameControl & Smsgs_dataFields_tempSensor)
    {
        sensorData.tempSensor.ambienceTemp = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.tempSensor.objectTemp = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
    }

    if(sensorData.frameControl & Smsgs_dataFields_lightSensor)
    {
        sensorData.lightSensor.rawData = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
    }

    if(sensorData.frameControl & Smsgs_dataFields_humiditySensor)
    {
        sensorData.humiditySensor.temp = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.humiditySensor.humidity = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
    }

    if(sensorData.frameControl & Smsgs_dataFields_msgStats)
    {
        sensorData.msgStats.joinAttempts = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.joinFails = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.msgsAttempted = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.msgsSent = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingRequests = Util_buildUint16(pBuf[0],
                                                                pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingResponseAttempts = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingResponseSent = Util_buildUint16(pBuf[0],
                                                                    pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configRequests = Util_buildUint16(pBuf[0],
                                                              pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configResponseAttempts = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configResponseSent = Util_buildUint16(pBuf[0],
                                                                  pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.channelAccessFailures = Util_buildUint16(pBuf[0],
                                                                     pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.macAckFailures = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.otherDataRequestFailures = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.syncLossIndications = Util_buildUint16(pBuf[0],
                                                                   pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.rxDecryptFailures = Util_buildUint16(pBuf[0],
                                                                 pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.txEncryptFailures = Util_buildUint16(pBuf[0],
                                                                 pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.resetCount = Util_buildUint16(pBuf[0],
                                                          pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.lastResetReason = Util_buildUint16(pBuf[0],
                                                               pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.joinTime = Util_buildUint16(pBuf[0],
                                                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.interimDelay = Util_buildUint16(pBuf[0],
                                                            pBuf[1]);
        pBuf += 2;
    }

    if(sensorData.frameControl & Smsgs_dataFields_configSettings)
    {
        sensorData.configSettings.reportingInterval = Util_buildUint32(pBuf[0],
                                                                       pBuf[1],
                                                                       pBuf[2],
                                                                       pBuf[3]);
        pBuf += 4;
        sensorData.configSettings.pollingInterval = Util_buildUint32(pBuf[0],
                                                                     pBuf[1],
                                                                     pBuf[2],
                                                                     pBuf[3]);
        pBuf += 4;
    }

    if(sensorData.frameControl & Smsgs_dataFields_pressureSensor)
    {
        sensorData.pressureSensor.pressureValue = Util_buildUint32(pBuf[0],
                                                                    pBuf[1],
                                                                    pBuf[2],
                                                                    pBuf[3]);
        pBuf += 4;
        sensorData.pressureSensor.tempValue =  Util_buildUint32(pBuf[0],
                                                                pBuf[1],
                                                                pBuf[2],
                                                                pBuf[3]);
        pBuf += 4;
    }

    if(sensorData.frameControl & Smsgs_dataFields_motionSensor)
    {
      sensorData.motionSensor.isMotion = *pBuf++;
    }

    if(sensorData.frameControl & Smsgs_dataFields_batterySensor)
    {

      sensorData.batterySensor.voltageValue = Util_buildUint32(pBuf[0],
                                                                     pBuf[1],
                                                                     pBuf[2],
                                                                     pBuf[3]);
      pBuf +=4;
    }

    if(sensorData.frameControl & Smsgs_dataFields_hallEffectSensor)
    {
      sensorData.hallEffectSensor.isOpen = *pBuf++;
      sensorData.hallEffectSensor.isTampered = *pBuf++;
    }

    if(sensorData.frameControl & Smsgs_dataFields_fanSensor)
    {
      sensorData.fanSensor.fanSpeed = *pBuf++;
    }

    if(sensorData.frameControl & Smsgs_dataFields_doorLockSensor)
    {
      sensorData.doorLockSensor.isLocked = *pBuf++;
    }
    if(sensorData.frameControl & Smsgs_dataFields_waterleakSensor)
    {
        sensorData.waterleakSensor.status =  Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
    }

    Collector_statistics.sensorMessagesReceived++;

    /* Report the sensor data */
    Csf_deviceSensorDataUpdate(&pDataInd->srcAddr, pDataInd->rssi,
                               &sensorData);

    processDataRetry(&(pDataInd->srcAddr));
}

/*!
 * @brief      Find the associated device table entry matching pAddr.
 *
 * @param      pAddr - pointer to device's address
 *
 * @return     pointer to the associated device table entry,
 *             NULL if not found.
 */
static Cllc_associated_devices_t *findDevice(ApiMac_sAddr_t *pAddr)
{
    int x;
    Cllc_associated_devices_t *pItem = NULL;

    /* Check for invalid parameters */
    if((pAddr == NULL) || (pAddr->addrMode == ApiMac_addrType_none))
    {
        return (NULL);
    }

    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if(Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
        {
            if(pAddr->addrMode == ApiMac_addrType_short)
            {
                if(pAddr->addr.shortAddr == Cllc_associatedDevList[x].shortAddr)
                {
                    pItem = &Cllc_associatedDevList[x];
                    break;
                }
            }
        }
    }

    return (pItem);
}

/*!
 * @brief      Find the associated device table entry matching status bit.
 *
 * @param      statusBit - what status bit to find
 *
 * @return     pointer to the associated device table entry,
 *             NULL if not found.
 */
static Cllc_associated_devices_t *findDeviceStatusBit(uint16_t mask, uint16_t statusBit)
{
    int x;
    Cllc_associated_devices_t *pItem = NULL;

    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if(Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
        {
            if((Cllc_associatedDevList[x].status & mask) == statusBit)
            {
                pItem = &Cllc_associatedDevList[x];
                break;
            }
        }
    }

    return (pItem);
}

/*!
 * @brief      Get the next MSDU Handle
 *             <BR>
 *             The MSDU handle has 3 parts:<BR>
 *             - The MSBit(7), when set means the the application sent the message
 *             - Bit 6, when set means that the app message is a config request
 *             - Bits 0-5, used as a message counter that rolls over.
 *
 * @param      msgType - message command id needed
 *
 * @return     msdu Handle
 */
static uint8_t getMsduHandle(Smsgs_cmdIds_t msgType)
{
    uint8_t msduHandle = deviceTxMsduHandle;

    /* Increment for the next msdu handle, or roll over */
    if(deviceTxMsduHandle >= MSDU_HANDLE_MAX)
    {
        deviceTxMsduHandle = 0;
    }
    else
    {
        deviceTxMsduHandle++;
    }

    /* Add the App specific bit */
    msduHandle |= APP_MARKER_MSDU_HANDLE;

    /* Add the message type bit */
    if(msgType == Smsgs_cmdIds_configReq)
    {
        msduHandle |= APP_CONFIG_MSDU_HANDLE;
    }

    return (msduHandle);
}

/*!
 * @brief      Send MAC data request
 *
 * @param      type - message type
 * @param      dstShortAddr - destination short address
 * @param      rxOnIdle - true if not a sleepy device
 * @param      len - length of payload
 * @param      pData - pointer to the buffer
 *
 * @return  true if sent, false if not
 */
static bool sendMsg(Smsgs_cmdIds_t type, uint16_t dstShortAddr, bool rxOnIdle,
                    uint16_t len,
                    uint8_t *pData)
{
    ApiMac_mcpsDataReq_t dataReq;

    /* Fill the data request field */
    memset(&dataReq, 0, sizeof(ApiMac_mcpsDataReq_t));

    dataReq.dstAddr.addrMode = ApiMac_addrType_short;
    dataReq.dstAddr.addr.shortAddr = dstShortAddr;
    dataReq.srcAddrMode = ApiMac_addrType_short;

    if(fhEnabled)
    {
        Llc_deviceListItem_t item;

        if(Csf_getDevice(&(dataReq.dstAddr), &item))
        {
            /* Switch to the long address */
            dataReq.dstAddr.addrMode = ApiMac_addrType_extended;
            memcpy(&dataReq.dstAddr.addr.extAddr, &item.devInfo.extAddress,
                   (APIMAC_SADDR_EXT_LEN));
            dataReq.srcAddrMode = ApiMac_addrType_extended;
        }
        else
        {
            /* Can't send the message */
            return (false);
        }
    }

    dataReq.dstPanId = devicePanId;

    dataReq.msduHandle = getMsduHandle(type);

    dataReq.txOptions.ack = true;
    if(rxOnIdle == false)
    {
        dataReq.txOptions.indirect = true;
    }

    dataReq.msdu.len = len;
    dataReq.msdu.p = pData;

#ifdef FEATURE_MAC_SECURITY
    /* Fill in the appropriate security fields */
    Cllc_securityFill(&dataReq.sec);
#endif /* FEATURE_MAC_SECURITY */

    /* Send the message */
    if(ApiMac_mcpsDataReq(&dataReq) != ApiMac_status_success)
    {
        /*  transaction overflow occurred */
        return (false);
    }
    else
    {
        return (true);
    }
}

/*!
 * @brief      Generate Config Requests for all associate devices
 *             that need one.
 */
static void generateConfigRequests(void)
{
    int x;

    if(CERTIFICATION_TEST_MODE)
    {
        /* In Certification mode only back to back uplink
         * data traffic shall be supported*/
        return;
    }

    /* Clear any timed out transactions */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        if((Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
           && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            if((Cllc_associatedDevList[x].status &
               (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP))
               == (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP))
            {
                Cllc_associatedDevList[x].status &= ~(ASSOC_CONFIG_SENT
                                | ASSOC_CONFIG_RSP);
            }
        }
    }

    /* Make sure we are only sending one config request at a time */
    if(findDeviceStatusBit(ASSOC_CONFIG_MASK, ASSOC_CONFIG_SENT) == NULL)
    {
        /* Run through all of the devices */
        for(x = 0; x < CONFIG_MAX_DEVICES; x++)
        {
            /* Make sure the entry is valid. */
            if((Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
               && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
            {
                uint16_t status = Cllc_associatedDevList[x].status;

                /*
                 Has the device been sent or already received a config request?
                 */
                if(((status & (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP)) == 0))
                {
                    ApiMac_sAddr_t dstAddr;
                    Collector_status_t stat;

                    /* Set up the destination address */
                    dstAddr.addrMode = ApiMac_addrType_short;
                    dstAddr.addr.shortAddr =
                        Cllc_associatedDevList[x].shortAddr;

                    /* Send the Config Request */
                    stat = Collector_sendConfigRequest(
                                    &dstAddr, (CONFIG_FRAME_CONTROL),
                                    (CONFIG_REPORTING_INTERVAL),
                                    (CONFIG_POLLING_INTERVAL));
                    if(stat == Collector_status_success)
                    {
                        /*
                         Mark as the message has been sent and expecting a response
                         */
                        Cllc_associatedDevList[x].status |= ASSOC_CONFIG_SENT;
                        Cllc_associatedDevList[x].status &= ~ASSOC_CONFIG_RSP;
                    }

                    /* Only do one at a time */
                    break;
                }
            }
        }
    }
}


/*!
 * @brief      Generate Config Requests for all associate devices
 *             that need one.
 */
static void generateTrackingRequests(void)
{
    int x;

    /* Run through all of the devices, looking for previous activity */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        if(CERTIFICATION_TEST_MODE)
        {
            /* In Certification mode only back to back uplink
             * data traffic shall be supported*/
            return;
        }
        /* Make sure the entry is valid. */
        if((Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
             && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            uint16_t status = Cllc_associatedDevList[x].status;

            /*
             Has the device been sent a tracking request or received a
             tracking response?
             */
            if(status & ASSOC_TRACKING_RETRY)
            {
                sendTrackingRequest(&Cllc_associatedDevList[x]);
                return;
            }
            else if((status & (ASSOC_TRACKING_SENT | ASSOC_TRACKING_RSP
                               | ASSOC_TRACKING_ERROR)))
            {
                Cllc_associated_devices_t *pDev = NULL;
                int y;

                if(status & (ASSOC_TRACKING_SENT | ASSOC_TRACKING_ERROR))
                {
                    ApiMac_deviceDescriptor_t devInfo;
                    Llc_deviceListItem_t item;
                    ApiMac_sAddr_t devAddr;

                    /*
                     Timeout occured, notify the user that the tracking
                     failed.
                     */
                    memset(&devInfo, 0, sizeof(ApiMac_deviceDescriptor_t));

                    devAddr.addrMode = ApiMac_addrType_short;
                    devAddr.addr.shortAddr =
                        Cllc_associatedDevList[x].shortAddr;

                    if(Csf_getDevice(&devAddr, &item))
                    {
                        memcpy(&devInfo.extAddress,
                               &item.devInfo.extAddress,
                               sizeof(ApiMac_sAddrExt_t));
                    }
                    devInfo.shortAddress = Cllc_associatedDevList[x].shortAddr;
                    devInfo.panID = devicePanId;
                    Csf_deviceNotActiveUpdate(&devInfo,
                        ((status & ASSOC_TRACKING_SENT) ? true : false));

                    /* Not responding, so remove the alive marker */
                    Cllc_associatedDevList[x].status
                            &= ~(CLLC_ASSOC_STATUS_ALIVE
                                | ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP);
                }

                /* Clear the tracking bits */
                Cllc_associatedDevList[x].status  &= ~(ASSOC_TRACKING_ERROR
                                | ASSOC_TRACKING_SENT | ASSOC_TRACKING_RSP);

                /* Find the next valid device */
                y = x;
                while(pDev == NULL)
                {
                    /* Check for rollover */
                    if(y == (CONFIG_MAX_DEVICES-1))
                    {
                        /* Move to the beginning */
                        y = 0;
                    }
                    else
                    {
                        /* Move the the next device */
                        y++;
                    }

                    if(y == x)
                    {
                        /* We've come back around */
                        break;
                    }

                    /*
                     Is the entry valid and active */
                    if((Cllc_associatedDevList[y].shortAddr
                                    != INVALID_SHORT_ADDR)
                         && (Cllc_associatedDevList[y].status
                                   & CLLC_ASSOC_STATUS_ALIVE))
                    {
                        pDev = &Cllc_associatedDevList[y];
                    }
                }

                if(pDev == NULL)
                {
                    /* Another device wasn't found, send to same device */
                    pDev = &Cllc_associatedDevList[x];
                }

                sendTrackingRequest(pDev);

                /* Only do one at a time */
                return;
            }
        }
    }

    /* if no activity found, find the first active device */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if((Cllc_associatedDevList[x].shortAddr != INVALID_SHORT_ADDR)
              && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            sendTrackingRequest(&Cllc_associatedDevList[x]);
            break;
        }
    }

    if(x == CONFIG_MAX_DEVICES)
    {
        /* No device found, Setup delay for next tracking message */
        Csf_setTrackingClock(TRACKING_DELAY_TIME);
    }
}

/*!
 * @brief      Generate Tracking Requests for a device
 *
 * @param      pDev - pointer to the device's associate device table entry
 */
static void sendTrackingRequest(Cllc_associated_devices_t *pDev)
{
    uint8_t cmdId = Smsgs_cmdIds_trackingReq;

    /* Send the Tracking Request */
   if((sendMsg(Smsgs_cmdIds_trackingReq, pDev->shortAddr,
            pDev->capInfo.rxOnWhenIdle,
            (SMSGS_TRACKING_REQUEST_MSG_LENGTH),
            &cmdId)) == true)
    {
        /* Mark as Tracking Request sent */
        pDev->status |= ASSOC_TRACKING_SENT;

        /* Setup Timeout for response */
        Csf_setTrackingClock(TRACKING_TIMEOUT_TIME);

        /* Update stats */
        Collector_statistics.trackingRequestAttempts++;
    }
    else
    {
        ApiMac_sAddr_t devAddr;
        devAddr.addrMode = ApiMac_addrType_short;
        devAddr.addr.shortAddr = pDev->shortAddr;
        processDataRetry(&devAddr);
    }
}

/*!
 * @brief      Process the MAC Comm Status Indication Callback
 *
 * @param      pCommStatusInd - Comm Status indication
 */
static void commStatusIndCB(ApiMac_mlmeCommStatusInd_t *pCommStatusInd)
{
    if(pCommStatusInd->reason == ApiMac_commStatusReason_assocRsp)
    {
        if(pCommStatusInd->status != ApiMac_status_success)
        {
            Cllc_associated_devices_t *pDev;

            pDev = findDevice(&pCommStatusInd->dstAddr);
            if(pDev)
            {
                /* Mark as inactive and clear config and tracking states */
                pDev->status = 0;
            }
        }
    }
}

/*!
 * @brief      Process the MAC Poll Indication Callback
 *
 * @param      pPollInd - poll indication
 */
static void pollIndCB(ApiMac_mlmePollInd_t *pPollInd)
{
    ApiMac_sAddr_t addr;

    addr.addrMode = ApiMac_addrType_short;
    if (pPollInd->srcAddr.addrMode == ApiMac_addrType_short)
    {
        addr.addr.shortAddr = pPollInd->srcAddr.addr.shortAddr;
    }
    else
    {
        addr.addr.shortAddr = Csf_getDeviceShort(
                        &pPollInd->srcAddr.addr.extAddr);
    }

    processDataRetry(&addr);
}

/*!
 * @brief      Process retries for config and tracking messages
 *
 * @param      addr - MAC address structure */
static void processDataRetry(ApiMac_sAddr_t *pAddr)
{
    if(pAddr->addr.shortAddr != INVALID_SHORT_ADDR)
    {
        Cllc_associated_devices_t *pItem;
        pItem = findDevice(pAddr);
        if(pItem)
        {
            /* Set device status to alive */
            pItem->status |= CLLC_ASSOC_STATUS_ALIVE;

            /* Check to see if we need to send it a config */
            if((pItem->status & (ASSOC_CONFIG_RSP | ASSOC_CONFIG_SENT)) == 0)
            {
                processConfigRetry();
            }
            /* Check to see if we need to send it a tracking message */
            if((pItem->status & (ASSOC_TRACKING_SENT| ASSOC_TRACKING_RETRY)) == 0)
            {
                /* Make sure we aren't already doing a tracking message */
                if(((Collector_events & COLLECTOR_TRACKING_TIMEOUT_EVT) == 0)
                    && (Csf_isTrackingTimerActive() == false)
                    && (findDeviceStatusBit(ASSOC_TRACKING_MASK,
                                            ASSOC_TRACKING_SENT) == NULL))
                {
                    /* Setup for next tracking */
                    Csf_setTrackingClock(TRACKING_DELAY_TIME);
                }
            }
        }
    }
}

/*!
 * @brief      Process retries for config messages
 */
static void processConfigRetry(void)
{
    /* retry config request if not already sent */
    if(((Collector_events & COLLECTOR_CONFIG_EVT) == 0)
        && (Csf_isConfigTimerActive() == false))
    {
        /* Set config event */
        Csf_setConfigClock(CONFIG_DELAY);
    }
}
