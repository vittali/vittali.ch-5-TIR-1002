/******************************************************************************

 @file api_mac.c

 @brief TIMAC 2.0 API

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

/******************************************************************************
 Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Utils/util.h>
#include <NPI/npiParse.h>
#include <NPIcmds/mtMac.h>
#include <NPIcmds/mtUtil.h>
#include "api_mac.h"

/*!
 This module is the ICall interface for the application and all ICall
 activity must go through this module, no ICall activity anywhere else.
 */

/******************************************************************************
 Constants and definitions
 *****************************************************************************/

/*! Capability Information - Device is capable of becoming a PAN coordinator */
#define CAPABLE_PAN_COORD       0x01
/*! Capability Information - Device is an FFD  */
#define CAPABLE_FFD             0x02
/*!
 Capability Information - Device is mains powered rather than battery powered
 */
#define CAPABLE_MAINS_POWER     0x04
/*! Capability Information - Device has its receiver on when idle  */
#define CAPABLE_RX_ON_IDLE      0x08
/*!
 Capability Information - Device is capable of sending
 and receiving secured frames
 */
#define CAPABLE_SECURITY        0x40
/*!
 Capability Information - Request allocation of a short address in the
 associate procedure
 */
#define CAPABLE_ALLOC_ADDR      0x80

/*! Offset into the payload for the payload IEs */
#define PAYLOAD_IE_OFFSET                    0
/*! Offset into the IE for the subIE */
#define PAYLOAD_IE_SUBIE_OFFSET              0

/*! Macro to get the IE Type */
#define PAYLOAD_IE_TYPE(p) (((p)[PAYLOAD_IE_OFFSET+1] >> 7) & 0x01)

/*! Macro to get the IE Group ID */
#define PAYLOAD_IE_GROUP_ID(p) ((((uint8_t *)p)[PAYLOAD_IE_OFFSET+1] >> 3) & 0x0f)

/*! Macro to get the IE Content Length */
#define PAYLOAD_IE_CONTENT_LEN(p) ((((uint8_t *)p)[PAYLOAD_IE_OFFSET+0] & 0x00ff) +\
                ((((uint8_t *)p)[PAYLOAD_IE_OFFSET+1] & 0x0007) << 8))

/*! Type value for payload IE */
#define PAYLOAD_IE_TYPE_VAL 1
/*! Type value for payload IE */
#define PAYLOAD_IE_HEADER_LEN 2

/*! Macro to get the short subIE length */
#define PAYLOAD_IE_SHORT_SUBIE_LEN(p) ((p)[PAYLOAD_IE_SUBIE_OFFSET+0])

/*! Macro to get the long subIE length */
#define PAYLOAD_IE_LONG_SUBIE_LEN(p) (((p)[PAYLOAD_IE_OFFSET+0] & 0x00ff) + \
                              (((p)[PAYLOAD_IE_OFFSET+1] & 0x0007)  << 8))

/*! Macro to get the subIE type */
#define PAYLOAD_IE_SUBIE_TYPE(p) (((p)[PAYLOAD_IE_SUBIE_OFFSET+1] >> 7) & 0x01)

/*! Macro to get the subIE ID */
#define PAYLOAD_IE_SUBIE_ID(p) ((p)[PAYLOAD_IE_SUBIE_OFFSET+1] & 0x7f)

/*! subIE header length */
#define PAYLOAD_SUB_IE_HEADER_LEN 2

/*! Short subIE type */
#define PAYLOAD_SUB_ID_IE_TYPE_SHORT 0
/*! Long subIE type */
#define PAYLOAD_SUB_ID_IE_TYPE_LONG 1

/*! Short subIE header length */
#define PAYLOAD_SUB_ID_IE_SHORT_HEADER_LEN  2

/*! Payload IE SubIE Type Size */
#define PAYLOAD_IE_SUB_IE_TYPE_SIZE 1
/*! Payload IE SubIE Type Position */
#define PAYLOAD_IE_SUB_IE_TYPE_POSITION 15
/*! Payload IE SubIE ID Short Size */
#define PAYLOAD_IE_SUB_IE_ID_SHORT_SIZE 7
/*! Payload IE SubIE ID Short Position */
#define PAYLOAD_IE_SUB_IE_ID_SHORT_POSITION 8
/*! Payload IE SubIE Short Length Size */
#define PAYLOAD_IE_SUB_IE_LEN_SHORT_SIZE 8
/*! Payload IE SubIE Short Length Position */
#define PAYLOAD_IE_SUB_IE_LEN_SHORT_POSITION 0
/*! Payload IE SubIE ID Long Size */
#define PAYLOAD_IE_SUB_IE_ID_LONG_SIZE 4
/*! Payload IE SubIE ID Long Position */
#define PAYLOAD_IE_SUB_IE_SUB_ID_LONG_POSITION 11
/*! Payload IE SubIE ID Long Length Size */
#define PAYLOAD_IE_SUB_IE_LEN_LONG_SIZE 11
/*! Payload IE SubIE Long Length Position */
#define PAYLOAD_IE_SUB_IE_LEN_LONG_POSITION 0

/*! Unpack a field from a uint16_t */
#define IE_UNPACKING(var,size,position) (((uint16_t)(var)>>(position))\
                &(((uint16_t)1<<(size))-1))

/*! Make a uint16_t from 2 uint8_t */
#define MAKE_UINT16(low,high) (((low)&0x00FF)|(((high)&0x00FF)<<8))

/*! Get the SubIE type field (bool) */
#define GET_SUBIE_TYPE(ctl) (bool)(IE_UNPACKING(ctl,\
                 PAYLOAD_IE_SUB_IE_TYPE_SIZE, PAYLOAD_IE_SUB_IE_TYPE_POSITION))

/*! Get the SubIE Long ID  */
#define GET_SUBIE_ID_LONG(ctl) (uint8_t)(IE_UNPACKING(ctl,\
       PAYLOAD_IE_SUB_IE_ID_LONG_SIZE, PAYLOAD_IE_SUB_IE_SUB_ID_LONG_POSITION))

/*! Get the SubIE Long Length */
#define GET_SUBIE_LEN_LONG(ctl) (uint16_t)(IE_UNPACKING(ctl,\
         PAYLOAD_IE_SUB_IE_LEN_LONG_SIZE, PAYLOAD_IE_SUB_IE_LEN_LONG_POSITION))

/*! Get the SubIE Short ID  */
#define GET_SUBIE_ID_SHORT(ctl) (uint8_t)(IE_UNPACKING(ctl,\
         PAYLOAD_IE_SUB_IE_ID_SHORT_SIZE, PAYLOAD_IE_SUB_IE_ID_SHORT_POSITION))

/*! Get the SubIE Short Length */
#define GET_SUBIE_LEN_SHORT(ctl) (uint16_t)(IE_UNPACKING(ctl,\
       PAYLOAD_IE_SUB_IE_LEN_SHORT_SIZE, PAYLOAD_IE_SUB_IE_LEN_SHORT_POSITION))

/******************************************************************************
 Structures
 *****************************************************************************/

/******************************************************************************
 Global variables
 *****************************************************************************/

/*!
 The ApiMac_extAddr is the MAC's IEEE address, setup with the Chip's
 IEEE addresses in main.c
 */
ApiMac_sAddrExt_t ApiMac_extAddr;

/******************************************************************************
 Local variables
 *****************************************************************************/

/*! MAC callback table, initialized to no callback table */
STATIC ApiMac_callbacks_t *pMacCallbacks = (ApiMac_callbacks_t *) NULL;




/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/
static ApiMac_status_t mlmeGetFhReq(uint16_t pibAttribute, void *pValue,
                                    uint16_t *pLen);
static ApiMac_status_t mlmeGetSecurityReq(uint16_t pibAttribute, void *pValue,
                                                 uint16_t *len);

static void ApiMac_mtAddrToApiMacsAddr(ApiMac_sAddr_t *apimacAddr, uint8_t *mtmacAddr, uint8_t addrMode);
//static uint16_t processIncomingICallMsg(macCbackEvent_t *pMsg);
//static void copyMacAddrToApiMacAddr(ApiMac_sAddr_t *pDst, sAddr_t *pSrc);
//static void copyMacPanDescToApiMacPanDesc(ApiMac_panDesc_t *pDst,
//                                          macPanDesc_t *pSrc);
//static void processBeaconNotifyInd(macMlmeBeaconNotifyInd_t *pInd);
//static void processScanCnf(macMlmeScanCnf_t *pCnf);
//static void macMsgDeallocate(macEventHdr_t *pData);
//static void copyDataInd(ApiMac_mcpsDataInd_t *pDst, macMcpsDataInd_t *pSrc);
//void copyApiMacAddrToMacAddr(sAddr_t *pDst, ApiMac_sAddr_t *pSrc);
static ApiMac_status_t parsePayloadIEs(uint8_t *pContent, uint16_t contentLen,
                                       ApiMac_payloadIeRec_t **pList,
                                       bool group);
uint16_t convertTxOptions(ApiMac_txOptions_t txOptions);
static void ApiMac_apiMacAddrToMtMac(uint8_t *addrmode, uint8_t *addr, ApiMac_sAddr_t *apimacAddr);

/******************************************************************************
 Local MT Callback Function Prototypes
 *****************************************************************************/
static void ApiMac_processMtMacDataCnfCB(MtMac_dataCnf_t *pDataCnf);
static void ApiMac_processMtMacDataIndCB(MtMac_dataInd_t *pDataInd);
static void ApiMac_processMtMacPurgeCnfCB(MtMac_purgeCnf_t *pPurgeCnf);
static void ApiMac_processMtMacWsAsyncIndCB(MtMac_wsAsyncInd_t *pWsAsyncInd);
static void ApiMac_processMtMacSyncLossIndCB(MtMac_syncLossInd_t *pSyncLossInd);
static void ApiMac_processMtMacAssociateIndCB(MtMac_associateInd_t *pAssociateInd);
static void ApiMac_processMtMacAssociateCnfCB(MtMac_associateCnf_t *pAssociateCnf);
static void ApiMac_processMtMacBeaconNotifyIndCB(MtMac_beaconNotifyInd_t *pBeaconNotifyInd);
static void ApiMac_processMtMacDisassociateIndCB(MtMac_disassociateInd_t *pDisassociateInd);
static void ApiMac_processMtMacDisassociateCnfCB(MtMac_disassociateCnf_t *pDisassociateCnf);
static void ApiMac_processMtMacOrphanIndCB(MtMac_orphanInd_t *pOrphanInd);
static void ApiMac_processMtMacPollCnfCB(MtMac_pollCnf_t *pPollCnf);
static void ApiMac_processMtMacPollIndCB(MtMac_pollInd_t *pPollInd);
static void ApiMac_processMtMacScanCnfCB(MtMac_scanCnf_t *pScanCnf);
static void ApiMac_processMtMacCommStatusIndCB(MtMac_commStatusInd_t *pCommStatusInd);
static void ApiMac_processMtMacStartCnfCB(MtMac_startCnf_t *pStartCnf);
static void ApiMac_processMtMacWsAsyncCnfCB(MtMac_wsAsyncCnf_t *pWsAsyncCnf);

