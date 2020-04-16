/******************************************************************************

 @file mtMac.h

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
/*============== AREQ CMD1 DEFINES ==============*/

#define MAC_DATA_CNF    0x84
#define MAC_DATA_IND    0x85
#define MAC_PURGE_CNF    0x90
#define MAC_WS_ASYNC_IND    0x93
#define MAC_SYNC_LOSS_IND    0x80
#define MAC_ASSOCIATE_IND    0x81
#define MAC_ASSOCIATE_CNF    0x82
#define MAC_BEACON_NOTIFY_IND    0x83
#define MAC_DISASSOCIATE_IND    0x86
#define MAC_DISASSOCIATE_CNF    0x87
#define MAC_ORPHAN_IND    0x8A
#define MAC_POLL_CNF    0x8B
#define MAC_POLL_IND    0x91
#define MAC_SCAN_CNF    0x8C
#define MAC_COMM_STATUS_IND    0x8D
#define MAC_START_CNF    0x8E
#define MAC_WS_ASYNC_CNF    0x92


#define ENERGY_DETECT_SCAN 0x00
#define ACTIVE_SCAN 0x01
#define PASSIVE_SCAN 0x02
#define ORPHAN_SCAN 0x03
#define ACTIVE_ENHANCED_SCAN 0x05

/*============== AREQ Typedefs ==============*/

typedef struct
{
    uint8_t Status;
    uint8_t Handle;
    uint32_t Timestamp;
    uint16_t Timestamp2;
    uint8_t Retries;
    uint8_t LinkQuality;
    uint8_t Correlation;
    uint8_t RSSI;
    uint32_t FrameCounter;
}MtMac_dataCnf_t;

typedef struct
{
    uint8_t SrcAddrMode;
    uint8_t SrcAddr[8];
    uint8_t DstAddrMode;
    uint8_t DstAddr[8];
    uint32_t Timestamp;
    uint16_t Timestamp2;
    uint16_t SrcPanId;
    uint16_t DstPanId;
    uint8_t LinkQuality;
    uint8_t Correlation;
    uint8_t RSSI;
    uint8_t DSN;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint32_t FrameCounter;
    uint16_t DataLength;
    uint16_t IELength;
    uint8_t *DataPayload;
    uint8_t *IEPayload;
}MtMac_dataInd_t;

typedef struct
{
    uint8_t Status;
    uint8_t Handle;
}MtMac_purgeCnf_t;

typedef struct
{
    uint8_t SrcAddrMode;
    uint8_t SrcAddr[8];
    uint8_t DstAddrMode;
    uint8_t DstAddr[8];
    uint32_t Timestamp;
    uint16_t Timestamp2;
    uint16_t SrcPanId;
    uint16_t DstPanId;
    uint8_t LinkQuality;
    uint8_t Correlation;
    uint8_t RSSI;
    uint8_t DSN;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint32_t FrameCounter;
    uint8_t FrameType;
    uint16_t DataLength;
    uint16_t IELength;
    uint8_t *DataPayload;
    uint8_t *IEPayload;
}MtMac_wsAsyncInd_t;

