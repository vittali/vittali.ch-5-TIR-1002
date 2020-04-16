/******************************************************************************

 @file csf.c

 @brief Collector Specific Functions

 Group: WCS LPC
 Target Device: CC13xx

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
 Release Name: simplelink_cc13x0_sdk_1_50_00_
 Release Date: 2017-09-27 23:49:29
 *****************************************************************************/

/******************************************************************************
 Includes
 *****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <mqueue.h>
#include <Utils/util.h>
#include <timer.h>
#include "config.h"
#include "nvoctp.h"

#include <API_MAC/api_mac.h>
#include "LinkController/llc.h"
#include "LinkController/cllc.h"

#include "smsgs.h"
#include "csf.h"
#include "appHandler.h"
/******************************************************************************
 Constants and definitions
 *****************************************************************************/

/* Initial timeout value for the tracking clock */
#define TRACKING_INIT_TIMEOUT_VALUE 100

/* NV Item ID - the device's network information */
#define CSF_NV_NETWORK_INFO_ID 0x0001
/* NV Item ID - the number of black list entries */
#define CSF_NV_BLACKLIST_ENTRIES_ID 0x0002
/* NV Item ID - the black list, use sub ID for each record in the list */
#define CSF_NV_BLACKLIST_ID 0x0003
/* NV Item ID - the number of device list entries */
#define CSF_NV_DEVICELIST_ENTRIES_ID 0x0004
/* NV Item ID - the device list, use sub ID for each record in the list */
#define CSF_NV_DEVICELIST_ID 0x0005
/* NV Item ID - this devices frame counter */
#define CSF_NV_FRAMECOUNTER_ID 0x0006
/* NV Item ID - reset reason */
#define CSF_NV_RESET_REASON_ID 0x0007

/* Maximum number of black list entries */
#define CSF_MAX_BLACKLIST_ENTRIES 10

/* Maximum number of device list entries */
#define CSF_MAX_DEVICELIST_ENTRIES CONFIG_MAX_DEVICES

/*
 Maximum sub ID for a blacklist item, this is failsafe.  This is
 not the maximum number of items in the list
 */
#define CSF_MAX_BLACKLIST_IDS (2*CONFIG_MAX_DEVICES)

/*
 Maximum sub ID for a device list item, this is failsafe.  This is
 not the maximum number of items in the list
 */
#define CSF_MAX_DEVICELIST_IDS (2*CONFIG_MAX_DEVICES)

/* timeout value for trickle timer initialization */
#define TRICKLE_TIMEOUT_VALUE       20

/* timeout value for join timer */
#define JOIN_TIMEOUT_VALUE       20
/* timeout value for config request delay */
#define CONFIG_TIMEOUT_VALUE 1000
/*
 The increment value needed to save a frame counter. Example, setting this
 constant to 100, means that the frame counter will be saved when the new
 frame counter is 100 more than the last saved frame counter.  Also, when
 the get frame counter function reads the value from NV it will add this value
 to the read value.
 */
#define FRAME_COUNTER_SAVE_WINDOW     25

/* Value returned from findDeviceListIndex() when not found */
#define DEVICE_INDEX_NOT_FOUND  -1

/*! NV driver item ID for reset reason */
#define NVID_RESET {NVINTF_SYSID_APP, CSF_NV_RESET_REASON_ID, 0}

/******************************************************************************
 External variables
 *****************************************************************************/



/******************************************************************************
 Local variables
 *****************************************************************************/


/* Clock/timer resources */
static Clock_Struct trackingClkStruct;
static Clock_Handle trackingClkHandle;
static Clock_Struct broadcastClkStruct;
static Clock_Handle broadcastClkHandle;

/* Clock/timer resources for CLLC */
/* trickle timer */
STATIC Clock_Struct tricklePAClkStruct;
STATIC Clock_Handle tricklePAClkHandle;
STATIC Clock_Struct tricklePCClkStruct;
STATIC Clock_Handle tricklePCClkHandle;

/* timer for join permit */
STATIC Clock_Struct joinClkStruct;
STATIC Clock_Handle joinClkHandle;

/* timer for config request delay */
STATIC Clock_Struct configClkStruct;
STATIC Clock_Handle configClkHandle;

/* NV Function Pointers */
static NVINTF_nvFuncts_t *pNV = NULL;



static bool started = false;

/* The last saved coordinator frame counter */
static uint32_t lastSavedCoordinatorFrameCounter = 0;



/******************************************************************************
 Global variables
 *****************************************************************************/
/* Key press parameters */
uint8_t Csf_keys;

/* pending Csf_events */
uint16_t Csf_events = 0;

/* Saved CLLC state */
Cllc_states_t savedCllcState = Cllc_states_initWaiting;

/******************************************************************************
 Local function prototypes
 *****************************************************************************/

static void processTackingTimeoutCallback(UArg a0);
static void processBroadcastTimeoutCallback(UArg a0);
static void processPATrickleTimeoutCallback(UArg a0);
static void processPCTrickleTimeoutCallback(UArg a0);
static void processJoinTimeoutCallback(UArg a0);
static void processConfigTimeoutCallback(UArg a0);
static bool addDeviceListItem(Llc_deviceListItem_t *pItem);
static void updateDeviceListItem(Llc_deviceListItem_t *pItem);
static int findDeviceListIndex(ApiMac_sAddrExt_t *pAddr);
static int findUnusedDeviceListIndex(void);
static void saveNumDeviceListEntries(uint16_t numEntries);
static int findBlackListIndex(ApiMac_sAddr_t *pAddr);
static int findUnusedBlackListIndex(void);
static uint16_t getNumBlackListEntries(void);
static void saveNumBlackListEntries(uint16_t numEntries);
void removeBlackListItem(ApiMac_sAddr_t *pAddr);
#if defined(TEST_REMOVE_DEVICE)
static void removeTheFirstDevice(void);
#else
static uint16_t getTheFirstDevice(void);
#endif