MtMac_callbacks_t mtMacCallbacks =
{
    ApiMac_processMtMacDataCnfCB,
    ApiMac_processMtMacDataIndCB,
    ApiMac_processMtMacPurgeCnfCB,
    ApiMac_processMtMacWsAsyncIndCB,
    ApiMac_processMtMacSyncLossIndCB,
    ApiMac_processMtMacAssociateIndCB,
    ApiMac_processMtMacAssociateCnfCB,
    ApiMac_processMtMacBeaconNotifyIndCB,
    ApiMac_processMtMacDisassociateIndCB,
    ApiMac_processMtMacDisassociateCnfCB,
    ApiMac_processMtMacOrphanIndCB,
    ApiMac_processMtMacPollCnfCB,
    ApiMac_processMtMacPollIndCB,
    ApiMac_processMtMacScanCnfCB,
    ApiMac_processMtMacCommStatusIndCB,
    ApiMac_processMtMacStartCnfCB,
    ApiMac_processMtMacWsAsyncCnfCB
};


/******************************************************************************
 Public Functions
 *****************************************************************************/

/*!
 Initialize this module.

 Public function defined in api_mac.h
 */
void ApiMac_init(bool enableFH)
{
    MtUtil_utilGetExtAddr_t getExtData;
    MtUtil_utilGetExtAddrSrsp_t rspData;
    MtMac_RegisterCbs(&mtMacCallbacks);
    /* Enable frequency hopping? */
    if(enableFH)
    {
        ApiMac_enableFH();
    }

    /* Reset the MAC */
    ApiMac_mlmeResetReq(true);

    /* Get IEEE address only for Host processor when using CoP*/
    getExtData.Type = 0x01;
    MtUtil_utilGetExtAddr(&getExtData, &rspData);
    memcpy(ApiMac_extAddr, rspData.ExtAddress, APIMAC_SADDR_EXT_LEN);
    /* Set the device IEEE address */
    ApiMac_mlmeSetReqArray(ApiMac_attribute_extendedAddress, ApiMac_extAddr);
}

/*!
 Register for MAC callbacks.

 Public function defined in api_mac.h
 */
void ApiMac_registerCallbacks(ApiMac_callbacks_t *pCallbacks)
{
    /* Save the application's callback table */
    pMacCallbacks = pCallbacks;
}

/*!
 Register for MAC callbacks.

 Public function defined in api_mac.h
 */
void ApiMac_processIncoming(void)
{
//    ICall_EntityID src;
//    ICall_EntityID dest;
//    macCbackEvent_t *pMsg;
//
//    /* Wait for response message */
//    if(ICall_wait(ICALL_TIMEOUT_FOREVER) == ICALL_ERRNO_SUCCESS)
//    {
//        /* Retrieve the response message */
//        if(ICall_fetchMsg(&src, &dest, (void **)&pMsg) == ICALL_ERRNO_SUCCESS)
//        {
//            if(dest == ApiMac_appEntity)
//            {
//                if(src == macEntityID)
//                {
//                    /* Process the message from the MAC stack */
//                    processIncomingICallMsg(pMsg);
//                }
//                else if(pMacCallbacks->pUnprocessedCb)
//                {
//                    /* Initiate the unprocessed message callback */
//                    pMacCallbacks->pUnprocessedCb((uint16_t)src, 0,
//                                                  (void *)pMsg);
//                }
//            }
//
//            if(pMsg != NULL)
//            {
//                freeMsg(pMsg);
//            }
//        }
//    }
}

static void ApiMac_apiMacAddrToMtMac(uint8_t *addrmode, uint8_t *addr, ApiMac_sAddr_t *apimacAddr)
{
    *addrmode = (ApiMac_addrType_t)apimacAddr->addrMode;
    if(apimacAddr->addrMode == ApiMac_addrType_extended)
    {
        memcpy(addr, apimacAddr->addr.extAddr, APIMAC_SADDR_EXT_LEN);
    }
    else if(apimacAddr->addrMode == ApiMac_addrType_short)
    {
        Util_bufferUint16(addr, apimacAddr->addr.shortAddr);
    }
}



/*!
 This function sends application data to the MAC for
 transmission in a MAC data frame.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mcpsDataReq(ApiMac_mcpsDataReq_t *pData)
{
    MtMac_dataReq_t dataReq;

    ApiMac_apiMacAddrToMtMac(&dataReq.DestAddressMode,
                             dataReq.DestAddress,
                             &pData->dstAddr);
    dataReq.DestPanId = pData->dstPanId;
    dataReq.SrcAddressMode = (uint8_t)pData->srcAddrMode;
    dataReq.Handle = pData->msduHandle;
    dataReq.TxOption = convertTxOptions(pData->txOptions);
    dataReq.Channel = pData->channel;
    dataReq.Power = pData->power;
    memcpy(dataReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    dataReq.SecurityLevel = pData->sec.securityLevel;
    dataReq.KeyIdMode = pData->sec.keyIdMode;
    dataReq.KeyIndex = pData->sec.keyIndex;
    dataReq.IncludeFhIEs = pData->includeFhIEs;
    dataReq.DataLength = pData->msdu.len;
    dataReq.IELength = pData->payloadIELen;
    dataReq.DataPayload = pData->msdu.p;
    dataReq.IEPayload = pData->pIEList;

    return ((ApiMac_status_t)MtMac_dataReq(&dataReq));
}

/*!
 This function purges and discards a data request from the MAC
 data queue.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mcpsPurgeReq(uint8_t msduHandle)
{
    MtMac_purgeReq_t purgeReq;

    purgeReq.Handle = msduHandle;

    return ((ApiMac_status_t)MtMac_purgeReq(&purgeReq));
}

/*!
 This function sends an associate request to a coordinator
 device.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeAssociateReq(ApiMac_mlmeAssociateReq_t *pData)
{
    MtMac_associateReq_t assocReq;

    assocReq.LogicalChannel = pData->logicalChannel;
    assocReq.ChannelPage = pData->channelPage;
    assocReq.PhyId = pData->phyID;
    assocReq.CoordAddressMode = pData->coordAddress.addrMode;
    memcpy(assocReq.CoordAddress, pData->coordAddress.addr.extAddr, APIMAC_SADDR_EXT_LEN);
    assocReq.CoordPanId = pData->coordPanId;
    assocReq.CapabilityInformation = 0;
    if(pData->capabilityInformation.panCoord)
    {
        assocReq.CapabilityInformation |= CAPABLE_PAN_COORD;
    }
    if(pData->capabilityInformation.ffd)
    {
        assocReq.CapabilityInformation |= CAPABLE_FFD;
    }
    if(pData->capabilityInformation.mainsPower)
    {
        assocReq.CapabilityInformation |= CAPABLE_MAINS_POWER;
    }
    if(pData->capabilityInformation.rxOnWhenIdle)
    {
        assocReq.CapabilityInformation |= CAPABLE_RX_ON_IDLE;
    }
    if(pData->capabilityInformation.security)
    {
        assocReq.CapabilityInformation |= CAPABLE_SECURITY;
    }
    if(pData->capabilityInformation.allocAddr)
    {
        assocReq.CapabilityInformation |= CAPABLE_ALLOC_ADDR;
    }
    memcpy(assocReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    assocReq.SecurityLevel = pData->sec.securityLevel;
    assocReq.KeyIdMode = pData->sec.keyIdMode;
    assocReq.KeyIndex = pData->sec.keyIndex;

    return ((ApiMac_status_t)MtMac_associateReq(&assocReq));
}

/*!
 This function sends an associate response to a device
 requesting to associate.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeAssociateRsp(ApiMac_mlmeAssociateRsp_t *pData)
{
    MtMac_associateRsp_t mtAssocRsp;
    memcpy(mtAssocRsp.ExtendedAddress, pData->deviceAddress, APIMAC_SADDR_EXT_LEN);
    mtAssocRsp.AssocShortAddress = pData->assocShortAddress;
    mtAssocRsp.AssocStatus = pData->status;

    memcpy(mtAssocRsp.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    mtAssocRsp.SecurityLevel = pData->sec.securityLevel;
    mtAssocRsp.KeyIdMode = pData->sec.keyIdMode;
    mtAssocRsp.KeyIndex = pData->sec.keyIndex;

    return ((ApiMac_status_t)MtMac_associateRsp(&mtAssocRsp));
}

/*!
 This function is used by an associated device to notify the
 coordinator of its intent to leave the PAN.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeDisassociateReq(ApiMac_mlmeDisassociateReq_t *pData)
{
    MtMac_disassociateReq_t disassocReq;

    disassocReq.DeviceAddressMode = pData->deviceAddress.addrMode;
    memcpy(disassocReq.DeviceAddress, pData->deviceAddress.addr.extAddr, APIMAC_SADDR_EXT_LEN);
    disassocReq.DevicePanId = pData->devicePanId;
    disassocReq.DisassociateReason = (uint8_t)pData->disassociateReason;
    disassocReq.TxIndirect = pData->txIndirect;
    memcpy(disassocReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    disassocReq.SecurityLevel = pData->sec.securityLevel;
    disassocReq.KeyIdMode = pData->sec.keyIdMode;
    disassocReq.KeyIndex = pData->sec.keyIndex;

    return ((ApiMac_status_t)MtMac_disassociateReq(&disassocReq));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetReqBool(ApiMac_attribute_bool_t pibAttribute,
bool *pValue)
{
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = (uint8_t*)pValue;
    return ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetReqUint8(ApiMac_attribute_uint8_t pibAttribute,
                                       uint8_t *pValue)
{
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = (uint8_t*)pValue;
    return ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetReqUint16(ApiMac_attribute_uint16_t pibAttribute,
                                        uint16_t *pValue)
{
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = (uint8_t*)pValue;
    return ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetReqUint32(ApiMac_attribute_uint32_t pibAttribute,
                                        uint32_t *pValue)
{
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = (uint8_t*)pValue;
    return ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetReqArray(ApiMac_attribute_array_t pibAttribute,
                                       uint8_t *pValue)
{
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = pValue;
    return ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
}

/*!
 * @brief       This direct execute function retrieves an attribute value from
 *              the MAC PIB.
 *
 * @param       pibAttribute - The attribute identifier
 * @param       pValue - pointer to the attribute value
 * @param       pLen - pointer to the read length
 *
 * @return      The status of the request
 */
