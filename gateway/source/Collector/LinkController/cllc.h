/******************************************************************************

 @file cllc.h

 @brief Coordinator Logical Link Controller

 Group: WCS LPC
 Target Device: CC13xx

 ******************************************************************************

 Copyright (c) 2016, Texas Instruments Incorporated
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
 Release Name: simplelink_cc13x0_sdk_1_00_00_13"
 Release Date: 2016-11-21 18:05:40
 *****************************************************************************/
#ifndef CLLC_H
#define CLLC_H

/******************************************************************************
 Includes
 *****************************************************************************/


#ifdef __cplusplus
extern "C"
{
#endif
#include <API_MAC/api_mac.h>
#include <Collector/config.h>
/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define INVALID_SHORT_ADDR   0xFFFF

/*! Event ID - LLC Event for PA Async command */
#define CLLC_PA_EVT             0x0001
/*! Event ID - LLC Event for PC Async command */
#define CLLC_PC_EVT             0x0002
/*! Event ID - LLC Event for Join Permit duration */
#define CLLC_JOIN_EVT           0x0004
/*! Event ID - State change event */
#define CLLC_STATE_CHANGE_EVT_EVT   0x0008

/*! Association status */
#define CLLC_ASSOC_STATUS_ALIVE 0x0001

/*!
 Coordinator State Values
 */
typedef enum
{
    /*! Powered up, not started and waiting for user to start */
    Cllc_states_initWaiting,
    /*! Starting coordinator, scanning and selecting the best parameters */
    Cllc_states_startingCoordinator,
    /*!
     Powered up, found network information, and restoring device in network
     */
    Cllc_states_initRestoringCoordinator,
    /*! Device is operating as coordinator */
    Cllc_states_started,
    /*! Device is restored as coordinator in the network */
    Cllc_states_restored,
    /*! Joining allowed state has changed to allowed */
    Cllc_states_joiningAllowed,
    /*! Joining allowed state has changed to not allowed */
    Cllc_states_joiningNotAllowed
} Cllc_states_t;

/*!
 Coordinator starting State Values
 */
typedef enum
{
    /*! Initialized state */
    Cllc_coordStates_initialized = 0,
    /*! MAC  coordinator is performing a active scan  */
    Cllc_coordStates_scanActive,
    /*! active scan  confirm received*/
    Cllc_coordStates_scanActiveCnf,
    /*! Energy detect scan confirm received */
    Cllc_coordStates_scanEdCnf,
    /*! Start confirm received */
    Cllc_coordStates_startCnf
} Cllc_coord_states_t;

/*! Building block for association table */
typedef struct
{
    /*! Short address of associated device */
    uint16_t shortAddr;
    uint8_t extAddr[8];
    /*! capability information */
    ApiMac_capabilityInfo_t capInfo;
    /*! RSSI */
    int8_t rssi;
    /*! Device alive status */
    uint16_t status;
} Cllc_associated_devices_t;

/*! Cllc statistics */
typedef struct
{
    /*! number of PA Solicit messages received */
    uint32_t fhNumPASolicitReceived;
    /*! number of PA messages sent */
    uint32_t fhNumPASent;
    /*! number of PC Solicit messages received */
    uint32_t fhNumPANConfigSolicitsReceived;
    /*! number of PC messages sent */
    uint32_t fhNumPANConfigSent;
    uint32_t otherStats;
} Cllc_statistics_t;

/*! Association table */
extern Cllc_associated_devices_t Cllc_associatedDevList[CONFIG_MAX_DEVICES];
/*! Cllc statistics */
extern Cllc_statistics_t Cllc_statistics;

/*! Cllc events flags */
extern uint16_t Cllc_events;

/******************************************************************************
 Structures - Building blocks for the Coordinator LLC
 *****************************************************************************/

/*!
 Coordinator started callback.
 */
typedef void (*Cllc_startedFp_t)(Llc_netInfo_t *pStartedInfo);

/*!
 Device joining callback - Indication that a device is joining.  pDevInfo
 is a pointer to the device's information and capInfo is the device's
 capabilities.  The application will determine if the device can join by
 the return value - ApiMac_assocStatus_t.  If this callback is ignored,
 callback pointer is NULL, all devices will be allowed to join, assuming
 that associate permit is true.
 */
typedef ApiMac_assocStatus_t (*Cllc_deviceJoiningFp_t)(
                ApiMac_deviceDescriptor_t *pDevInfo,
                ApiMac_capabilityInfo_t *pCapInfo);

/*!
 State Changed callback - This indication says that the CLLC's state
 has changed.
 */
typedef void (*Cllc_stateChangedFp_t)(Cllc_states_t state);

/*!
 Structure containing all the CLLC callbacks (indications).
 To recieve the callback fill in the structure item with a pointer
 to the function that will handle that callback.  To ignore a callback
 set that function pointer to NULL.
 */
typedef struct _Cllc_callbacks_t
{
    /*! Coordinator Started Indication callback */
    Cllc_startedFp_t pStartedCb;
    /*! Device joining callback */
    Cllc_deviceJoiningFp_t pDeviceJoiningCb;
    /*! The state has changed callback */
    Cllc_stateChangedFp_t pStateChangeCb;
} Cllc_callbacks_t;

/******************************************************************************
 Function Prototypes
 *****************************************************************************/

extern void triggerCollectorEvt(uint16_t evt);
extern void triggerCllcEvt(uint16_t evt);
extern void triggerCsfEvt(uint16_t evt);

/*!
 * @brief       Find entry in device list
 *
 * @param       size - number of bytes to allocate from heap
 *
 * @return      true if found, false if not
 */
extern void *Csf_malloc(uint16_t size);


/*!
 * @brief       Csf implementation for memory de-allocation
 *
 * @param       ptr - a valid pointer to the memory to free
 */
extern void Csf_free(void *ptr);

/*!
 * @brief       Update the Frame Counter
 *
 * @param       pDevAddr - pointer to device's address. If this pointer
 *                         is NULL, it means that this is the frame counter
 *                         for this device.
 * @param       frameCntr -  valur of frame counter
 */
extern void Csf_updateFrameCounter(ApiMac_sAddr_t *pDevAddr,
                                   uint32_t frameCntr);

/*!
 * @brief       Set trickle clock
 *
 * @param       trickleTime - duration of trickle timer( in msec)
 * @param       frameType - type of Async frame
 */
extern void Csf_setTrickleClock(uint32_t trickleTime, uint8_t frameType);


/*!
 * @brief       Set Join Permit clock
 *
 * @param       joinDuration - duration for which join permit is TRUE( in msec)
 */
extern void Csf_setJoinPermitClock(uint32_t joinDuration);

/*!
 * @brief       Delete an entry from the device list
 *
 * @param       pAddr - address to remove from device list.
 */
extern void Csf_removeDeviceListItem(ApiMac_sAddrExt_t *pAddr);

/*!
 * @brief       Initialize the trickle timer clock
 */
extern void Csf_initializeTrickleClock(void);

/*!
 * @brief       Initialize the clock setting join permit duration
 */
extern void Csf_initializeJoinPermitClock(void);

/*!
 * @brief       Find the short address from a given extended address
 *
 * @param       pExtAddr - extended address
 *
 * @return      CSF_INVALID_SHORT_ADDR if not found, otherwise the short address
 */
extern uint16_t Csf_getDeviceShort(ApiMac_sAddrExt_t *pExtAddr);

/*!
 * @brief       Find entry in device list from an address
 *
 * @param       pDevAddr - device address
 * @param       pItem - place to put the device information
 *
 * @return      true if found, false if not
 */
extern bool Csf_getDevice(ApiMac_sAddr_t *pDevAddr, Llc_deviceListItem_t *pItem);

/*!
 * @brief       Initialize this module and update the callback structure.
 *              <BR>
 *              This function will take the application's MAC callback
 *              structure and override some callbacks with its own callbacks.
 *              The application callbacks that are overridden are saved so that
 *              they can be chain called when processing the callbacks.
 *
 * @param       pMacCbs - pointer to the API MAC callback structure
 * @param       pCllcCbs - pointer to the CLLC callback structure
 */
extern void Cllc_init(ApiMac_callbacks_t *pMacCbs, Cllc_callbacks_t *pCllcCbs);

/*!
 * @brief       Application task processing.
 *              <BR>
 *              This function handles the events in CLLC for trickle timer,
 *              controlling associate permit flag and transitioning between
 *              various states during operation.
 */
extern void Cllc_process(void);

/*!
 * @brief       Start a network.
 *              <BR>
 *              The application will call this function to start the process
 *              of making this device a coordinator of a new network.
 *              <BR>
 *              This module will automatically determine the best channel(s)
 *              and PANID, before starting.
 */
extern void Cllc_startNetwork(void);

/*!
 * @brief       Restore the coodinator in the network.
 *              <BR>
 *              The application will call this function to restore the
 *              coordinator in the network by passing all the network
 *              information needed to restore the device.
 *              <BR>
 *              This module will configure the MAC with all the network
 *              information then start the coordinator without scanning.
 *
 * @param       pNetworkInfo - network information
 * @param       numDevices - number of devices in association table
 * @param       pDevList - list of devices
 */
extern void Cllc_restoreNetwork(Llc_netInfo_t *pNetworkInfo, uint8_t numDevices,
		Llc_deviceListItem_t *pDevList);
/*!
 * @brief       Remove device from the network.
 *              <BR>
 *              The application will call this function to remove a device
 *              from the network by passing the extended address of the
 *              device.
 *              <BR>
 *
 * @param       pExtAddr - pointer to extended address of the device
 */
extern void Cllc_removeDevice(ApiMac_sAddrExt_t *pExtAddr);

/*!
 * @brief       Set Join Permit PIB value.
 *              <BR>
 *              The application will call this function to set or clear
 *              <BR>
 *
 * @param       duration - duration for join permit to be turned on in
 *              milliseconds.
 *              0 sets it Off, 0xFFFFFFFF sets it ON indefinitely
 *              Any other non zero value sets it on for that duration
 *
 * @return      ApiMac_status_t - ApiMac_status_success for successful
 *                                operation
 */
extern ApiMac_status_t Cllc_setJoinPermit(uint32_t duration);

/*!
 * @brief       API for app to set disassociation request.
 */
extern void Cllc_sendDisassociationRequest(uint16_t shortAddr,bool rxOnIdle);

/*!
 * @brief       Initialize the MAC Security
 *
 * @param       frameCounter - Initial frame counter
 */
extern void Cllc_securityInit(uint32_t frameCounter);

/*!
 * @brief       Fill in the security structure
 *
 * @param       pSec - pointer to security structure
 */
extern void Cllc_securityFill(ApiMac_sec_t *pSec);

/*!
 * @brief       Check the security level against expected level
 *
 * @param       pSec - pointer to security structure
 *
 * @return      true is matches expected security level, false if not
 */
extern bool Cllc_securityCheck(ApiMac_sec_t *pSec);

/*!
 * @brief      Add a device to the MAC security device table.
 *
 * @param      panID - PAN ID
 * @param      shortAddr - short address of the device
 * @param      pExtAddr - pointer to the extended address
 * @param      frameCounter - starting frame counter
 *
 * @return     status returned by ApiMac_secAddDevice()
 */
extern ApiMac_status_t Cllc_addSecDevice(uint16_t panID,
                                              uint16_t shortAddr,
                                              ApiMac_sAddrExt_t *pExtAddr,
                                              uint32_t frameCounter);

//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
}
#endif

#endif /* CLLC_H */