/******************************************************************************
 Public Functions
 *****************************************************************************/

/*!
 The application calls this function during initialization

 Public function defined in csf.h
 */
void Csf_init(void)
{

#ifdef NV_RESTORE

    /* Setup the NV driver */
#ifdef ONE_PAGE_NV
    NVOCOP_loadApiPtrs(pNV);
#else
    pNV = malloc(sizeof(NVINTF_nvFuncts_t));
    NVOCTP_loadApiPtrs(pNV);
#endif

    if(pNV->initNV)
    {
        pNV->initNV(NULL);
    }
#endif


    /* Initialize PA/LNA if enabled */
    ApiMac_mlmeSetReqUint8(ApiMac_attribute_rangeExtender,
                           (uint8_t)CONFIG_RANGE_EXT_MODE);

    /* Initialize keys */
    //if(Board_Key_initialize(processKeyChangeCallback) == KEY_RIGHT)
    {
        /* Right key is pressed on power up, clear all NV */
        //Csf_clearAllNVItems();
    }


}

/*!
 The application must call this function periodically to
 process any Csf_events that this module needs to process.

 Public function defined in csf.h
 */
void Csf_processEvents(void)
{
    /* Did a key press occur? */


}

/*!
 The application calls this function to retrieve the stored
 network information.

 Public function defined in csf.h
 */
bool Csf_getNetworkInformation(Llc_netInfo_t *pInfo)
{
    if((pNV != NULL) && (pNV->readItem != NULL) && (pInfo != NULL))
    {
        NVINTF_itemID_t id;

        /* Setup NV ID */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_NETWORK_INFO_ID;
        id.subID = 0;

        /* Read Network Information from NV */
        if(pNV->readItem(id, 0, sizeof(Llc_netInfo_t), pInfo) == NVINTF_SUCCESS)
        {
            return(true);
        }
    }
    return(false);
}

/*!
 The application calls this function to indicate that it has
 started or restored the device in a network

 Public function defined in csf.h
 */
void Csf_networkUpdate(bool restored, Llc_netInfo_t *pNetworkInfo)
{
    /* check for valid structure pointer, ignore if not */
    if(pNetworkInfo != NULL)
    {
        if((pNV != NULL) && (pNV->writeItem != NULL))
        {
            NVINTF_itemID_t id;

            /* Setup NV ID */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_NETWORK_INFO_ID;
            id.subID = 0;

            /* Write the NV item */
            pNV->writeItem(id, sizeof(Llc_netInfo_t), pNetworkInfo);
        }

        started = true;

        if(restored == false)
        {
            //LCD_WRITE_STRING("Started", 2);
        }
        else
        {
           // LCD_WRITE_STRING("Restarted", 2);
        }

        if(pNetworkInfo->fh == false)
        {
            //LCD_WRITE_STRING_VALUE("Channel: ", pNetworkInfo->channel, 10, 3);
        }
        else
        {
            //LCD_WRITE_STRING("Freq. Hopping", 3);
        }

        //Board_Led_control(board_led_type_LED1, board_led_state_ON);
        appsrv_networkUpdate(restored, pNetworkInfo);


    }
}

/*!
 The application calls this function to indicate that a device
 has joined the network.

 Public function defined in csf.h
 */
ApiMac_assocStatus_t Csf_deviceUpdate(ApiMac_deviceDescriptor_t *pDevInfo,
                                      ApiMac_capabilityInfo_t *pCapInfo)
{
    ApiMac_assocStatus_t status = ApiMac_assocStatus_success;
    ApiMac_sAddr_t shortAddr;
    ApiMac_sAddr_t extAddr;

    shortAddr.addrMode = ApiMac_addrType_short;
    shortAddr.addr.shortAddr = pDevInfo->shortAddress;

    extAddr.addrMode = ApiMac_addrType_extended;
    memcpy(&extAddr.addr.extAddr, &pDevInfo->extAddress, APIMAC_SADDR_EXT_LEN);

    /* Is the device in the black list? */
    if((findBlackListIndex(&shortAddr) >= 0)
       || (findBlackListIndex(&extAddr) >= 0))
    {
        /* Denied */
        status = ApiMac_assocStatus_panAccessDenied;


    }
    else
    {
        /* Save the device information */
        Llc_deviceListItem_t dev;

        memcpy(&dev.devInfo, pDevInfo, sizeof(ApiMac_deviceDescriptor_t));
        memcpy(&dev.capInfo, pCapInfo, sizeof(ApiMac_capabilityInfo_t));
        dev.rxFrameCounter = 0;

        if(addDeviceListItem(&dev) == false)
        {
#ifdef NV_RESTORE
            status = ApiMac_assocStatus_panAtCapacity;

#else
            status = ApiMac_assocStatus_success;
            appsrv_deviceUpdate(&dev);

#endif
        }
        else
        {
            appsrv_deviceUpdate(&dev);
        }
    }

#if defined(MT_CSF)
    MTCSF_deviceUpdateIndCB(pDevInfo, pCapInfo);
#endif

    /* Return the status of the joining device */
    return (status);
}

