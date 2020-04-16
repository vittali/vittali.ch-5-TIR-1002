/******************************************************************************

 @file commonDefs.h

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
#ifndef COMMON_COMMONDEFS_H_
#define COMMON_COMMONDEFS_H_
//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stdbool.h>
#include <Collector/LinkController/llc.h>
#include <Collector/LinkController/cllc.h>
#include "Collector/smsgs.h"
#include <API_MAC/api_mac.h>

#define TASKSTACKSIZE           2048


//USER DEFS
#define MAX_NUM_OF_DEVICES      25
#define MAX_NUM_OF_OBJECTS      15

#define SL_TASK_PRI             6
#define GTWAY_TASK_PRI          5
#define NPI_TASK_PRI            4
#define COLLECTOR_TASK_PRI      3
#define CLOUDSRV_TASK_PRI       3
#define CLOUDRX_TASK_PRI        6
#define HIGHEST_PRI             6

//IPSO DEFS
#define TEMP_TYPE           "temperature"
#define HUM_TYPE            "humidity"
#define PRESS_TYPE          "barometer"
#define MOTION_TYPE         "motion"
#define VOLTAGE_TYPE        "voltage"
#define HALL_OPEN_TYPE      "halleffect"
#define HALL_TMPR_TYPE      "halleffecttmpr"
#define WATR_LEAK_TYPE      "waterleak"
#define LIGHT_TYPE          "illuminance"
#define ACTUATOR_TYPE       "actuation"
#define FAN_TYPE            "fan"
#define DOOR_LOCK_TYPE      "doorlock"
#define TIME_STAMP          "last_reported"
#define MAX_TYPE_CHAR_LEN   18

#define GEN_SENSOR_TYPE_ID  3300
#define PRESENSE_TYPE_ID    3302
#define TEMP_TYPE_ID        3303
#define HUM_TYPE_ID         3304
#define LIGHT_TYPE_ID       3301
#define ACTUATOR_TYPE_ID    3306

// QUEUE names
#define NPI_MQ          "npiMq"
#define COLLECTOR_MQ    "collectorMq"
#define GATEWAY_MQ      "gatewayMq"
#define CLOUDSERVICE_MQ "clousServiceMq"
#define MT_SRSP_MQ      "mtSrspMq"

#define MQ_HIGH_PRIOR    1
#define MQ_LOW_PRIOR     0

//Common EVENTS
typedef enum
{
    CommonEvent_WLAN_DISCONNECTED = 0x80,
    CommonEvent_WLAN_CONNECTED,
    CommonEvent_CLOUD_SERV_CONNECTED,
    CommonEvent_CLOUD_SERV_DISCONNECTED,
    CommonEvent_INVALID_EVENT = 0xFF,

}CommonEvent;

// NPI EVENTS

typedef enum
{
    NPIEvent_TRANSPRT_RX,
    NPIEvent_TRANSPRT_TX,
}NPIEvent;
// COLLECTOR EVENTS

typedef enum
{
    CollectorEvent_START_COP,
    CollectorEvent_PROCESS_NPI_CMD,
    CollectorEvent_SEND_SNSR_CMD,
    CollectorEvent_PERMIT_JOIN,
    CollectorEvent_RESET_COP,
    CollectorEvent_INIT_COP

}CollectorEvent;
// MT_EVENTS
typedef enum
{
    MtEvent_RECEIVED_SRSP
}MtEvent;
//GATEWAY EVENTS

typedef enum
{
    //Provisioning Events
    GatewayEvent_STARTED,
    GatewayEvent_CONNECTED,
    GatewayEvent_IP_ACQUIRED,
    GatewayEvent_DISCONNECT,

    GatewayEvent_PING_COMPLETE,

    GatewayEvent_PROVISIONING_STARTED,
    GatewayEvent_PROVISIONING_SUCCESS,
    GatewayEvent_PROVISIONING_STOPPED,
    GatewayEvent_PROVISIONING_WAIT_CONN,

    GatewayEvent_TIMEOUT,
    GatewayEvent_ERROR,
    GatewayEvent_RESTART,
    GatewayEvent_MAX,
    GatewayEvent_INIT_COMPLETE,

    // Collector to Gateway event
    GatewayEvent_NWK_UPDATE,
    GatewayEvent_DEV_UPDATE,
    GatewayEvent_DEV_CNF_UPDATE,
    GatewayEvent_DEV_NOT_ACTIVE,
    GatewayEvent_SENSOR_DATA_UPDATE,
    GatewayEvent_NWK_STATE_CHANGE,
    // Cloud Service to Gateway Event
    GatewayEvent_PERMIT_JOIN,
    GatewayEvent_DEVICE_CMD,

}GatewayEvent;

//CLOUDSERVICE EVENTS
typedef enum
{
    //Cloud to Cloud events
    CloudServiceEvt_CLOUDINFO_RX,
    //Gateway to Cloud Service events
    CloudServiceEvt_NWK_UPDATE,
    CloudServiceEvt_DEV_UPDATE,
    CloudServiceEvt_STATE_CNF_EVT,
    CloudServiceEvt_CLOUD_IN_MQTT,
    CloudServiceEvt_LOCAL_SERVR_HTTP

}CloudServiceEvt;


typedef struct
{
    uint8_t     event;
    void        *msgPtr;
    int32_t     msgPtrLen;
}msgQueue_t;

typedef struct nwk_t
{
    char name[7];
    uint8_t channel;
    uint16_t panId;
    uint16_t shortAddr;
    uint8_t extAddr[8];
    bool security_enable;
    bool mode; //frequency hopping if true
    Cllc_states_t state; //enum
    uint8_t devCount;
}nwk_t;

typedef struct smartObject_t
{
    int typeId;
    char type[MAX_TYPE_CHAR_LEN];
    int sensorVal;
    char unit[7];
}smartObject_t;

typedef struct dev_t
{
    char name[10];
    uint16_t shortAddr;
    uint8_t extAddr[8];
    char *topic;
    signed int rssi;
    bool active;
    smartObject_t object[MAX_NUM_OF_OBJECTS];
    uint8_t objectCount;
}dev_t;

typedef struct
{
    bool permitJoin;
}permitJoinCmd_t;


typedef enum
{
    CmdType_PERMIT_JOIN,
    CmdType_DEVICE_DATA,
    CmdType_FAN_DATA,
    CmdType_DOORLOCK_DATA,
    CmdType_LED_DATA,
    CmdType_LEAK_DATA
}CmdTypes;

typedef struct
{
    uint16_t cmdType;
    uint16_t shortAddr;
    uint8_t  extAddr[8];
    uint32_t data;
}deviceCmd_t;







//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif
#endif /* COMMON_COMMONDEFS_H_ */