ApiMac_status_t ApiMac_mlmeGetReqArrayLen(ApiMac_attribute_array_t pibAttribute,
                                          uint8_t *pValue,
                                          uint16_t *pLen)
{
    ApiMac_status_t macStatus;
    MtMac_getReq_t gReq;
    MtMac_getReqSrsp_t gRsp;
    gReq.AttributeID = (uint8_t)pibAttribute;
    gRsp.Data = pValue;
    macStatus = ((ApiMac_status_t) MtMac_getReq(&gReq, &gRsp));
    *pLen = gRsp.AttrLen;
    return macStatus;
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC frequency Hopping PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetFhReqUint8(
                ApiMac_FHAttribute_uint8_t pibAttribute, uint8_t *pValue)
{
    return (mlmeGetFhReq((uint16_t)pibAttribute, (void *)pValue, NULL));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC frequency Hopping PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetFhReqUint16(
                ApiMac_FHAttribute_uint16_t pibAttribute, uint16_t *pValue)
{
    return (mlmeGetFhReq((uint16_t)pibAttribute, (void *)pValue, NULL));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC frequency Hopping PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetFhReqUint32(
                ApiMac_FHAttribute_uint32_t pibAttribute, uint32_t *pValue)
{
    return (mlmeGetFhReq((uint16_t)pibAttribute, (void *)pValue, NULL));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC frequency Hopping PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetFhReqArray(
                ApiMac_FHAttribute_array_t pibAttribute, uint8_t *pValue)
{
    return (mlmeGetFhReq((uint16_t)pibAttribute, (void *)pValue, NULL));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC frequency Hopping PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetFhReqArrayLen(
                ApiMac_FHAttribute_array_t pibAttribute,
                uint8_t *pValue,
                uint16_t *pLen)
{
    return (mlmeGetFhReq((uint16_t)pibAttribute, (void *)pValue, pLen));
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC Secutity PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetSecurityReqUint8(
                ApiMac_securityAttribute_uint8_t pibAttribute, uint8_t *pValue)
{
    return mlmeGetSecurityReq((uint16_t)pibAttribute, (void*)pValue, NULL);
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC Secutity PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetSecurityReqUint16(
                ApiMac_securityAttribute_uint16_t pibAttribute,
                uint16_t *pValue)
{
    return mlmeGetSecurityReq((uint16_t)pibAttribute, (void*)pValue, NULL);
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC Secutity PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetSecurityReqArray(
                ApiMac_securityAttribute_array_t pibAttribute, uint8_t *pValue)
{
    return mlmeGetSecurityReq((uint16_t)pibAttribute, (void*)pValue, NULL);
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC Secutity PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetSecurityReqArrayLen(
                ApiMac_securityAttribute_array_t pibAttribute,
                uint8_t *pValue,
                uint16_t *pLen
                )
{
    return mlmeGetSecurityReq((uint16_t)pibAttribute, (void*)pValue, pLen);
}

/*!
 This direct execute function retrieves an attribute value from
 the MAC Secutity PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeGetSecurityReqStruct(
                ApiMac_securityAttribute_struct_t pibAttribute, void *pValue)
{
    return mlmeGetSecurityReq((uint16_t)pibAttribute, (void*)pValue, NULL);
}

/*!
 This function is called in response to an orphan notification
 from a peer device.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeOrphanRsp(ApiMac_mlmeOrphanRsp_t *pData)
{
    MtMac_orphanRsp_t mtmacOrphanRsp;
    memcpy(mtmacOrphanRsp.ExtendedAddress, pData->orphanAddress, APIMAC_SADDR_EXT_LEN);
    mtmacOrphanRsp.AssocShortAddress = pData->shortAddress;
    mtmacOrphanRsp.AssociatedMember = pData->associatedMember;

    memcpy(mtmacOrphanRsp.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    mtmacOrphanRsp.SecurityLevel = pData->sec.securityLevel;
    mtmacOrphanRsp.KeyIdMode = pData->sec.keyIdMode;
    mtmacOrphanRsp.KeyIndex = pData->sec.keyIndex;
    return ((ApiMac_status_t)MtMac_orphanRsp(&mtmacOrphanRsp));
}

/*!
 This function is used to request pending data from the coordinator.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmePollReq(ApiMac_mlmePollReq_t *pData)
{
    MtMac_pollReq_t pollReq;

    pollReq.CoordAddressMode = pData->coordAddress.addrMode;
    memcpy(pollReq.CoordAddress, pData->coordAddress.addr.extAddr, APIMAC_SADDR_EXT_LEN);
    pollReq.CoordPanId = pData->coordPanId;
    memcpy(pollReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    pollReq.SecurityLevel = pData->sec.securityLevel;
    pollReq.KeyIdMode = pData->sec.keyIdMode;
    pollReq.KeyIndex = pData->sec.keyIndex;

    return ((ApiMac_status_t)MtMac_pollReq(&pollReq));
}

/*!
 This function must be called once at system startup before any other
 function in the management API is called.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeResetReq(bool setDefaultPib)
{
    MtMac_resetReq_t mtmacResetReq;
    mtmacResetReq.SetDefault = (uint8_t)setDefaultPib;
    return ((ApiMac_status_t)MtMac_resetReq(&mtmacResetReq));
}

/*!
 This function initiates an energy detect, active, passive, or
 orphan scan on one or more channels.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeScanReq(ApiMac_mlmeScanReq_t *pData)
{
#if defined (FEATURE_BEACON_MODE) || defined (FEATURE_NON_BEACON_MODE)
    MtMac_scanReq_t mtmacScanReq;
    mtmacScanReq.ScanType = (uint8_t)pData->scanType;
    mtmacScanReq.ScanDuration = pData->scanDuration;
    mtmacScanReq.ChannelPage = pData->channelPage;
    mtmacScanReq.PhyId = pData->phyID;
    mtmacScanReq.MaxResults = pData->maxResults;
    mtmacScanReq.PermitJoin = pData->permitJoining;
    mtmacScanReq.LinkQuality = pData->linkQuality;
    mtmacScanReq.RspFilter = pData->percentFilter;
    mtmacScanReq.MpmScan = (uint8_t)pData->MPMScan;
    mtmacScanReq.MpmType = pData->MPMScanType;
    mtmacScanReq.MpmDuration = pData->MPMScanDuration;
    memcpy(mtmacScanReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    mtmacScanReq.SecLevel = pData->sec.securityLevel;
    mtmacScanReq.KeyIdMode = pData->sec.keyIdMode;
    mtmacScanReq.KeyIndex = pData->sec.keyIndex;
    memcpy(mtmacScanReq.Channels, pData->scanChannels, APIMAC_154G_CHANNEL_BITMAP_SIZ);

    return ((ApiMac_status_t)MtMac_scanReq(&mtmacScanReq));
#else
    return (ApiMac_status_unsupported);
#endif /* FEATURE_BEACON_MODE || FEATURE_NON_BEACON_MODE */
}

/*!
 This direct execute function sets an attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetReqBool(ApiMac_attribute_bool_t pibAttribute,
bool value)
{
    MtMac_setReq_t setReq;
    setReq.AttributeID = pibAttribute;
    memset(setReq.AttributeValue, 0x00, 16);
    setReq.AttributeValue[0] = (uint8_t)value;
    return ((ApiMac_status_t)MtMac_setReq(&setReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetReqUint8(ApiMac_attribute_uint8_t pibAttribute,
                                       uint8_t value)
{
    MtMac_setReq_t setReq;
    setReq.AttributeID = pibAttribute;
    memset(setReq.AttributeValue, 0x00, 16);
    setReq.AttributeValue[0] = value;
    return ((ApiMac_status_t)MtMac_setReq(&setReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetReqUint16(ApiMac_attribute_uint16_t pibAttribute,
                                        uint16_t value)
{
    MtMac_setReq_t setReq;
    setReq.AttributeID = pibAttribute;
    memset(setReq.AttributeValue, 0x00, 16);
    Util_bufferUint16(setReq.AttributeValue, value);
    return ((ApiMac_status_t)MtMac_setReq(&setReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetReqUint32(ApiMac_attribute_uint32_t pibAttribute,
                                        uint32_t value)
{
    MtMac_setReq_t setReq;
    setReq.AttributeID = pibAttribute;
    memset(setReq.AttributeValue, 0x00, 16);
    Util_bufferUint32(setReq.AttributeValue, value);
    return ((ApiMac_status_t)MtMac_setReq(&setReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetReqArray(ApiMac_attribute_array_t pibAttribute,
                                       uint8_t *pValue)
{
    MtMac_setReq_t setReq;
    setReq.AttributeID = pibAttribute;
    memset(setReq.AttributeValue, 0x00, 16);
    memcpy(setReq.AttributeValue, pValue, 16);
    return ((ApiMac_status_t)MtMac_setReq(&setReq));
}

/*!
 This direct execute function sets a frequency hopping attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetFhReqUint8(
                ApiMac_FHAttribute_uint8_t pibAttribute, uint8_t value)
{
    MtMac_fhSetReq_t fhSet;
    fhSet.AttributeID = (uint16_t)pibAttribute;
    fhSet.AttrLen = sizeof(uint8_t);
    fhSet.Data = (uint8_t*)&value;
    return ((ApiMac_status_t)MtMac_fhSetReq(&fhSet));
}

/*!
 This direct execute function sets a frequency hopping attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetFhReqUint16(
                ApiMac_FHAttribute_uint16_t pibAttribute, uint16_t value)
{
    MtMac_fhSetReq_t fhSet;
    fhSet.AttributeID = (uint16_t)pibAttribute;
    fhSet.AttrLen = sizeof(uint16_t);
    fhSet.Data = (uint8_t*)&value;
    return ((ApiMac_status_t)MtMac_fhSetReq(&fhSet));
}

/*!
 This direct execute function sets a frequency hopping attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetFhReqUint32(
                ApiMac_FHAttribute_uint32_t pibAttribute, uint32_t value)
{
    MtMac_fhSetReq_t fhSet;
    fhSet.AttributeID = (uint16_t)pibAttribute;
    fhSet.AttrLen = sizeof(uint32_t);
    fhSet.Data = (uint8_t*)&value;
    return ((ApiMac_status_t)MtMac_fhSetReq(&fhSet));
}

/*!
 This direct execute function sets a frequency hopping attribute value
 in the MAC PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetFhReqArray(
                ApiMac_FHAttribute_array_t pibAttribute, uint8_t *pValue)
{
    MtMac_fhSetReq_t fhSet;
    MtMac_fhGetReq_t fhGetLen;
    MtMac_fhGetReqSrsp_t fhGetLenRsp;

    fhGetLen.AttributeID = (uint16_t)pibAttribute;
    fhGetLenRsp.Data = NULL;
    fhGetLenRsp.AttrLen = 0;
    MtMac_fhGetReq(&fhGetLen, &fhGetLenRsp);

    fhSet.AttributeID = (uint16_t)pibAttribute;
    fhSet.AttrLen = fhGetLenRsp.AttrLen;
    fhSet.Data = pValue;
    return ((ApiMac_status_t)MtMac_fhSetReq(&fhSet));
}

/*!
 This direct execute function sets an attribute value
 in the MAC Security PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetSecurityReqUint8(
                ApiMac_securityAttribute_uint8_t pibAttribute, uint8_t value)
{
    MtMac_securitySetReq_t secReq;
    secReq.AttrLen = 1;
    secReq.AttrValue = &value;
    secReq.AttributeID = pibAttribute;
    secReq.Index1 = 0;
    secReq.Index2 = 0;
    return ((ApiMac_status_t)MtMac_securitySetReq(&secReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC Security PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetSecurityReqUint16(
                ApiMac_securityAttribute_uint16_t pibAttribute, uint16_t value)
{
    MtMac_securitySetReq_t secReq;
    secReq.AttrLen = 2;
    secReq.AttrValue = (uint8_t*)&value;
    secReq.AttributeID = pibAttribute;
    secReq.Index1 = 0;
    secReq.Index2 = 0;
    return ((ApiMac_status_t)MtMac_securitySetReq(&secReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC Security PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetSecurityReqArray(
                ApiMac_securityAttribute_array_t pibAttribute, uint8_t *pValue)
{
    MtMac_securitySetReq_t secReq;
    uint16_t len;
    ApiMac_mlmeGetSecurityReqArrayLen(pibAttribute, NULL, &len);

    secReq.AttrLen = len;
    secReq.AttrValue = pValue;
    secReq.AttributeID = pibAttribute;
    secReq.Index1 = 0;
    secReq.Index2 = 0;
    return ((ApiMac_status_t)MtMac_securitySetReq(&secReq));
}

/*!
 This direct execute function sets an attribute value
 in the MAC Security PIB.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSetSecurityReqStruct(
                ApiMac_securityAttribute_struct_t pibAttribute, void *pValue)
{
    MtMac_securitySetReq_t secReq;
    uint16_t len;
    ApiMac_mlmeGetSecurityReqArrayLen((ApiMac_securityAttribute_array_t)pibAttribute, NULL, &len);
    secReq.AttrLen = len;
    secReq.AttrValue = pValue;
    secReq.AttributeID = pibAttribute;
    if(pibAttribute == ApiMac_securityAttribute_securityLevelEntry)
    {
        secReq.Index1 = ((ApiMac_securityPibSecurityLevelEntry_t*)(pValue))->levelIndex;
        secReq.AttrValue = (uint8_t*)&((ApiMac_securityPibSecurityLevelEntry_t*)(pValue))->levelEntry;
    }
    else
    {
        secReq.Index1 = 0;
    }
    secReq.Index2 = 0;
    return ((ApiMac_status_t)MtMac_securitySetReq(&secReq));
}

/*!
 This function is called by a coordinator or PAN coordinator
 to start or reconfigure a network.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeStartReq(ApiMac_mlmeStartReq_t *pData)
{
    MtMac_startReq_t strtReq;
    strtReq.StartTime = pData->startTime;
    strtReq.PanId = pData->panId;
    strtReq.LogicalChannel = pData->logicalChannel;
    strtReq.ChannelPage = pData->channelPage;
    strtReq.PhyId = pData->phyID;
    strtReq.BeaconOrder = pData->beaconOrder;
    strtReq.SuperFrameOrder = pData->superframeOrder;
    strtReq.PanCoordinator = pData->panCoordinator;
    strtReq.BatteryLifeExt = pData->batteryLifeExt;
    strtReq.CoordRealignment = pData->coordRealignment;
    memcpy(strtReq.RealignKeySource, pData->realignSec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    strtReq.RealignSecurityLevel = pData->realignSec.securityLevel;
    strtReq.RealignKeyIdMode = pData->realignSec.keyIdMode;
    strtReq.RealignKeyIndex = pData->realignSec.keyIndex;
    memcpy(strtReq.BeaconKeySource, pData->beaconSec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    strtReq.BeaconSecurityLevel = pData->beaconSec.securityLevel;
    strtReq.BeaconKeyIdMode = pData->beaconSec.keyIdMode;
    strtReq.BeaconKeyIndex = pData->beaconSec.keyIndex;
    strtReq.StartFH = pData->startFH;
    strtReq.EnhBeaconOrder = pData->mpmParams.eBeaconOrder;
    strtReq.OfsTimeSlot = pData->mpmParams.offsetTimeSlot;
    strtReq.NonBeaconOrder = pData->mpmParams.NBPANEBeaconOrder;
    strtReq.NumIEs = pData->mpmParams.numIEs;
    strtReq.IEIDList = pData->mpmParams.pIEIDs;

    return ((ApiMac_status_t)MtMac_startReq(&strtReq));
}

/*!
 This function requests the MAC to synchronize with the
 coordinator by acquiring and optionally tracking its beacons.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeSyncReq(ApiMac_mlmeSyncReq_t *pData)
{

    MtMac_syncReq_t syncReq;

    syncReq.ChannelPage = pData->channelPage;
    syncReq.LogicalChannel = pData->logicalChannel;
    syncReq.PhyId = pData->phyID;
    syncReq.TrackBeacon = pData->trackBeacon;

    return ((ApiMac_status_t)MtMac_syncReq(&syncReq));
}

/*!
 This function returns a random byte from the MAC random number
 generator.

 Public function defined in api_mac.h
 */
uint8_t ApiMac_randomByte(void)
{
    return (MtUtil_utilRandom());
}

/*!
 Update Device Table entry and PIB with new Pan Id.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_updatePanId(uint16_t panId)
{
    ApiMac_status_t ret = ApiMac_status_noResources;
#ifdef FEATURE_MAC_SECURITY
    MtMac_updatePanidReq_t updtPanId;
    updtPanId.PanID = panId;
    ret = ((ApiMac_status_t)
            MtMac_updatePanidReq(&updtPanId));
    ret = ApiMac_status_success;
#endif  /* FEATURE_MAC_SECURITY */
    return ret;
}

/*!
 This functions handles the WiSUN async request.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_mlmeWSAsyncReq(ApiMac_mlmeWSAsyncReq_t* pData)
{
    MtMac_wsAsyncReq_t wsAsyncReq;

    memcpy(wsAsyncReq.Channels, pData->channels, sizeof(wsAsyncReq.Channels));
    wsAsyncReq.FrameType = pData->frameType;
    wsAsyncReq.KeyIdMode = pData->sec.keyIdMode;
    wsAsyncReq.KeyIndex = pData->sec.keyIndex;
    memcpy(wsAsyncReq.KeySource, pData->sec.keySource, APIMAC_KEY_SOURCE_MAX_LEN);
    wsAsyncReq.Operation = pData->operation;
    wsAsyncReq.SecurityLevel = pData->sec.securityLevel;

    return ((ApiMac_status_t)MtMac_wsAsyncReq(&wsAsyncReq));
}

/*!
 This function start the Frequency hopping operation.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_startFH(void)
{
    return ((ApiMac_status_t)MtMac_fhStartReq());
}

/*!
 Enables the Frequency hopping operation.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_enableFH(void)
{
    return ((ApiMac_status_t)MtMac_fhEnableReq());
}

/*!
 Parses the payload information elements.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_parsePayloadGroupIEs(uint8_t *pPayload,
                                            uint16_t payloadLen,
                                            ApiMac_payloadIeRec_t **pList)
{
    return (parsePayloadIEs(pPayload, payloadLen, pList, true));
}

/*!
 Parses the payload Sub Information Elements.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_parsePayloadSubIEs(uint8_t *pContent,
                                          uint16_t contentLen,
                                          ApiMac_payloadIeRec_t **pList)
{
    return (parsePayloadIEs(pContent, contentLen, pList, false));
}

/*!
 Free memory allocated by ApiMac.

 Public function defined in api_mac.h
 */
void ApiMac_freeIEList(ApiMac_payloadIeRec_t *pList)
{
    /* Loop through the list */
    while(pList)
    {
        ApiMac_payloadIeRec_t *pTmp = pList;

        /* Move to the next item in the list */
        pList = pTmp->pNext;

        /* free the current item */
        free(pTmp);
    }
}

/*!
 Convert ApiMac_capabilityInfo_t data type to uint8_t capInfo

 Public function defined in api_mac.h
 */
uint8_t ApiMac_convertCapabilityInfo(ApiMac_capabilityInfo_t *pMsgcapInfo)
{
    uint8_t capInfo = 0;

    if(pMsgcapInfo->panCoord)
    {
        capInfo |= CAPABLE_PAN_COORD;
    }

    if(pMsgcapInfo->ffd)
    {
        capInfo |= CAPABLE_FFD;
    }

    if(pMsgcapInfo->mainsPower)
    {
        capInfo |= CAPABLE_MAINS_POWER;
    }

    if(pMsgcapInfo->rxOnWhenIdle)
    {
        capInfo |= CAPABLE_RX_ON_IDLE;
    }

    if(pMsgcapInfo->security)
    {
        capInfo |= CAPABLE_SECURITY;
    }

    if(pMsgcapInfo->allocAddr)
    {
        capInfo |= CAPABLE_ALLOC_ADDR;
    }

    return (capInfo);
}

/*!
 Convert from bitmask byte to API MAC capInfo

 Public function defined in api_mac.h
 */
void ApiMac_buildMsgCapInfo(uint8_t cInfo, ApiMac_capabilityInfo_t *pPBcapInfo)
{
    if(cInfo & CAPABLE_PAN_COORD)
    {
        pPBcapInfo->panCoord = 1;
    }

    if(cInfo & CAPABLE_FFD)
    {
        pPBcapInfo->ffd = 1;
    }

    if(cInfo & CAPABLE_MAINS_POWER)
    {
        pPBcapInfo->mainsPower = 1;
    }

    if(cInfo & CAPABLE_RX_ON_IDLE)
    {
        pPBcapInfo->rxOnWhenIdle = 1;
    }

    if(cInfo & CAPABLE_SECURITY)
    {
        pPBcapInfo->security = 1;
    }

    if(cInfo & CAPABLE_ALLOC_ADDR)
    {
        pPBcapInfo->allocAddr = 1;
    }
}

/*!
 Adds a new MAC device table entry.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secAddDevice(ApiMac_secAddDevice_t *pAddDevice)
{

    MtMac_addDeviceReq_t addDeviceReq;

    addDeviceReq.PanID = pAddDevice->panID;
    addDeviceReq.ShortAddr = pAddDevice->shortAddr;
    memcpy(addDeviceReq.ExtAddr, pAddDevice->extAddr, APIMAC_SADDR_EXT_LEN);
    addDeviceReq.FrameCounter = pAddDevice->frameCounter;
    addDeviceReq.Exempt = pAddDevice->exempt;
    addDeviceReq.Unique = pAddDevice->uniqueDevice;
    addDeviceReq.Duplicate = pAddDevice->duplicateDevFlag;
    addDeviceReq.DataSize = pAddDevice->keyIdLookupDataSize;
    memcpy(addDeviceReq.LookupData, pAddDevice->keyIdLookupData, sizeof(pAddDevice->keyIdLookupData));

    return ((ApiMac_status_t)MtMac_addDeviceReq(&addDeviceReq));
}

/*!
 Removes MAC device table entries.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secDeleteDevice(ApiMac_sAddrExt_t *pExtAddr)
{

    MtMac_deleteDeviceReq_t deleteDeviceReq;

    memcpy(deleteDeviceReq.ExtAddr, pExtAddr, APIMAC_SADDR_EXT_LEN);

    return ((ApiMac_status_t)MtMac_deleteDeviceReq(&deleteDeviceReq));
}

/*!
 Removes the key at the specified key Index and removes all MAC device table
 enteries associated with this key.

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secDeleteKeyAndAssocDevices(uint8_t keyIndex)
{
    MtMac_deleteKeyReq_t deleteKeyReq;

    deleteKeyReq.Index = keyIndex;

    return ((ApiMac_status_t)MtMac_deleteKeyReq(&deleteKeyReq));
}

/*!
 Removes all MAC device table entries

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secDeleteAllDevices(void)
{
    return ((ApiMac_status_t)MtMac_deleteAllDevicesReq());
}

/*!
 Reads the frame counter value associated with a MAC security key indexed
 by the designated key identifier and the default key source

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secGetDefaultSourceKey(uint8_t keyId,
                                              uint32_t *pFrameCounter)
{
    ApiMac_status_t macStatus;
    MtMac_readKeyReq_t keyReq;
    MtMac_readKeyReqSrsp_t keyRsp;
    keyReq.Index = keyId;
    macStatus = (ApiMac_status_t)MtMac_readKeyReq(&keyReq, &keyRsp);
    *pFrameCounter = keyRsp.FrameCounter;
    return ((ApiMac_status_t)macStatus);
}

/*!
 Adds the MAC security key, adds the associated lookup list for the key,
 initializes the frame counter to the value provided

 Public function defined in api_mac.h
 */
ApiMac_status_t ApiMac_secAddKeyInitFrameCounter(
                ApiMac_secAddKeyInitFrameCounter_t *pInfo)
{
    MtMac_writeKeyReq_t keyReq;
    keyReq.DataSize = pInfo->lookupDataSize;
    keyReq.FrameCounter = pInfo->frameCounter;
    keyReq.Index = pInfo->replaceKeyIndex;
    memcpy(keyReq.Key, pInfo->key, 16);
    memcpy(keyReq.LookupData, pInfo->lookupData, 9);
    keyReq.New = (uint8_t)pInfo->newKeyFlag;

    return ((ApiMac_status_t)MtMac_writeKeyReq(&keyReq));
}

/******************************************************************************
 Local Functions
 *****************************************************************************/

/*!
 * @brief       Generic function to get an FH attribute
 *
 * @param       pibAttribute - attribute to get
 * @param       pValue - pointer to put the attribute value
 * @param       pLen - pointer to place to put length
 *
 * @return      status result
 */
static ApiMac_status_t mlmeGetFhReq(uint16_t pibAttribute, void *pValue,
                                    uint16_t *len)
{
    ApiMac_status_t macStatus;
    MtMac_fhGetReq_t getReq;
    MtMac_fhGetReqSrsp_t getRsp;
    getReq.AttributeID = pibAttribute;
    getRsp.Data = pValue;

    macStatus = (ApiMac_status_t)(MtMac_fhGetReq(&getReq, &getRsp));
    if(len)
    {
        *len = (uint16_t)getRsp.AttrLen;
    }
    return macStatus;
}

/*!
 * @brief       Generic function to get an Security attribute
 *
 * @param       pibAttribute - attribute to get
 * @param       pValue - pointer to put the attribute value
 * @param       pLen - pointer to place to put length
 *
 * @return      status result
 */
static ApiMac_status_t mlmeGetSecurityReq(uint16_t pibAttribute, void *pValue,
                                    uint16_t *len)
{
    ApiMac_status_t macStatus;
    MtMac_securityGetReq_t getReq;
    MtMac_securityGetReqSrsp_t getRsp;
    getReq.AttributeID = pibAttribute;
    getReq.Index1 = 0;
    getReq.Index2 = 0;
    getRsp.Data = pValue;

    macStatus = (ApiMac_status_t)(MtMac_securityGetReq(&getReq, &getRsp));
    switch(pibAttribute)
    {
    case ApiMac_securityAttribute_defaultKeySource:
        *len = 8;
        break;
    case ApiMac_securityAttribute_keyTable:
        *len = 0;
        break;
    case ApiMac_securityAttribute_securityLevelEntry:
        *len = 4;
        break;
    }
    return macStatus;
}

/*!
  Convert from bitmask byte to API MAC capInfo

  Public function defined in api_mac.h
*/
void ApiMac_mtCapInfToApiMacCapInf(ApiMac_capabilityInfo_t *pPBcapInfo, uint8_t cInfo)
{
    pPBcapInfo->panCoord = (bool)(cInfo & CAPABLE_PAN_COORD);
    pPBcapInfo->ffd = (bool)(cInfo & CAPABLE_FFD);
    pPBcapInfo->mainsPower = (bool)(cInfo & CAPABLE_MAINS_POWER);
    pPBcapInfo->rxOnWhenIdle = (bool)(cInfo & CAPABLE_RX_ON_IDLE);
    pPBcapInfo->security = (bool)(cInfo & CAPABLE_SECURITY);
    pPBcapInfo->allocAddr = (bool)(cInfo & CAPABLE_ALLOC_ADDR);
}

static void ApiMac_mtAddrToApiMacsAddr(ApiMac_sAddr_t *apimacAddr, uint8_t *mtmacAddr, uint8_t addrMode)
{
    apimacAddr->addrMode = (ApiMac_addrType_t)addrMode;
    if(apimacAddr->addrMode == ApiMac_addrType_extended)
    {
        memcpy(apimacAddr->addr.extAddr, mtmacAddr, APIMAC_SADDR_EXT_LEN);
    }
    else if(apimacAddr->addrMode == ApiMac_addrType_short)
    {
        apimacAddr->addr.shortAddr = Util_parseUint16(mtmacAddr);
    }
}

/*!
 * @brief common code to parse a pan descriptor from an MT message
 *
 * @param pMsg - message to parse
 * @param pD - where to put the message
 */
static void ApiMac_mtPanDescParse(ApiMac_panDesc_t *pD, MtMac_panDesc_t *pMsg)
{
    ApiMac_mtAddrToApiMacsAddr(&pD->coordAddress,
                                   pMsg->CoordAddress,
                                   pMsg->CoordAddrMode);
        pD->coordPanId = pMsg->PanId;
        pD->superframeSpec = pMsg->SuperframeSpec;
        pD->logicalChannel = pMsg->LogicalChannel;
        pD->channelPage = pMsg->ChannelPage;
        pD->gtsPermit = (bool)pMsg->GTSPermit;
        pD->linkQuality = pMsg->LinkQuality;
        pD->timestamp = pMsg->Timestamp;
        pD->securityFailure = (bool)pMsg->SecurityFailure;
        memcpy(pD->sec.keySource, pMsg->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        pD->sec.securityLevel = pMsg->SecurityLevel;
        pD->sec.keyIdMode = pMsg->KeyIdMode;
        pD->sec.keyIndex = pMsg->KeyIndex;
}

static void ApiMac_processMtMacDataCnfCB(MtMac_dataCnf_t *pDataCnf)
{
    if(pMacCallbacks && pMacCallbacks->pDataCnfCb)
    {
        ApiMac_mcpsDataCnf_t apiMacDataCnf;
        apiMacDataCnf.status = (ApiMac_status_t)pDataCnf->Status;
        apiMacDataCnf.msduHandle = pDataCnf->Handle;
        apiMacDataCnf.timestamp = pDataCnf->Timestamp;
        apiMacDataCnf.timestamp2 = pDataCnf->Timestamp2;
        apiMacDataCnf.retries = pDataCnf->Retries;
        apiMacDataCnf.mpduLinkQuality = pDataCnf->LinkQuality;
        apiMacDataCnf.correlation = pDataCnf->Correlation;
        apiMacDataCnf.rssi = pDataCnf->RSSI;
        apiMacDataCnf.frameCntr = pDataCnf->FrameCounter;
        pMacCallbacks->pDataCnfCb(&apiMacDataCnf);
    }
}
static void ApiMac_processMtMacDataIndCB(MtMac_dataInd_t *pDataInd)
{
    if(pMacCallbacks && pMacCallbacks->pDataIndCb)
    {
        ApiMac_mcpsDataInd_t apimacDataInd;
        ApiMac_mtAddrToApiMacsAddr(&apimacDataInd.srcAddr, pDataInd->SrcAddr, pDataInd->SrcAddrMode);
        ApiMac_mtAddrToApiMacsAddr(&apimacDataInd.dstAddr, pDataInd->DstAddr, pDataInd->DstAddrMode);
        apimacDataInd.timestamp = pDataInd->Timestamp;
        apimacDataInd.timestamp2 = pDataInd->Timestamp2;
        apimacDataInd.srcPanId = pDataInd->SrcPanId;
        apimacDataInd.dstPanId = pDataInd->DstPanId;
        apimacDataInd.mpduLinkQuality = pDataInd->LinkQuality;
        apimacDataInd.correlation = pDataInd->Correlation;
        apimacDataInd.rssi = pDataInd->RSSI;
        apimacDataInd.dsn = pDataInd->DSN;
        apimacDataInd.payloadIeLen = pDataInd->IELength;
        apimacDataInd.pPayloadIE = pDataInd->IEPayload;
        apimacDataInd.frameCntr = pDataInd->FrameCounter;
        memcpy(apimacDataInd.sec.keySource, pDataInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacDataInd.sec.securityLevel = pDataInd->SecurityLevel;
        apimacDataInd.sec.keyIdMode = pDataInd->KeyIdMode;
        apimacDataInd.sec.keyIndex = pDataInd->KeyIndex;
        apimacDataInd.msdu.len = pDataInd->DataLength;
        apimacDataInd.msdu.p = pDataInd->DataPayload;
        pMacCallbacks->pDataIndCb(&apimacDataInd);
    }
}
static void ApiMac_processMtMacPurgeCnfCB(MtMac_purgeCnf_t *pPurgeCnf)
{
    if(pMacCallbacks && pMacCallbacks->pPurgeCnfCb)
    {
        ApiMac_mcpsPurgeCnf_t apimacPurgeCnf;
        apimacPurgeCnf.msduHandle = pPurgeCnf->Handle;
        apimacPurgeCnf.status = (ApiMac_status_t)pPurgeCnf->Status;
        pMacCallbacks->pPurgeCnfCb(&apimacPurgeCnf);
    }
}
static void ApiMac_processMtMacWsAsyncIndCB(MtMac_wsAsyncInd_t *pWsAsyncInd)
{
    if(pMacCallbacks && pMacCallbacks->pWsAsyncIndCb)
    {
        ApiMac_mlmeWsAsyncInd_t apimacWsAsyncInd;
        ApiMac_mtAddrToApiMacsAddr(&apimacWsAsyncInd.srcAddr, pWsAsyncInd->SrcAddr, pWsAsyncInd->SrcAddrMode);
        ApiMac_mtAddrToApiMacsAddr(&apimacWsAsyncInd.dstAddr, pWsAsyncInd->DstAddr, pWsAsyncInd->DstAddrMode);
        apimacWsAsyncInd.timestamp = pWsAsyncInd->Timestamp;
        apimacWsAsyncInd.timestamp2 = pWsAsyncInd->Timestamp2;
        apimacWsAsyncInd.srcPanId = pWsAsyncInd->SrcPanId;
        apimacWsAsyncInd.dstPanId = pWsAsyncInd->DstPanId;
        apimacWsAsyncInd.mpduLinkQuality = pWsAsyncInd->LinkQuality;
        apimacWsAsyncInd.correlation = pWsAsyncInd->Correlation;
        apimacWsAsyncInd.rssi = pWsAsyncInd->RSSI;
        apimacWsAsyncInd.dsn = pWsAsyncInd->DSN;
        apimacWsAsyncInd.payloadIeLen = pWsAsyncInd->IELength;
        apimacWsAsyncInd.pPayloadIE = pWsAsyncInd->IEPayload;
        apimacWsAsyncInd.fhFrameType = (ApiMac_fhFrameType_t)pWsAsyncInd->FrameType;
        apimacWsAsyncInd.frameCntr = pWsAsyncInd->FrameCounter;
        memcpy(apimacWsAsyncInd.sec.keySource, pWsAsyncInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacWsAsyncInd.sec.securityLevel = pWsAsyncInd->SecurityLevel;
        apimacWsAsyncInd.sec.keyIdMode = pWsAsyncInd->KeyIdMode;
        apimacWsAsyncInd.sec.keyIndex = pWsAsyncInd->KeyIndex;
        apimacWsAsyncInd.msdu.len = pWsAsyncInd->DataLength;
        apimacWsAsyncInd.msdu.p = pWsAsyncInd->DataPayload;
        pMacCallbacks->pWsAsyncIndCb(&apimacWsAsyncInd);
    }
}
static void ApiMac_processMtMacSyncLossIndCB(MtMac_syncLossInd_t *pSyncLossInd)
{
    if(pMacCallbacks && pMacCallbacks->pSyncLossIndCb)
    {
        ApiMac_mlmeSyncLossInd_t apimacSyncLossInd;

        apimacSyncLossInd.channelPage = pSyncLossInd->ChannelPage;
        apimacSyncLossInd.logicalChannel = pSyncLossInd->LogicalChannel;
        apimacSyncLossInd.panId = pSyncLossInd->PanId;
        apimacSyncLossInd.phyID = pSyncLossInd->PhyId;
        apimacSyncLossInd.reason = (ApiMac_status_t)pSyncLossInd->Status;
        apimacSyncLossInd.sec.keyIdMode = pSyncLossInd->KeyIdMode;
        apimacSyncLossInd.sec.keyIndex = pSyncLossInd->KeyIndex;
        memcpy(apimacSyncLossInd.sec.keySource, pSyncLossInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacSyncLossInd.sec.securityLevel = pSyncLossInd->SecurityLevel;

        pMacCallbacks->pSyncLossIndCb(&apimacSyncLossInd);
    }
}
static void ApiMac_processMtMacAssociateIndCB(MtMac_associateInd_t *pAssociateInd)
{
    if(pMacCallbacks && pMacCallbacks->pAssocIndCb)
    {
        ApiMac_mlmeAssociateInd_t apimacAssocInd;
        memcpy(apimacAssocInd.deviceAddress, pAssociateInd->ExtendedAddress, APIMAC_SADDR_EXT_LEN);
        ApiMac_mtCapInfToApiMacCapInf(&apimacAssocInd.capabilityInformation, pAssociateInd->Capabilities);
        memcpy(apimacAssocInd.sec.keySource, pAssociateInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacAssocInd.sec.securityLevel = pAssociateInd->SecurityLevel;
        apimacAssocInd.sec.keyIdMode = pAssociateInd->KeyIdMode;
        apimacAssocInd.sec.keyIndex = pAssociateInd->KeyIndex;

        pMacCallbacks->pAssocIndCb(&apimacAssocInd);
    }
}
static void ApiMac_processMtMacAssociateCnfCB(MtMac_associateCnf_t *pAssociateCnf)
{
    if(pMacCallbacks && pMacCallbacks->pAssocCnfCb)
    {
        ApiMac_mlmeAssociateCnf_t apimacAssocCnf;

        apimacAssocCnf.assocShortAddress = pAssociateCnf->ShortAddress;
        apimacAssocCnf.sec.keyIdMode = pAssociateCnf->KeyIdMode;
        apimacAssocCnf.sec.keyIndex = pAssociateCnf->KeyIndex;
        memcpy(apimacAssocCnf.sec.keySource, pAssociateCnf->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacAssocCnf.sec.securityLevel = pAssociateCnf->SecurityLevel;
        apimacAssocCnf.status = (ApiMac_assocStatus_t)pAssociateCnf->Status;

        pMacCallbacks->pAssocCnfCb(&apimacAssocCnf);
    }
}
static void ApiMac_processMtMacBeaconNotifyIndCB(MtMac_beaconNotifyInd_t *pBeaconNotifyInd)
{
    if(pMacCallbacks && pMacCallbacks->pBeaconNotifyIndCb)
    {
        ApiMac_mlmeBeaconNotifyInd_t apimacBeacNotiInd;
        apimacBeacNotiInd.beaconType = (ApiMac_beaconType_t)pBeaconNotifyInd->BeaconType;
        apimacBeacNotiInd.bsn = pBeaconNotifyInd->BSN;
        ApiMac_mtAddrToApiMacsAddr(&apimacBeacNotiInd.panDesc.coordAddress,
                                   pBeaconNotifyInd->CoordExtendedAddress,
                                   pBeaconNotifyInd->CoordAddressMode);
        apimacBeacNotiInd.panDesc.coordPanId = pBeaconNotifyInd->PanId;
        apimacBeacNotiInd.panDesc.superframeSpec = pBeaconNotifyInd->SuperframeSpec;
        apimacBeacNotiInd.panDesc.logicalChannel = pBeaconNotifyInd->LogicalChannel;
        apimacBeacNotiInd.panDesc.channelPage = pBeaconNotifyInd->ChannelPage;
        apimacBeacNotiInd.panDesc.gtsPermit = (bool)pBeaconNotifyInd->GTSPermit;
        apimacBeacNotiInd.panDesc.linkQuality = pBeaconNotifyInd->LinkQuality;
        apimacBeacNotiInd.panDesc.timestamp = pBeaconNotifyInd->Timestamp;
        apimacBeacNotiInd.panDesc.securityFailure = (bool)pBeaconNotifyInd->SecurityFailure;
        memcpy(apimacBeacNotiInd.panDesc.sec.keySource, pBeaconNotifyInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacBeacNotiInd.panDesc.sec.securityLevel = pBeaconNotifyInd->SecurityLevel;
        apimacBeacNotiInd.panDesc.sec.keyIdMode = pBeaconNotifyInd->KeyIdMode;
        apimacBeacNotiInd.panDesc.sec.keyIndex = pBeaconNotifyInd->KeyIndex;

        pMacCallbacks->pBeaconNotifyIndCb(&apimacBeacNotiInd);
    }
}
static void ApiMac_processMtMacDisassociateIndCB(MtMac_disassociateInd_t *pDisassociateInd)
{
    if(pMacCallbacks && pMacCallbacks->pDisassociateIndCb)
    {
        ApiMac_mlmeDisassociateInd_t apimacDissInd;
        memcpy(apimacDissInd.deviceAddress, pDisassociateInd->ExtendedAddress, APIMAC_SADDR_EXT_LEN);
        apimacDissInd.disassociateReason = (ApiMac_disassocateReason_t)pDisassociateInd->DisassociateReason;
        memcpy(apimacDissInd.sec.keySource, pDisassociateInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacDissInd.sec.securityLevel = pDisassociateInd->SecurityLevel;
        apimacDissInd.sec.keyIdMode = pDisassociateInd->KeyIdMode;
        apimacDissInd.sec.keyIndex = pDisassociateInd->KeyIndex;

        pMacCallbacks->pDisassociateIndCb(&apimacDissInd);
    }
}
static void ApiMac_processMtMacDisassociateCnfCB(MtMac_disassociateCnf_t *pDisassociateCnf)
{
    if(pMacCallbacks && pMacCallbacks->pDisassociateCnfCb)
    {
        ApiMac_mlmeDisassociateCnf_t apimacDissCnf;

        memcpy(apimacDissCnf.deviceAddress.addr.extAddr, pDisassociateCnf->DeviceAddr, APIMAC_SADDR_EXT_LEN);
        apimacDissCnf.deviceAddress.addrMode = (ApiMac_addrType_t)pDisassociateCnf->DeviceAddrMode;
        apimacDissCnf.panId = pDisassociateCnf->DevicePanId;
        apimacDissCnf.status = (ApiMac_status_t)pDisassociateCnf->Status;

        pMacCallbacks->pDisassociateCnfCb(&apimacDissCnf);
    }
}
static void ApiMac_processMtMacOrphanIndCB(MtMac_orphanInd_t *pOrphanInd)
{
    if(pMacCallbacks && pMacCallbacks->pOrphanIndCb)
    {
        ApiMac_mlmeOrphanInd_t apimacOrphanInd;
        memcpy(apimacOrphanInd.orphanAddress, pOrphanInd->ExtendedAddress, APIMAC_SADDR_EXT_LEN);
        memcpy(apimacOrphanInd.sec.keySource, pOrphanInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacOrphanInd.sec.securityLevel = pOrphanInd->SecurityLevel;
        apimacOrphanInd.sec.keyIdMode = pOrphanInd->KeyIdMode;
        apimacOrphanInd.sec.keyIndex = pOrphanInd->KeyIndex;

        pMacCallbacks->pOrphanIndCb(&apimacOrphanInd);
    }
}
static void ApiMac_processMtMacPollCnfCB(MtMac_pollCnf_t *pPollCnf)
{
    if(pMacCallbacks && pMacCallbacks->pPollCnfCb)
    {
        ApiMac_mlmePollCnf_t apimacPollCnf;

        apimacPollCnf.framePending = pPollCnf->FramePending;
        apimacPollCnf.status = (ApiMac_status_t)pPollCnf->Status;

        pMacCallbacks->pPollCnfCb(&apimacPollCnf);
    }
}
static void ApiMac_processMtMacPollIndCB(MtMac_pollInd_t *pPollInd)
{
    if(pMacCallbacks && pMacCallbacks->pPollIndCb)
    {
        ApiMac_mlmePollInd_t apimacPollInd;
        ApiMac_mtAddrToApiMacsAddr(&apimacPollInd.srcAddr,
                                   pPollInd->DevAddr,
                                   pPollInd->AddrMode);
        apimacPollInd.srcPanId = pPollInd->PanID;
        apimacPollInd.noRsp = (bool)pPollInd->NoResponse;

        pMacCallbacks->pPollIndCb(&apimacPollInd);
    }
}
static void ApiMac_processMtMacScanCnfCB(MtMac_scanCnf_t *pScanCnf)
{
    if(pMacCallbacks && pMacCallbacks->pScanCnfCb)
    {
        ApiMac_panDesc_t *tempPanDesc = NULL;
        ApiMac_mlmeScanCnf_t apimacScanCnf;
        apimacScanCnf.status = (ApiMac_status_t)pScanCnf->Status;
        apimacScanCnf.scanType = (ApiMac_scantype_t)pScanCnf->ScanType;
        apimacScanCnf.channelPage = pScanCnf->ChannelPage;
        apimacScanCnf.phyId = pScanCnf->PhyId;
        memcpy(apimacScanCnf.unscannedChannels, pScanCnf->UnscannedChannels, APIMAC_154G_CHANNEL_BITMAP_SIZ);
        apimacScanCnf.resultListSize = pScanCnf->ResultListCount;
        if(apimacScanCnf.scanType == ApiMac_scantype_active ||
           apimacScanCnf.scanType == ApiMac_scantype_activeEnhanced ||
           apimacScanCnf.scanType == ApiMac_scantype_passive)
        {
            MtMac_panDesc_t *tempMtPanDesc;
            apimacScanCnf.result.pPanDescriptor = (ApiMac_panDesc_t*)malloc(apimacScanCnf.resultListSize*sizeof(ApiMac_panDesc_t));
            for(uint8_t lCount = 0; lCount < apimacScanCnf.resultListSize; lCount++)
            {
                tempPanDesc = &apimacScanCnf.result.pPanDescriptor[lCount];
                tempMtPanDesc = &pScanCnf->Result.pPanDescriptor[lCount];
                ApiMac_mtPanDescParse(tempPanDesc, tempMtPanDesc);
            }
            tempPanDesc = apimacScanCnf.result.pPanDescriptor;
        }
        else
        {
            apimacScanCnf.result.pEnergyDetect = pScanCnf->Result.pEnergyDetect;
        }

        pMacCallbacks->pScanCnfCb(&apimacScanCnf);
        if(tempPanDesc)
        {
            free(tempPanDesc);
        }
    }
}
static void ApiMac_processMtMacCommStatusIndCB(MtMac_commStatusInd_t *pCommStatusInd)
{
    if(pMacCallbacks && pMacCallbacks->pCommStatusCb)
    {
        ApiMac_mlmeCommStatusInd_t apimacCommStatInd;
        apimacCommStatInd.status = (ApiMac_status_t)pCommStatusInd->Status;
        ApiMac_mtAddrToApiMacsAddr(&apimacCommStatInd.srcAddr,
                                   pCommStatusInd->SrcAddr,
                                   pCommStatusInd->SrcAddrMode);
        ApiMac_mtAddrToApiMacsAddr(&apimacCommStatInd.dstAddr,
                                   pCommStatusInd->DstAddr,
                                   pCommStatusInd->DstAddrMode);
        apimacCommStatInd.panId = pCommStatusInd->DevicePanId;
        apimacCommStatInd.reason = (ApiMac_commStatusReason_t)pCommStatusInd->Reason;
        memcpy(apimacCommStatInd.sec.keySource, pCommStatusInd->KeySource, APIMAC_KEY_SOURCE_MAX_LEN);
        apimacCommStatInd.sec.securityLevel = pCommStatusInd->SecurityLevel;
        apimacCommStatInd.sec.keyIdMode = pCommStatusInd->KeyIdMode;
        apimacCommStatInd.sec.keyIndex = pCommStatusInd->KeyIndex;

        pMacCallbacks->pCommStatusCb(&apimacCommStatInd);
    }
}
static void ApiMac_processMtMacStartCnfCB(MtMac_startCnf_t *pStartCnf)
{
    if(pMacCallbacks && pMacCallbacks->pStartCnfCb)
    {
        ApiMac_mlmeStartCnf_t apimacStrtCnf;
        apimacStrtCnf.status = (ApiMac_status_t)pStartCnf->Status;

        pMacCallbacks->pStartCnfCb(&apimacStrtCnf);
    }
}
static void ApiMac_processMtMacWsAsyncCnfCB(MtMac_wsAsyncCnf_t *pWsAsyncCnf)
{
    if(pMacCallbacks && pMacCallbacks->pWsAsyncCnfCb)
    {
        ApiMac_mlmeWsAsyncCnf_t apimacWsAsyncCnf;
        apimacWsAsyncCnf.status = (ApiMac_status_t)pWsAsyncCnf->Status;

        pMacCallbacks->pWsAsyncCnfCb(&apimacWsAsyncCnf);
    }
}




/*!
 * @brief       Copy the common address type from Mac Stack type to App type.
 *
 * @param       pDst - pointer to the application type
 * @param       pSrc - pointer to the mac stack type
 */
//static void copyMacAddrToApiMacAddr(ApiMac_sAddr_t *pDst, sAddr_t *pSrc)
//{
//    /* Copy each element of the structure */
//    pDst->addrMode = (ApiMac_addrType_t)pSrc->addrMode;
//    if(pDst->addrMode == ApiMac_addrType_short)
//    {
//        pDst->addr.shortAddr = pSrc->addr.shortAddr;
//    }
//    else
//    {
//        memcpy(pDst->addr.extAddr, pSrc->addr.extAddr,
//               sizeof(ApiMac_sAddrExt_t));
//    }
//}

/*!
 * @brief       Copy the common address type from Mac Stack type to App type.
 *
 * @param       pDst - pointer to the application type
 * @param       pSrc - pointer to the mac stack type
 */
//static void copyMacPanDescToApiMacPanDesc(ApiMac_panDesc_t *pDst,
//                                          macPanDesc_t *pSrc)
//{
//    /* Copy each element of the structure */
//    copyMacAddrToApiMacAddr(&(pDst->coordAddress), &(pSrc->coordAddress));
//    pDst->coordPanId = (uint16_t)pSrc->coordPanId;
//    pDst->superframeSpec = (uint16_t)pSrc->superframeSpec;
//    pDst->logicalChannel = (uint8_t)pSrc->logicalChannel;
//    pDst->channelPage = (uint8_t)pSrc->channelPage;
//    pDst->gtsPermit = (bool)pSrc->gtsPermit;
//    pDst->linkQuality = (uint8_t)pSrc->linkQuality;
//    pDst->timestamp = (uint32_t)pSrc->timestamp;
//    pDst->securityFailure = (bool)pSrc->securityFailure;
//    memcpy(&(pDst->sec), &(pSrc->sec), sizeof(ApiMac_sec_t));
//}

/*!
 * @brief       Process the incoming Beacon Notification callback.
 *
 * @param       pInd - pointer MAC Beacon indication info
 */
//static void processBeaconNotifyInd(macMlmeBeaconNotifyInd_t *pInd)
//{
//    /* Indication structure */
//    ApiMac_mlmeBeaconNotifyInd_t ind;

//    /* Initialize the structure */
//    memset(&ind, 0, sizeof(ApiMac_mlmeBeaconNotifyInd_t));
//
//    /* copy the message to the indication structure */
//    ind.beaconType = (ApiMac_beaconType_t)pInd->beaconType;
//    ind.bsn = pInd->bsn;
//
//    if(ind.beaconType == ApiMac_beaconType_normal)
//    {
//        uint8_t *pAddrList;
//
//        /* Fill in the PAN descriptor */
//        if(pInd->info.beaconData.pPanDesc)
//        {
//            copyMacPanDescToApiMacPanDesc(&ind.panDesc,
//                                          pInd->info.beaconData.pPanDesc);
//        }
//
//        /* Add the pending address lists for short address and extended address */
//        pAddrList = pInd->info.beaconData.pAddrList;
//        ind.beaconData.beacon.numPendShortAddr = MAC_PEND_NUM_SHORT(
//                        pInd->info.beaconData.pendAddrSpec);
//        ind.beaconData.beacon.numPendExtAddr = MAC_PEND_NUM_EXT(
//                        pInd->info.beaconData.pendAddrSpec);
//        if(ind.beaconData.beacon.numPendShortAddr)
//        {
//            ind.beaconData.beacon.pShortAddrList = (uint16_t *)pAddrList;
//            pAddrList += ind.beaconData.beacon.numPendShortAddr * 2;
//        }
//        if(ind.beaconData.beacon.numPendExtAddr)
//        {
//            ind.beaconData.beacon.pExtAddrList = (uint8_t *)pAddrList;
//        }
//
//        /* Add the beacon payload */
//        ind.beaconData.beacon.sduLength = pInd->info.beaconData.sduLength;
//        ind.beaconData.beacon.pSdu = pInd->info.beaconData.pSdu;
//    }
//    else
//    {
//        /* Fill in the PAN descriptor */
//        if(pInd->info.eBeaconData.pPanDesc)
//        {
//            copyMacPanDescToApiMacPanDesc(&ind.panDesc,
//                                          pInd->info.eBeaconData.pPanDesc);
//        }
//
//        /* Must be an enhanced beacon */
//        ind.beaconData.eBeacon.coexist.beaconOrder = pInd->info.eBeaconData
//                        .coexist.beaconOrder;
//        ind.beaconData.eBeacon.coexist.superFrameOrder = pInd->info.eBeaconData
//                        .coexist.superFrameOrder;
//        ind.beaconData.eBeacon.coexist.finalCapSlot = pInd->info.eBeaconData
//                        .coexist.finalCapSlot;
//        ind.beaconData.eBeacon.coexist.eBeaconOrder = pInd->info.eBeaconData
//                        .coexist.eBeaconOrder;
//        ind.beaconData.eBeacon.coexist.offsetTimeSlot = pInd->info.eBeaconData
//                        .coexist.offsetTimeSlot;
//        ind.beaconData.eBeacon.coexist.capBackOff = pInd->info.eBeaconData
//                        .coexist.capBackOff;
//        ind.beaconData.eBeacon.coexist.eBeaconOrderNBPAN = pInd->info
//                        .eBeaconData.coexist.eBeaconOrderNBPAN;
//    }
//
//    /*
//     * Initiate the callback, no need to check pMacCallbacks or the function
//     * pointer for non-null, the calling function will check the function
//     * pointer
//     */
//    pMacCallbacks->pBeaconNotifyIndCb(&ind);
//}

/*!
 * @brief       Process the incoming Scan Confirm callback.
 *
 * @param       pCnf - pointer MAC Scan Confirm info
 */
//static void processScanCnf(macMlmeScanCnf_t *pCnf)
//{
//    /* Confirmation structure */
//    ApiMac_mlmeScanCnf_t cnf;
//
//    /* Initialize the structure */
//    memset(&cnf, 0, sizeof(ApiMac_mlmeScanCnf_t));
//
//    /* copy the message to the confirmation structure */
//    cnf.status = (ApiMac_status_t)pCnf->hdr.status;
//
//    cnf.scanType = (ApiMac_scantype_t)pCnf->scanType;
//    cnf.channelPage = pCnf->channelPage;
//    cnf.phyId = pCnf->phyID;
//    memcpy(cnf.unscannedChannels, pCnf->unscannedChannels,
//    APIMAC_154G_CHANNEL_BITMAP_SIZ);
//    cnf.resultListSize = pCnf->resultListSize;
//
//    if(cnf.resultListSize)
//    {
//        if(cnf.scanType == ApiMac_scantype_energyDetect)
//        {
//            cnf.result.pEnergyDetect = pCnf->result.pEnergyDetect;
//
//        }
//        else
//        {
//            cnf.result.pPanDescriptor =
//              (ApiMac_panDesc_t*)pCnf->result.pPanDescriptor;
//        }
//    }
//
//    /*
//     * Initiate the callback, no need to check pMacCallbacks or the function
//     * pointer for non-null, the calling function will check the function
//     * pointer
//     */
//    pMacCallbacks->pScanCnfCb(&cnf);
//}

/*!
 * @brief       Deallocate message function, MAC will deallocate the message.
 *
 * @param       pData - pointer to message to deallocate.
 */
//static void macMsgDeallocate(macEventHdr_t *pData)
//{
//    if(pData != NULL)
//    {
//        /* Fill in the message content */
//        pData->event = MAC_MSG_DEALLOCATE;
//        pData->status = 0;
//
//        /* Send the message */
//        ICall_sendServiceMsg(ApiMac_appEntity, ICALL_SERVICE_CLASS_TIMAC,
//                             (ICALL_MSG_FORMAT_KEEP),
//                             pData);
//    }
//}

/*!
 * @brief       Copy the MAC data indication to the API MAC data indication
 *
 * @param       pDst - pointer to the API MAC data indication
 * @param       pSrc - pointer to the MAC data indication
 */
//static void copyDataInd(ApiMac_mcpsDataInd_t *pDst, macMcpsDataInd_t *pSrc)
//{
//    /* Initialize the structure */
//    memset(pDst, 0, sizeof(ApiMac_mcpsDataInd_t));
//
//    /* copy the message to the indication structure */
//    copyMacAddrToApiMacAddr(&(pDst->srcAddr), &(pSrc->mac.srcAddr));
//    copyMacAddrToApiMacAddr(&(pDst->dstAddr), &(pSrc->mac.dstAddr));
//    pDst->timestamp = pSrc->mac.timestamp;
//    pDst->timestamp2 = pSrc->mac.timestamp2;
//    pDst->srcPanId = pSrc->mac.srcPanId;
//    pDst->dstPanId = pSrc->mac.dstPanId;
//    pDst->mpduLinkQuality = pSrc->mac.mpduLinkQuality;
//    pDst->correlation = pSrc->mac.correlation;
//    pDst->rssi = pSrc->mac.rssi;
//    pDst->dsn = pSrc->mac.dsn;
//    pDst->payloadIeLen = pSrc->mac.payloadIeLen;
//    pDst->pPayloadIE = pSrc->mac.pPayloadIE;
//    pDst->fhFrameType = (ApiMac_fhFrameType_t)pSrc->internal.fhFrameType;
//    pDst->fhProtoDispatch = (ApiMac_fhDispatchType_t)pSrc->mac.fhProtoDispatch;
//    pDst->frameCntr = (uint32_t)pSrc->mac.frameCntr;
//    memcpy(&(pDst->sec), &(pSrc->sec), sizeof(ApiMac_sec_t));
//
//    /* Copy the payload information */
//    pDst->msdu.len = pSrc->msdu.len;
//    pDst->msdu.p = pSrc->msdu.p;
//}

/*!
 * @brief       Copy the common address type from App type to Mac Stack type.
 *
 * @param       pDst - pointer to the mac stack type
 * @param       pSrc - pointer to the application type
 */
//void copyApiMacAddrToMacAddr(sAddr_t *pDst, ApiMac_sAddr_t *pSrc)
//{
//    /* Copy each element of the structure */
//    pDst->addrMode = pSrc->addrMode;
//    if(pSrc->addrMode == ApiMac_addrType_short)
//    {
//        pDst->addr.shortAddr = pSrc->addr.shortAddr;
//    }
//    else
//    {
//        memcpy(pDst->addr.extAddr, pSrc->addr.extAddr, sizeof(sAddrExt_t));
//    }
//}

/*!
 * @brief Parses the payload information element.
 *
 * @param pPayload - pointer to the buffer with the payload IEs.
 * @param payloadLen - length of the buffer with the payload IEs.
 * @param pList - pointer to point of place to allocated the link list.
 * @param group - true to check for termination IE.
 *
 * @return      ApiMac_status_t
 */
static ApiMac_status_t parsePayloadIEs(uint8_t *pContent, uint16_t contentLen,
                                       ApiMac_payloadIeRec_t **pList,
                                       bool group)
{
    ApiMac_payloadIeRec_t* pIe = (ApiMac_payloadIeRec_t*) NULL;
    ApiMac_payloadIeRec_t* pTempIe;
    uint16_t lenContent = 0;
    ApiMac_status_t status = ApiMac_status_success;

    if((pContent == NULL) || (contentLen == 0))
    {
        return (ApiMac_status_noData);
    }

    /* Initialize the list pointer */
    *pList = (ApiMac_payloadIeRec_t*) NULL;

    while(lenContent < contentLen)
    {
        uint16_t hdr;
        bool typeLong;
        uint8_t ieId;

        hdr = MAKE_UINT16(pContent[0], pContent[1]);
        pContent += PAYLOAD_IE_HEADER_LEN; /* Move past the header */

        typeLong = GET_SUBIE_TYPE(hdr);
        if(typeLong)
        {
            ieId = GET_SUBIE_ID_LONG(hdr);
        }
        else
        {
            ieId = GET_SUBIE_ID_SHORT(hdr);
        }

        if(group)
        {
            if(!typeLong)
            {
                /* Only long IE types when parsing Group IEs */
                status = ApiMac_status_unsupported;
                break;
            }

            if(ApiMac_payloadIEGroup_term == ieId)
            {
                /* Termination IE found */
                break;
            }
        }

        pTempIe = (ApiMac_payloadIeRec_t *)malloc(
                        sizeof(ApiMac_payloadIeRec_t));

        if(pTempIe)
        {
            memset(pTempIe, 0, sizeof(ApiMac_payloadIeRec_t));

            /* If nothing in the list, add the node first otherwise
             add it to the end of the list */
            if(*pList == NULL)
            {
                *pList = pTempIe;
            }
            else
            {
                /* pIe should point to the previous node,
                 since it was allocated in the previous iteration */
                pIe->pNext = pTempIe;
            }

            pIe = pTempIe;
            pTempIe = NULL;

            /* Fill in the IE information */
            pIe->item.ieTypeLong = typeLong;
            pIe->item.ieId = ieId;

            if(pIe->item.ieTypeLong)
            {
                pIe->item.ieContentLen = GET_SUBIE_LEN_LONG(hdr);
            }
            else
            {
                pIe->item.ieContentLen = GET_SUBIE_LEN_SHORT(hdr);
            }
            pIe->item.pIEContent = pContent;

            /* Update length and pointer */
            lenContent += PAYLOAD_IE_HEADER_LEN + pIe->item.ieContentLen;
            pContent += pIe->item.ieContentLen;
        }
        else
        {
            status = ApiMac_status_noResources;
            break;
        }
    }

    if((status != ApiMac_status_success) && (NULL != *pList))
    {
        /* not successful in parsing all header ie's, free the linked list */
        pIe = *pList;
        while(NULL != pIe)
        {
            pTempIe = pIe->pNext;
            free(pIe);
            pIe = pTempIe;
        }
        *pList = NULL;
    }

    return (status);
}

/*!
 * @brief       Convert API txOptions to bitmasked txOptions.
 *
 * @param       txOptions - tx options structure
 *
 * @return      bitmasked txoptions
 */
uint16_t convertTxOptions(ApiMac_txOptions_t txOptions)
{
    uint16_t retVal = 0;

    if(txOptions.ack == true)
    {
        retVal |= MAC_TXOPTION_ACK;
    }
    if(txOptions.indirect == true)
    {
        retVal |= MAC_TXOPTION_INDIRECT;
    }
    if(txOptions.pendingBit == true)
    {
        retVal |= MAC_TXOPTION_PEND_BIT;
    }
    if(txOptions.noRetransmits == true)
    {
        retVal |= MAC_TXOPTION_NO_RETRANS;
    }
    if(txOptions.noConfirm == true)
    {
        retVal |= MAC_TXOPTION_NO_CNF;
    }
    if(txOptions.useAltBE == true)
    {
        retVal |= MAC_TXOPTION_ALT_BE;
    }
    if(txOptions.usePowerAndChannel == true)
    {
        retVal |= MAC_TXOPTION_PWR_CHAN;
    }

    return (retVal);
}