/*!
 The application calls this function to indicate that a device
 is no longer active in the network.

 Public function defined in csf.h
 */
void Csf_deviceNotActiveUpdate(ApiMac_deviceDescriptor_t *pDevInfo,
bool timeout)
{
    appsrv_deviceNotActiveUpdate(pDevInfo, timeout);


}

/*!
 The application calls this function to indicate that a device
 has responded to a Config Request.

 Public function defined in csf.h
 */
void Csf_deviceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                            Smsgs_configRspMsg_t *pMsg)
{
    appsrv_deviceConfigUpdate(pSrcAddr,rssi,pMsg);
}

/*!
 The application calls this function to indicate that a device
 has reported sensor data.

 Public function defined in csf.h
 */
void Csf_deviceSensorDataUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                Smsgs_sensorMsg_t *pMsg)
{

    appsrv_deviceSensorDataUpdate(pSrcAddr, rssi, pMsg);
    //Board_Led_toggle(board_led_type_LED2);

    //LCD_WRITE_STRING_VALUE("Sensor 0x", pSrcAddr->addr.shortAddr, 16, 6);

}

/*!
 The application calls this function to indicate that a device
 set a Toggle LED Response message.

 Public function defined in csf.h
 */
void Csf_toggleResponseReceived(ApiMac_sAddr_t *pSrcAddr, bool ledState)
{
#if defined(MT_CSF)
    uint16_t shortAddr = 0xFFFF;

    if(pSrcAddr)
    {
        if(pSrcAddr->addrMode == ApiMac_addrType_short)
        {
            shortAddr = pSrcAddr->addr.shortAddr;
        }
        else
        {
            /* Convert extended to short addr */
            shortAddr = Csf_getDeviceShort(&pSrcAddr->addr.extAddr);
        }
    }
    MTCSF_deviceToggleIndCB(shortAddr, ledState);
#endif
}

/*!
 The application calls this function to indicate that the
 Coordinator's state has changed.

 Public function defined in csf.h
 */
void Csf_stateChangeUpdate(Cllc_states_t state)
{
    if(started == true)
    {
        if(state == Cllc_states_joiningAllowed)
        {
            /* Flash LED1 while allowing joining */
            //Board_Led_control(board_led_type_LED1, board_led_state_BLINKING);
        }
        else if(state == Cllc_states_joiningNotAllowed)
        {
            /* Don't flash when not allowing joining */
            //Board_Led_control(board_led_type_LED1, board_led_state_ON);
        }
    }
    appsrv_stateChangeUpdate(state);
    /* Save the state to be used later */
    savedCllcState = state;

}

/*!
 Initialize the tracking clock.

 Public function defined in csf.h
 */
void Csf_initializeTrackingClock(void)
{
    /* Initialize the timers needed for this application */
    if(trackingClkHandle == NULL)
    {
        trackingClkHandle = Timer_construct(&trackingClkStruct,
                                            processTackingTimeoutCallback,
                                            TRACKING_INIT_TIMEOUT_VALUE,
                                            0,
                                            false,
                                            0);
    }
    else if(Timer_isActive(&trackingClkStruct) == true)
    {
        Timer_stop(&trackingClkStruct);
    }
}

/*!
 Initialize the broadcast cmd clock.

 Public function defined in csf.h
 */
void Csf_initializeBroadcastClock(void)
{
    /* Initialize the timers needed for this application */
    broadcastClkHandle = Timer_construct(&broadcastClkStruct,
                                        processBroadcastTimeoutCallback,
                                        TRACKING_INIT_TIMEOUT_VALUE,
                                        0,
                                        false,
                                        0);
}

/*!
 Initialize the trickle clock.

 Public function defined in csf.h
 */
void Csf_initializeTrickleClock(void)
{
    /* Initialize trickle timer */
    if(tricklePAClkHandle == NULL)
    {
        tricklePAClkHandle = Timer_construct(&tricklePAClkStruct,
                                             processPATrickleTimeoutCallback,
                                             TRICKLE_TIMEOUT_VALUE,
                                             0,
                                             false,
                                             0);
    }
    else if(Timer_isActive(&tricklePAClkStruct) == true)
    {
        Timer_stop(&tricklePAClkStruct);
    }

    if(tricklePCClkHandle == NULL)
    {
        tricklePCClkHandle = Timer_construct(&tricklePCClkStruct,
                                             processPCTrickleTimeoutCallback,
                                             TRICKLE_TIMEOUT_VALUE,
                                             0,
                                             false,
                                             0);
    }
    else if(Timer_isActive(&tricklePCClkStruct) == true)
    {
        Timer_stop(&tricklePCClkStruct);
    }
}

/*!
 Initialize the clock for join permit attribute.

 Public function defined in csf.h
 */
void Csf_initializeJoinPermitClock(void)
{
    /* Initialize join permit timer */
    if(joinClkHandle == NULL)
    {
        joinClkHandle = Timer_construct(&joinClkStruct,
                                        processJoinTimeoutCallback,
                                        JOIN_TIMEOUT_VALUE,
                                        0,
                                        false,
                                        0);
    }
    else if(Timer_isActive(&joinClkStruct) == true)
    {
        Timer_stop(&joinClkStruct);
    }
}

/*!
 Initialize the clock for config request delay

 Public function defined in csf.h
 */