typedef struct
{
    uint8_t Status;
    uint16_t PanId;
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t PhyId;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_syncLossInd_t;

typedef struct
{
    uint8_t ExtendedAddress[8];
    uint8_t Capabilities;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_associateInd_t;

typedef struct
{
    uint8_t Status;
    uint16_t ShortAddress;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_associateCnf_t;

typedef struct
{
    uint8_t BeaconType;
    uint8_t BSN;
    uint32_t Timestamp;
    uint8_t CoordAddressMode;
    uint8_t CoordExtendedAddress[8];
    uint16_t PanId;
    uint16_t SuperframeSpec;
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t GTSPermit;
    uint8_t LinkQuality;
    uint8_t SecurityFailure;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint8_t ShortAddrs;
    uint8_t ExtAddrs;
    uint8_t SDULength;
    uint8_t *ShortAddrList;
    uint8_t *ExtAddrList;
    uint8_t *NSDU;
}MtMac_beaconNotifyInd_t;

typedef struct
{
    uint8_t ExtendedAddress[8];
    uint8_t DisassociateReason;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_disassociateInd_t;

typedef struct
{
    uint8_t Status;
    uint8_t DeviceAddrMode;
    uint8_t DeviceAddr[8];
    uint16_t DevicePanId;
}MtMac_disassociateCnf_t;

typedef struct
{
    uint8_t ExtendedAddress[8];
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_orphanInd_t;

typedef struct
{
    uint8_t Status;
    uint8_t FramePending;
}MtMac_pollCnf_t;

typedef struct
{
    uint8_t AddrMode;
    uint8_t DevAddr[8];
    uint16_t PanID;
    uint8_t NoResponse;
}MtMac_pollInd_t;

typedef struct
{
    uint8_t CoordAddrMode;
    uint8_t CoordAddress[8];
    uint16_t PanId;
    uint16_t SuperframeSpec;
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t GTSPermit;
    uint8_t LinkQuality;
    uint32_t Timestamp;
    uint8_t SecurityFailure;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_panDesc_t;

typedef struct
{
    uint8_t Status;
    uint8_t ScanType;
    uint8_t ChannelPage;
    uint8_t PhyId;
    uint8_t UnscannedChannels[17];
    uint8_t ResultListCount;
    union
    {
        /*! The list of energy measurements, one for each channel scanned */
        uint8_t *pEnergyDetect;
        /*! The list of PAN descriptors, one for each beacon found */
        MtMac_panDesc_t *pPanDescriptor;
    } Result;
}MtMac_scanCnf_t;


typedef struct
{
    uint8_t Status;
    uint8_t SrcAddrMode;
    uint8_t SrcAddr[8];
    uint8_t DstAddrMode;
    uint8_t DstAddr[8];
    uint16_t DevicePanId;
    uint8_t Reason;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_commStatusInd_t;

typedef struct
{
    uint8_t Status;
}MtMac_startCnf_t;

typedef struct
{
    uint8_t Status;
}MtMac_wsAsyncCnf_t;


typedef void (*MtMac_dataCnfFp_t)(MtMac_dataCnf_t *pDataCnf);
typedef void (*MtMac_dataIndFp_t)(MtMac_dataInd_t *pDataInd);
typedef void (*MtMac_purgeCnfFp_t)(MtMac_purgeCnf_t *pPurgeCnf);
typedef void (*MtMac_wsAsyncIndFp_t)(MtMac_wsAsyncInd_t *pWsAsyncInd);
typedef void (*MtMac_syncLossIndFp_t)(MtMac_syncLossInd_t *pSyncLossInd);
typedef void (*MtMac_associateIndFp_t)(MtMac_associateInd_t *pAssociateInd);
typedef void (*MtMac_associateCnfFp_t)(MtMac_associateCnf_t *pAssociateCnf);
typedef void (*MtMac_beaconNotifyIndFp_t)(MtMac_beaconNotifyInd_t *pBeaconNotifyInd);
typedef void (*MtMac_disassociateIndFp_t)(MtMac_disassociateInd_t *pDisassociateInd);
typedef void (*MtMac_disassociateCnfFp_t)(MtMac_disassociateCnf_t *pDisassociateCnf);
typedef void (*MtMac_orphanIndFp_t)(MtMac_orphanInd_t *pOrphanInd);
typedef void (*MtMac_pollCnfFp_t)(MtMac_pollCnf_t *pPollCnf);
typedef void (*MtMac_pollIndFp_t)(MtMac_pollInd_t *pPollInd);
typedef void (*MtMac_scanCnfFp_t)(MtMac_scanCnf_t *pScanCnf);
typedef void (*MtMac_commStatusIndFp_t)(MtMac_commStatusInd_t *pCommStatusInd);
typedef void (*MtMac_startCnfFp_t)(MtMac_startCnf_t *pStartCnf);
typedef void (*MtMac_wsAsyncCnfFp_t)(MtMac_wsAsyncCnf_t *pWsAsyncCnf);

typedef struct
{
    MtMac_dataCnfFp_t pDataCnfCb;
    MtMac_dataIndFp_t pDataIndCb;
    MtMac_purgeCnfFp_t pPurgeCnfCb;
    MtMac_wsAsyncIndFp_t pWsAsyncIndCb;
    MtMac_syncLossIndFp_t pSyncLossIndCb;
    MtMac_associateIndFp_t pAssociateIndCb;
    MtMac_associateCnfFp_t pAssociateCnfCb;
    MtMac_beaconNotifyIndFp_t pBeaconNotifyIndCb;
    MtMac_disassociateIndFp_t pDisassociateIndCb;
    MtMac_disassociateCnfFp_t pDisassociateCnfCb;
    MtMac_orphanIndFp_t pOrphanIndCb;
    MtMac_pollCnfFp_t pPollCnfCb;
    MtMac_pollIndFp_t pPollIndCb;
    MtMac_scanCnfFp_t pScanCnfCb;
    MtMac_commStatusIndFp_t pCommStatusIndCb;
    MtMac_startCnfFp_t pStartCnfCb;
    MtMac_wsAsyncCnfFp_t pWsAsyncCnfCb;

}MtMac_callbacks_t;
void MtMac_RegisterCbs(MtMac_callbacks_t *pCbFuncs);


/*============== SREQ CMD1 DEFINES ==============*/

#define MAC_INIT    0x02
#define MAC_DATA_REQ    0x05
#define MAC_PURGE_REQ    0x0E
#define MAC_ASSOCIATE_REQ    0x06
#define MAC_ASSOCIATE_RSP    0x50
#define MAC_DISASSOCIATE_REQ    0x07
#define MAC_GET_REQ    0x08
#define MAC_SET_REQ    0x09
#define MAC_SECURITY_GET_REQ    0x30
#define MAC_SECURITY_SET_REQ    0x31
#define MAC_UPDATE_PANID_REQ    0x32
#define MAC_ADD_DEVICE_REQ    0x33
#define MAC_DELETE_DEVICE_REQ    0x34
#define MAC_DELETE_ALL_DEVICES_REQ    0x35
#define MAC_DELETE_KEY_REQ    0x36
#define MAC_READ_KEY_REQ    0x37
#define MAC_WRITE_KEY_REQ    0x38
#define MAC_ORPHAN_RSP    0x51
#define MAC_POLL_REQ    0x0D
#define MAC_RESET_REQ    0x01
#define MAC_SCAN_REQ    0x0C
#define MAC_START_REQ    0x03
#define MAC_SYNC_REQ    0x04
#define MAC_SET_RX_GAIN_REQ    0x0F
#define MAC_WS_ASYNC_REQ    0x44
#define MAC_FH_ENABLE_REQ    0x40
#define MAC_FH_START_REQ    0x41
#define MAC_FH_GET_REQ    0x42
#define MAC_FH_SET_REQ    0x43

/*============== SREQ Typedefs ==============*/


typedef struct
{
    uint8_t DestAddressMode;
    uint8_t DestAddress[8];
    uint16_t DestPanId;
    uint8_t SrcAddressMode;
    uint8_t Handle;
    uint8_t TxOption;
    uint8_t Channel;
    uint8_t Power;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint32_t IncludeFhIEs;
    uint16_t DataLength;
    uint16_t IELength;
    uint8_t *DataPayload;
    uint8_t *IEPayload;
}MtMac_dataReq_t;

typedef struct
{
    uint8_t Handle;
}MtMac_purgeReq_t;

typedef struct
{
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t PhyId;
    uint8_t CoordAddressMode;
    uint8_t CoordAddress[8];
    uint16_t CoordPanId;
    uint8_t CapabilityInformation;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_associateReq_t;

typedef struct
{
    uint8_t ExtendedAddress[8];
    uint16_t AssocShortAddress;
    uint8_t AssocStatus;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_associateRsp_t;

typedef struct
{
    uint8_t DeviceAddressMode;
    uint8_t DeviceAddress[8];
    uint16_t DevicePanId;
    uint8_t DisassociateReason;
    uint8_t TxIndirect;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_disassociateReq_t;

typedef struct
{
    uint8_t AttributeID;
}MtMac_getReq_t;
typedef struct
{
    uint8_t *Data;
    uint8_t AttrLen;
}MtMac_getReqSrsp_t;

typedef struct
{
    uint8_t AttributeID;
    uint8_t AttributeValue[16];
}MtMac_setReq_t;

typedef struct
{
    uint8_t AttributeID;
    uint8_t Index1;
    uint8_t Index2;
}MtMac_securityGetReq_t;
typedef struct
{
    uint8_t Index1;
    uint8_t Index2;
    uint8_t *Data;
    uint8_t AttrLen;
}MtMac_securityGetReqSrsp_t;

typedef struct
{
    uint8_t AttributeID;
    uint8_t Index1;
    uint8_t Index2;
    uint8_t *AttrValue;
    uint8_t AttrLen;
}MtMac_securitySetReq_t;

typedef struct
{
    uint16_t PanID;
}MtMac_updatePanidReq_t;

typedef struct
{
    uint16_t PanID;
    uint16_t ShortAddr;
    uint8_t ExtAddr[8];
    uint32_t FrameCounter;
    uint8_t Exempt;
    uint8_t Unique;
    uint8_t Duplicate;
    uint8_t DataSize;
    uint8_t LookupData[9];
}MtMac_addDeviceReq_t;

typedef struct
{
    uint8_t ExtAddr[8];
}MtMac_deleteDeviceReq_t;


typedef struct
{
    uint8_t Index;
}MtMac_deleteKeyReq_t;

typedef struct
{
    uint8_t Index;
}MtMac_readKeyReq_t;
typedef struct
{
    uint32_t FrameCounter;
}MtMac_readKeyReqSrsp_t;

typedef struct
{
    uint8_t New;
    uint8_t Index;
    uint8_t Key[16];
    uint32_t FrameCounter;
    uint8_t DataSize;
    uint8_t LookupData[9];
}MtMac_writeKeyReq_t;

typedef struct
{
    uint8_t ExtendedAddress[8];
    uint16_t AssocShortAddress;
    uint8_t AssociatedMember;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_orphanRsp_t;

typedef struct
{
    uint8_t CoordAddressMode;
    uint8_t CoordAddress[8];
    uint16_t CoordPanId;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
}MtMac_pollReq_t;

typedef struct
{
    uint8_t SetDefault;
}MtMac_resetReq_t;

typedef struct
{
    uint8_t ScanType;
    uint8_t ScanDuration;
    uint8_t ChannelPage;
    uint8_t PhyId;
    uint8_t MaxResults;
    uint8_t PermitJoin;
    uint8_t LinkQuality;
    uint8_t RspFilter;
    uint8_t MpmScan;
    uint8_t MpmType;
    uint16_t MpmDuration;
    uint8_t KeySource[8];
    uint8_t SecLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint8_t Channels[17];
}MtMac_scanReq_t;

typedef struct
{
    uint32_t StartTime;
    uint16_t PanId;
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t PhyId;
    uint8_t BeaconOrder;
    uint8_t SuperFrameOrder;
    uint8_t PanCoordinator;
    uint8_t BatteryLifeExt;
    uint8_t CoordRealignment;
    uint8_t RealignKeySource[8];
    uint8_t RealignSecurityLevel;
    uint8_t RealignKeyIdMode;
    uint8_t RealignKeyIndex;
    uint8_t BeaconKeySource[8];
    uint8_t BeaconSecurityLevel;
    uint8_t BeaconKeyIdMode;
    uint8_t BeaconKeyIndex;
    uint8_t StartFH;
    uint8_t EnhBeaconOrder;
    uint8_t OfsTimeSlot;
    uint16_t NonBeaconOrder;
    uint8_t NumIEs;
    uint8_t *IEIDList;
}MtMac_startReq_t;

typedef struct
{
    uint8_t LogicalChannel;
    uint8_t ChannelPage;
    uint8_t TrackBeacon;
    uint8_t PhyId;
}MtMac_syncReq_t;

typedef struct
{
    uint8_t Mode;
}MtMac_setRxGainReq_t;

typedef struct
{
    uint8_t Operation;
    uint8_t FrameType;
    uint8_t KeySource[8];
    uint8_t SecurityLevel;
    uint8_t KeyIdMode;
    uint8_t KeyIndex;
    uint8_t Channels[17];
}MtMac_wsAsyncReq_t;



typedef struct
{
    uint16_t AttributeID;
}MtMac_fhGetReq_t;
typedef struct
{
    uint8_t AttrLen;
    uint8_t *Data;
}MtMac_fhGetReqSrsp_t;

typedef struct
{
    uint16_t AttributeID;
    uint8_t *Data;
    uint8_t AttrLen;
}MtMac_fhSetReq_t;


/*============== SREQ Declarations ==============*/

uint8_t MtMac_init(void);
uint8_t MtMac_dataReq(MtMac_dataReq_t *pData);
uint8_t MtMac_purgeReq(MtMac_purgeReq_t *pData);
uint8_t MtMac_associateReq(MtMac_associateReq_t *pData);
uint8_t MtMac_associateRsp(MtMac_associateRsp_t *pData);
uint8_t MtMac_disassociateReq(MtMac_disassociateReq_t *pData);
uint8_t MtMac_getReq(MtMac_getReq_t *pData, MtMac_getReqSrsp_t *pRspData);
uint8_t MtMac_setReq(MtMac_setReq_t *pData);
uint8_t MtMac_securityGetReq(MtMac_securityGetReq_t *pData, MtMac_securityGetReqSrsp_t *pRspData);
uint8_t MtMac_securitySetReq(MtMac_securitySetReq_t *pData);
uint8_t MtMac_updatePanidReq(MtMac_updatePanidReq_t *pData);
uint8_t MtMac_addDeviceReq(MtMac_addDeviceReq_t *pData);
uint8_t MtMac_deleteDeviceReq(MtMac_deleteDeviceReq_t *pData);
uint8_t MtMac_deleteAllDevicesReq(void);
uint8_t MtMac_deleteKeyReq(MtMac_deleteKeyReq_t *pData);
uint8_t MtMac_readKeyReq(MtMac_readKeyReq_t *pData, MtMac_readKeyReqSrsp_t *pRspData);
uint8_t MtMac_writeKeyReq(MtMac_writeKeyReq_t *pData);
uint8_t MtMac_orphanRsp(MtMac_orphanRsp_t *pData);
uint8_t MtMac_pollReq(MtMac_pollReq_t *pData);
uint8_t MtMac_resetReq(MtMac_resetReq_t *pData);
uint8_t MtMac_scanReq(MtMac_scanReq_t *pData);
uint8_t MtMac_startReq(MtMac_startReq_t *pData);
uint8_t MtMac_syncReq(MtMac_syncReq_t *pData);
uint8_t MtMac_setRxGainReq(MtMac_setRxGainReq_t *pData);
uint8_t MtMac_wsAsyncReq(MtMac_wsAsyncReq_t *pData);
uint8_t MtMac_fhEnableReq(void);
uint8_t MtMac_fhStartReq(void);
uint8_t MtMac_fhGetReq(MtMac_fhGetReq_t *pData, MtMac_fhGetReqSrsp_t *pRspData);
uint8_t MtMac_fhSetReq(MtMac_fhSetReq_t *pData);