void Csf_initializeConfigClock(void)
{
    /* Initialize join permit timer */
    if(configClkHandle == NULL)
    {
        configClkHandle = Timer_construct(&configClkStruct,
                                        processConfigTimeoutCallback,
                                        CONFIG_TIMEOUT_VALUE,
                                        0,
                                        false,
                                        0);
    }
    else if(Timer_isActive(&configClkStruct) == true)
    {
        Timer_stop(&configClkStruct);
    }
}

/*!
 Set the tracking clock.

 Public function defined in csf.h
 */
void Csf_setTrackingClock(uint32_t trackingTime)
{
    /* Stop the Tracking timer */
    if(Timer_isActive(&trackingClkStruct) == true)
    {
        Timer_stop(&trackingClkStruct);
    }

    if(trackingTime)
    {
        /* Setup timer */
        Timer_setTimeout(trackingClkHandle, trackingTime);
        Timer_start(&trackingClkStruct);
    }
}

/*!
 Set the broadcast clock.

 Public function defined in csf.h
 */
void Csf_setBroadcastClock(uint32_t broadcastTime)
{
    /* Stop the Tracking timer */
    if(Timer_isActive(&broadcastClkStruct) == true)
    {
        Timer_stop(&broadcastClkStruct);
    }

    if(broadcastTime)
    {
        /* Setup timer */
        Timer_setTimeout(broadcastClkHandle, broadcastTime);
        Timer_start(&broadcastClkStruct);
    }
}

/*!
 Set the trickle clock.

 Public function defined in csf.h
 */
void Csf_setTrickleClock(uint32_t trickleTime, uint8_t frameType)
{
    uint16_t randomNum = 0;
    uint16_t randomTime = 0;

    if(trickleTime > 0)
    {
        randomNum = ((ApiMac_randomByte() << 8) + ApiMac_randomByte());
        randomTime = (trickleTime >> 1) +
                      (randomNum % (trickleTime >> 1));
    }

    if(frameType == ApiMac_wisunAsyncFrame_advertisement)
    {
        /* Stop the PA trickle timer */
        if(Timer_isActive(&tricklePAClkStruct) == true)
        {
            Timer_stop(&tricklePAClkStruct);
        }

        if(trickleTime > 0)
        {
            /* Setup timer */
            Timer_setTimeout(tricklePAClkHandle, randomTime);
            Timer_start(&tricklePAClkStruct);
        }
    }
    else if(frameType == ApiMac_wisunAsyncFrame_config)
    {
        /* Stop the PC trickle timer */
        if(Timer_isActive(&tricklePCClkStruct) == true)
        {
            Timer_stop(&tricklePCClkStruct);
        }

        if(trickleTime > 0)
        {
            /* Setup timer */
            Timer_setTimeout(tricklePCClkHandle, randomTime);
            Timer_start(&tricklePCClkStruct);
        }
    }
}

/*!
 Set the clock join permit attribute.

 Public function defined in csf.h
 */
void Csf_setJoinPermitClock(uint32_t joinDuration)
{
    /* Stop the join timer */
    if(Timer_isActive(&joinClkStruct) == true)
    {
        Timer_stop(&joinClkStruct);
    }

    if(joinDuration != 0)
    {
        /* Setup timer */
        Timer_setTimeout(joinClkHandle, joinDuration);
        Timer_start(&joinClkStruct);
    }
}

/*!
 Set the clock config request delay.

 Public function defined in csf.h
 */
void Csf_setConfigClock(uint32_t delay)
{
    /* Stop the join timer */
    if(Timer_isActive(&configClkStruct) == true)
    {
        Timer_stop(&configClkStruct);
    }

    if(delay != 0)
    {
        /* Setup timer */
        Timer_setTimeout(configClkHandle, delay);
        Timer_start(&configClkStruct);
    }
}

/*!
 Read the number of device list items stored

 Public function defined in csf.h
 */
uint16_t Csf_getNumDeviceListEntries(void)
{
    uint16_t numEntries = 0;

    if(pNV != NULL)
    {
        NVINTF_itemID_t id;
        uint8_t stat;

        /* Setup NV ID for the number of entries in the device list */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_DEVICELIST_ENTRIES_ID;
        id.subID = 0;

        /* Read the number of device list items from NV */
        stat = pNV->readItem(id, 0, sizeof(uint16_t), &numEntries);
        if(stat != NVINTF_SUCCESS)
        {
            numEntries = 0;
        }
    }
    return (numEntries);
}

/*!
 Find the short address from a given extended address

 Public function defined in csf.h
 */
uint16_t Csf_getDeviceShort(ApiMac_sAddrExt_t *pExtAddr)
{
    Llc_deviceListItem_t item;
    ApiMac_sAddr_t devAddr;
    uint16_t shortAddr =INVALID_SHORT_ADDR;

    devAddr.addrMode = ApiMac_addrType_extended;
    memcpy(&devAddr.addr.extAddr, pExtAddr, sizeof(ApiMac_sAddrExt_t));

    if(Csf_getDevice(&devAddr,&item))
    {
        shortAddr = item.devInfo.shortAddress;
    }

    return(shortAddr);
}

/*!
 Find entry in device list

 Public function defined in csf.h
 */
bool Csf_getDevice(ApiMac_sAddr_t *pDevAddr, Llc_deviceListItem_t *pItem)
{
    if((pNV != NULL) && (pItem != NULL))
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;
            uint8_t stat;
            int subId = 0;
            int readItems = 0;

            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;

            while((readItems < numEntries) && (subId
                                               < CSF_MAX_DEVICELIST_IDS))
            {
                Llc_deviceListItem_t item;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_SUCCESS)
                {
                    if(((pDevAddr->addrMode == ApiMac_addrType_short)
                        && (pDevAddr->addr.shortAddr
                            == item.devInfo.shortAddress))
                       || ((pDevAddr->addrMode == ApiMac_addrType_extended)
                           && (memcmp(&pDevAddr->addr.extAddr,
                                      &item.devInfo.extAddress,
                                      (APIMAC_SADDR_EXT_LEN))
                               == 0)))
                    {
                        memcpy(pItem, &item, sizeof(Llc_deviceListItem_t));
                        return (true);
                    }
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (false);
}

/*!
 Find entry in device list

 Public function defined in csf.h
 */
bool Csf_getDeviceItem(uint16_t devIndex, Llc_deviceListItem_t *pItem)
{
    if((pNV != NULL) && (pItem != NULL))
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;
            uint8_t stat;
            int subId = 0;
            int readItems = 0;

            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;

            while((readItems < numEntries) && (subId
                                               < CSF_MAX_DEVICELIST_IDS))
            {
                Llc_deviceListItem_t item;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_SUCCESS)
                {
                    if(readItems == devIndex)
                    {
                        memcpy(pItem, &item, sizeof(Llc_deviceListItem_t));
                        return (true);
                    }
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (false);
}

/*!
 Csf implementation for memory allocation

 Public function defined in csf.h
 */
void *Csf_malloc(uint16_t size)
{
    return(malloc(size));
}

/*!
 Csf implementation for memory de-allocation

 Public function defined in csf.h
 */
void Csf_free(void *ptr)
{
    if(ptr != NULL)
    {
        free(ptr);
    }
}

/*!
 Update the Frame Counter

 Public function defined in csf.h
 */
void Csf_updateFrameCounter(ApiMac_sAddr_t *pDevAddr, uint32_t frameCntr)
{
    if((pNV != NULL) && (pNV->writeItem != NULL))
    {
        if(pDevAddr == NULL)
        {
            /* Update this device's frame counter */
            if((frameCntr >=
                (lastSavedCoordinatorFrameCounter + FRAME_COUNTER_SAVE_WINDOW)))
            {
                NVINTF_itemID_t id;

                /* Setup NV ID */
                id.systemID = NVINTF_SYSID_APP;
                id.itemID = CSF_NV_FRAMECOUNTER_ID;
                id.subID = 0;

                /* Write the NV item */
                if(pNV->writeItem(id, sizeof(uint32_t), &frameCntr)
                                == NVINTF_SUCCESS)
                {
                    lastSavedCoordinatorFrameCounter = frameCntr;
                }
            }
        }
        else
        {
            /* Child frame counter update */
            Llc_deviceListItem_t devItem;

            /* Is the device in our database? */
            if(Csf_getDevice(pDevAddr, &devItem))
            {
                /*
                 Don't save every update, only save if the new frame
                 counter falls outside the save window.
                 */
                if((devItem.rxFrameCounter + FRAME_COUNTER_SAVE_WINDOW)
                                <= frameCntr)
                {
                    /* Update the frame counter */
                    devItem.rxFrameCounter = frameCntr;
                    updateDeviceListItem(&devItem);
                }
            }
        }
    }
}

/*!
 Get the Frame Counter

 Public function defined in csf.h
 */
bool Csf_getFrameCounter(ApiMac_sAddr_t *pDevAddr, uint32_t *pFrameCntr)
{
    /* Check for valid pointer */
    if(pFrameCntr != NULL)
    {
        /*
         A pDevAddr that is null means to get the frame counter for this device
         */
        if(pDevAddr == NULL)
        {
            if((pNV != NULL) && (pNV->readItem != NULL))
            {
                NVINTF_itemID_t id;

                /* Setup NV ID */
                id.systemID = NVINTF_SYSID_APP;
                id.itemID = CSF_NV_FRAMECOUNTER_ID;
                id.subID = 0;

                /* Read Network Information from NV */
                if(pNV->readItem(id, 0, sizeof(uint32_t), pFrameCntr)
                                == NVINTF_SUCCESS)
                {
                    /* Set to the next window */
                    *pFrameCntr += FRAME_COUNTER_SAVE_WINDOW;
                    return(true);
                }
                else
                {
                    /*
                     Wasn't found, so write 0, so the next time it will be
                     greater than 0
                     */
                    uint32_t fc = 0;

                    /* Setup NV ID */
                    id.systemID = NVINTF_SYSID_APP;
                    id.itemID = CSF_NV_FRAMECOUNTER_ID;
                    id.subID = 0;

                    /* Write the NV item */
                    pNV->writeItem(id, sizeof(uint32_t), &fc);
                }
            }
        }

        *pFrameCntr = 0;
    }
    return (false);
}


/*!
 Delete an entry from the device list

 Public function defined in csf.h
 */
void Csf_removeDeviceListItem(ApiMac_sAddrExt_t *pAddr)
{
    if((pNV != NULL) && (pNV->deleteItem != NULL))
    {
        int index;

        /* Does the item exist? */
        index = findDeviceListIndex(pAddr);
        if(index != DEVICE_INDEX_NOT_FOUND)
        {
            uint8_t stat;
            NVINTF_itemID_t id;

            /* Setup NV ID for the device list record */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;
            id.subID = (uint16_t)index;

            stat = pNV->deleteItem(id);
            if(stat == NVINTF_SUCCESS)
            {
                /* Update the number of entries */
                uint16_t numEntries = Csf_getNumDeviceListEntries();
                if(numEntries > 0)
                {
                    numEntries--;
                    saveNumDeviceListEntries(numEntries);
                }
            }
        }
    }
}

/*!
 Assert Indication

 Public function defined in csf.h
 */
void Csf_assertInd(uint8_t reason)
{
#if defined(MT_CSF)
    if((pNV != NULL) && (pNV->writeItem != NULL))
    {
        /* Attempt to save reason to read after reset */
        (void)pNV->writeItem(nvResetId, 1, &reason);
    }
#endif
}

/*!
 Clear all the NV Items

 Public function defined in csf.h
 */
void Csf_clearAllNVItems(void)
{
    if((pNV != NULL) && (pNV->deleteItem != NULL))
    {
        NVINTF_itemID_t id;
        uint16_t entries;

        /* Clear Network Information */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_NETWORK_INFO_ID;
        id.subID = 0;
        pNV->deleteItem(id);

        /* Clear the black list entries number */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_BLACKLIST_ENTRIES_ID;
        id.subID = 0;
        pNV->deleteItem(id);

        /*
         Clear the black list entries.  Brute force through
         every possible subID, if it doesn't exist that's fine,
         it will fail in deleteItem.
         */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_BLACKLIST_ID;
        for(entries = 0; entries < CSF_MAX_BLACKLIST_IDS; entries++)
        {
            id.subID = entries;
            pNV->deleteItem(id);
        }

        /* Clear the device list entries number */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_DEVICELIST_ENTRIES_ID;
        id.subID = 0;
        pNV->deleteItem(id);

        /*
         Clear the device list entries.  Brute force through
         every possible subID, if it doesn't exist that's fine,
         it will fail in deleteItem.
         */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_DEVICELIST_ID;
        for(entries = 0; entries < CSF_MAX_DEVICELIST_IDS; entries++)
        {
            id.subID = entries;
            pNV->deleteItem(id);
        }

        /* Clear the device tx frame counter */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_FRAMECOUNTER_ID;
        id.subID = 0;
        pNV->deleteItem(id);
    }
}


/*!
 Add an entry into the black list

 Public function defined in csf.h
 */
bool Csf_addBlackListItem(ApiMac_sAddr_t *pAddr)
{
    bool retVal = false;

    if((pNV != NULL) && (pAddr != NULL)
       && (pAddr->addrMode != ApiMac_addrType_none))
    {
        if(findBlackListIndex(pAddr))
        {
            retVal = true;
        }
        else
        {
            uint8_t stat;
            NVINTF_itemID_t id;
            uint16_t numEntries = getNumBlackListEntries();

            /* Check the maximum size */
            if(numEntries < CSF_MAX_BLACKLIST_ENTRIES)
            {
                /* Setup NV ID for the black list record */
                id.systemID = NVINTF_SYSID_APP;
                id.itemID = CSF_NV_BLACKLIST_ID;
                id.subID = (uint16_t)findUnusedBlackListIndex();

                /* write the black list record */
                stat = pNV->writeItem(id, sizeof(ApiMac_sAddr_t), pAddr);
                if(stat == NVINTF_SUCCESS)
                {
                    /* Update the number of entries */
                    numEntries++;
                    saveNumBlackListEntries(numEntries);
                    retVal = true;
                }
            }
        }
    }

    return (retVal);
}

/*!
 Check if config timer is active

 Public function defined in csf.h
 */
bool Csf_isConfigTimerActive(void)
{
    return(Timer_isActive(&configClkStruct));
}

/*!
 Check if tracking timer is active

 Public function defined in csf.h
 */
bool Csf_isTrackingTimerActive(void)
{
    return(Timer_isActive(&trackingClkStruct));
}
/******************************************************************************
 Local Functions
 *****************************************************************************/

/*!
 * @brief       Tracking timeout handler function.
 *
 * @param       a0 - ignored
 */
static void processTackingTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */

    triggerCollectorEvt(COLLECTOR_TRACKING_TIMEOUT_EVT);

}

/*!
 * @brief       Tracking timeout handler function.
 *
 * @param       a0 - ignored
 */
static void processBroadcastTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */

    triggerCollectorEvt(COLLECTOR_BROADCAST_TIMEOUT_EVT);

}

/*!
 * @brief       Join permit timeout handler function.
 *
 * @param       a0 - ignored
 */
static void processJoinTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */
    triggerCllcEvt(CLLC_JOIN_EVT);
}

/*!
 * @brief       Config delay timeout handler function.
 *
 * @param       a0 - ignored
 */
static void processConfigTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */

    triggerCollectorEvt(COLLECTOR_CONFIG_EVT);
}

/*!
 * @brief       Trickle timeout handler function for PA .
 *
 * @param       a0 - ignored
 */
static void processPATrickleTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */
    triggerCllcEvt(CLLC_PA_EVT);
}

/*!
 * @brief       Trickle timeout handler function for PC.
 *
 * @param       a0 - ignored
 */
static void processPCTrickleTimeoutCallback(UArg a0)
{
    (void)a0; /* Parameter is not used */
    triggerCllcEvt(CLLC_PC_EVT);
}



/*!
 * @brief       Add an entry into the device list
 *
 * @param       pItem - pointer to the device list entry
 *
 * @return      true if added or already existed, false if problem
 */
static bool addDeviceListItem(Llc_deviceListItem_t *pItem)
{
    bool retVal = false;

    if((pNV != NULL) && (pItem != NULL))
    {
        if(findDeviceListIndex(&pItem->devInfo.extAddress)
                        != DEVICE_INDEX_NOT_FOUND)
        {
            retVal = true;
        }
        else
        {
            uint8_t stat;
            NVINTF_itemID_t id;
            uint16_t numEntries = Csf_getNumDeviceListEntries();

            /* Check the maximum size */
            if(numEntries < CSF_MAX_DEVICELIST_ENTRIES)
            {
                /* Setup NV ID for the device list record */
                id.systemID = NVINTF_SYSID_APP;
                id.itemID = CSF_NV_DEVICELIST_ID;
                id.subID = (uint16_t)findUnusedDeviceListIndex();

                /* write the device list record */
                stat = pNV->writeItem(id, sizeof(Llc_deviceListItem_t), pItem);
                if(stat == NVINTF_SUCCESS)
                {
                    /* Update the number of entries */
                    numEntries++;
                    saveNumDeviceListEntries(numEntries);
                    retVal = true;
                }
            }
        }
    }

    return (retVal);
}

/*!
 * @brief       Update an entry in the device list
 *
 * @param       pItem - pointer to the device list entry
 */
static void updateDeviceListItem(Llc_deviceListItem_t *pItem)
{
    if((pNV != NULL) && (pItem != NULL))
    {
        int idx;

        idx = findDeviceListIndex(&pItem->devInfo.extAddress);
        if(idx != DEVICE_INDEX_NOT_FOUND)
        {
            NVINTF_itemID_t id;

            /* Setup NV ID for the device list record */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;
            id.subID = (uint16_t)idx;

            /* write the device list record */
            pNV->writeItem(id, sizeof(Llc_deviceListItem_t), pItem);
        }
    }
}

/*!
 * @brief       Find entry in device list
 *
 * @param       pAddr - address to of device to find
 *
 * @return      sub index into the device list, -1 (DEVICE_INDEX_NOT_FOUND)
 *              if not found
 */
static int findDeviceListIndex(ApiMac_sAddrExt_t *pAddr)
{
    if((pNV != NULL) && (pAddr != NULL))
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;
            uint8_t stat;
            int subId = 0;
            int readItems = 0;

            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;

            while((readItems < numEntries) && (subId
                                               < CSF_MAX_DEVICELIST_IDS))
            {
                Llc_deviceListItem_t item;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_SUCCESS)
                {
                    /* Is the address the same */
                    if(memcmp(pAddr, &item.devInfo.extAddress,
                              (APIMAC_SADDR_EXT_LEN))
                       == 0)
                    {
                        return (subId);
                    }
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (DEVICE_INDEX_NOT_FOUND);
}

/*!
 * @brief       Find an unused device list index
 *
 * @return      index that is not in use
 */
static int findUnusedDeviceListIndex(void)
{
    int subId = 0;

    if(pNV != NULL)
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;

            int readItems = 0;
            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;

            while((readItems < numEntries) && (subId
                                               < CSF_MAX_DEVICELIST_IDS))
            {
                Llc_deviceListItem_t item;
                uint8_t stat;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_NOTFOUND)
                {
                    /* Use this sub id */
                    break;
                }
                else if(stat == NVINTF_SUCCESS)
                {
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (subId);
}

/*!
 * @brief       Read the number of device list items stored
 *
 * @param       numEntries - number of entries in the device list
 */
static void saveNumDeviceListEntries(uint16_t numEntries)
{
    if(pNV != NULL)
    {
        NVINTF_itemID_t id;

        /* Setup NV ID for the number of entries in the device list */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_DEVICELIST_ENTRIES_ID;
        id.subID = 0;

        /* Read the number of device list items from NV */
        pNV->writeItem(id, sizeof(uint16_t), &numEntries);
    }
}

/*!
 * @brief       Find entry in black list
 *
 * @param       pAddr - address to add into the black list
 *
 * @return      sub index into the blacklist, -1 if not found
 */
static int findBlackListIndex(ApiMac_sAddr_t *pAddr)
{
    if((pNV != NULL) && (pAddr != NULL)
       && (pAddr->addrMode != ApiMac_addrType_none))
    {
        uint16_t numEntries;

        numEntries = getNumBlackListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;
            uint8_t stat;
            int subId = 0;
            int readItems = 0;

            /* Setup NV ID for the black list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_BLACKLIST_ID;

            while((readItems < numEntries) && (subId < CSF_MAX_BLACKLIST_IDS))
            {
                ApiMac_sAddr_t item;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(ApiMac_sAddr_t), &item);
                if(stat == NVINTF_SUCCESS)
                {
                    if(pAddr->addrMode == item.addrMode)
                    {
                        /* Is the address the same */
                        if(((pAddr->addrMode == ApiMac_addrType_short)
                            && (pAddr->addr.shortAddr == item.addr.shortAddr))
                           || ((pAddr->addrMode == ApiMac_addrType_extended)
                               && (memcmp(&pAddr->addr.extAddr,
                                          &item.addr.extAddr,
                                          APIMAC_SADDR_EXT_LEN)
                                   == 0)))
                        {
                            return (subId);
                        }
                    }
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (-1);
}

/*!
 * @brief       Find an unused blacklist index
 *
 * @return      index that is not in use
 */
static int findUnusedBlackListIndex(void)
{
    int subId = 0;

    if(pNV != NULL)
    {
        uint16_t numEntries;

        numEntries = getNumBlackListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;

            int readItems = 0;
            /* Setup NV ID for the black list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_BLACKLIST_ID;

            while((readItems < numEntries) && (subId < CSF_MAX_BLACKLIST_IDS))
            {
                ApiMac_sAddr_t item;
                uint8_t stat;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(ApiMac_sAddr_t), &item);
                if(stat == NVINTF_NOTFOUND)
                {
                    /* Use this sub id */
                    break;
                }
                else if(stat == NVINTF_SUCCESS)
                {
                    readItems++;
                }
                subId++;
            }
        }
    }

    return (subId);
}

/*!
 * @brief       Read the number of black list items stored
 *
 * @return      number of entries in the black list
 */
static uint16_t getNumBlackListEntries(void)
{
    uint16_t numEntries = 0;

    if(pNV != NULL)
    {
        NVINTF_itemID_t id;
        uint8_t stat;

        /* Setup NV ID for the number of entries in the black list */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_BLACKLIST_ENTRIES_ID;
        id.subID = 0;

        /* Read the number of black list items from NV */
        stat = pNV->readItem(id, 0, sizeof(uint16_t), &numEntries);
        if(stat != NVINTF_SUCCESS)
        {
            numEntries = 0;
        }
    }
    return (numEntries);
}

/*!
 * @brief       Read the number of black list items stored
 *
 * @param       numEntries - number of entries in the black list
 */
static void saveNumBlackListEntries(uint16_t numEntries)
{
    if(pNV != NULL)
    {
        NVINTF_itemID_t id;

        /* Setup NV ID for the number of entries in the black list */
        id.systemID = NVINTF_SYSID_APP;
        id.itemID = CSF_NV_BLACKLIST_ENTRIES_ID;
        id.subID = 0;

        /* Read the number of black list items from NV */
        pNV->writeItem(id, sizeof(uint16_t), &numEntries);
    }
}

/*!
 * @brief       Delete an address from the black list
 *
 * @param       pAddr - address to remove from black list.
 */
void removeBlackListItem(ApiMac_sAddr_t *pAddr)
{
    if(pNV != NULL)
    {
        int index;

        /* Does the item exist? */
        index = findBlackListIndex(pAddr);
        if(index > 0)
        {
            uint8_t stat;
            NVINTF_itemID_t id;

            /* Setup NV ID for the black list record */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_BLACKLIST_ID;
            id.subID = (uint16_t)index;

            stat = pNV->deleteItem(id);
            if(stat == NVINTF_SUCCESS)
            {
                /* Update the number of entries */
                uint16_t numEntries = getNumBlackListEntries();
                if(numEntries > 0)
                {
                    numEntries--;
                    saveNumBlackListEntries(numEntries);
                }
            }
        }
    }
}

#if defined(TEST_REMOVE_DEVICE)
/*!
 * @brief       This is an example function on how to remove a device
 *              from this network.
 */
static void removeTheFirstDevice(void)
{
    if(pNV != NULL)
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;
            uint16_t subId = 0;

            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;

            while(subId < CSF_MAX_DEVICELIST_IDS)
            {
                Llc_deviceListItem_t item;
                uint8_t stat;

                id.subID = (uint16_t)subId;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_SUCCESS)
                {
                    /* Found the first device in the list */
                    ApiMac_sAddr_t addr;

                    /* Send a disassociate to the device */
                    Cllc_sendDisassociationRequest(item.devInfo.shortAddress,
                                                   item.capInfo.rxOnWhenIdle);
                    /* remove device from the NV list */
                    Cllc_removeDevice(&item.devInfo.extAddress);

                    /* Remove it from the Device list */
                    Csf_removeDeviceListItem(&item.devInfo.extAddress);

                    /* Add the device to the black list so it can't join again */
                    addr.addrMode = ApiMac_addrType_extended;
                    memcpy(&addr.addr.extAddr, &item.devInfo.extAddress,
                           (APIMAC_SADDR_EXT_LEN));
                    Csf_addBlackListItem(&addr);
                    break;
                }
                subId++;
            }
        }
    }
}
#else
/*!
 * @brief       Retrieve the first device's short address
 *
 * @return      short address or 0xFFFF if not found
 */
static uint16_t getTheFirstDevice(void)
{
    uint16_t found = CSF_INVALID_SHORT_ADDR;
    if(pNV != NULL)
    {
        uint16_t numEntries;

        numEntries = Csf_getNumDeviceListEntries();

        if(numEntries > 0)
        {
            NVINTF_itemID_t id;

            /* Setup NV ID for the device list records */
            id.systemID = NVINTF_SYSID_APP;
            id.itemID = CSF_NV_DEVICELIST_ID;
            id.subID = 0;

            while(id.subID < CSF_MAX_DEVICELIST_IDS)
            {
                Llc_deviceListItem_t item;
                uint8_t stat;

                /* Read Network Information from NV */
                stat = pNV->readItem(id, 0, sizeof(Llc_deviceListItem_t),
                                     &item);
                if(stat == NVINTF_SUCCESS)
                {
                    found = item.devInfo.shortAddress;
                    break;
                }
                id.subID++;
            }
        }
    }
    return(found);
}
#endif
