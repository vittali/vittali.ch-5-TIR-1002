/******************************************************************************

 @file mtMac.c

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
#include <stdint.h>
#include <Utils/util.h>
#include <Common/commonDefs.h>
#include <NPI/npiParse.h>
#include "mtMac.h"


/*============== AREQs ==============*/

static void MtMac_processDataCnf(mtMsg_t *inMtCmd);
static void MtMac_processDataInd(mtMsg_t *inMtCmd);
static void MtMac_processPurgeCnf(mtMsg_t *inMtCmd);
static void MtMac_processWsAsyncInd(mtMsg_t *inMtCmd);
static void MtMac_processSyncLossInd(mtMsg_t *inMtCmd);
static void MtMac_processAssociateInd(mtMsg_t *inMtCmd);
static void MtMac_processAssociateCnf(mtMsg_t *inMtCmd);
static void MtMac_processBeaconNotifyInd(mtMsg_t *inMtCmd);
static void MtMac_processDisassociateInd(mtMsg_t *inMtCmd);
static void MtMac_processDisassociateCnf(mtMsg_t *inMtCmd);
static void MtMac_processOrphanInd(mtMsg_t *inMtCmd);
static void MtMac_processPollCnf(mtMsg_t *inMtCmd);
static void MtMac_processPollInd(mtMsg_t *inMtCmd);
static void MtMac_processScanCnf(mtMsg_t *inMtCmd);
static void MtMac_processCommStatusInd(mtMsg_t *inMtCmd);
static void MtMac_processStartCnf(mtMsg_t *inMtCmd);
static void MtMac_processWsAsyncCnf(mtMsg_t *inMtCmd);

/*============== Process AREQs ==============*/

static MtMac_callbacks_t *mtMacCbs = NULL;

void MtMac_RegisterCbs(MtMac_callbacks_t *pCbFuncs)
{
    mtMacCbs = pCbFuncs;
}

void MtMac_ProcessCmd(mtMsg_t *inMtCmd)
{

	if (mtMacCbs && (inMtCmd->cmd0 & MT_CMD_TYPE_MASK) == MT_CMD_AREQ)
	{
		switch (inMtCmd->cmd1)
		{
            case MAC_DATA_CNF:
			    MtMac_processDataCnf(inMtCmd);
			break;
            case MAC_DATA_IND:
			    MtMac_processDataInd(inMtCmd);
			break;
            case MAC_PURGE_CNF:
			    MtMac_processPurgeCnf(inMtCmd);
			break;
            case MAC_WS_ASYNC_IND:
			    MtMac_processWsAsyncInd(inMtCmd);
			break;
            case MAC_SYNC_LOSS_IND:
			    MtMac_processSyncLossInd(inMtCmd);
			break;
            case MAC_ASSOCIATE_IND:
			    MtMac_processAssociateInd(inMtCmd);
			break;
            case MAC_ASSOCIATE_CNF:
			    MtMac_processAssociateCnf(inMtCmd);
			break;
            case MAC_BEACON_NOTIFY_IND:
			    MtMac_processBeaconNotifyInd(inMtCmd);
			break;
            case MAC_DISASSOCIATE_IND:
			    MtMac_processDisassociateInd(inMtCmd);
			break;
            case MAC_DISASSOCIATE_CNF:
			    MtMac_processDisassociateCnf(inMtCmd);
			break;
            case MAC_ORPHAN_IND:
			    MtMac_processOrphanInd(inMtCmd);
			break;
            case MAC_POLL_CNF:
			    MtMac_processPollCnf(inMtCmd);
			break;
            case MAC_POLL_IND:
			    MtMac_processPollInd(inMtCmd);
			break;
            case MAC_SCAN_CNF:
			    MtMac_processScanCnf(inMtCmd);
			break;
            case MAC_COMM_STATUS_IND:
			    MtMac_processCommStatusInd(inMtCmd);
			break;
            case MAC_START_CNF:
			    MtMac_processStartCnf(inMtCmd);
			break;
            case MAC_WS_ASYNC_CNF:
			    MtMac_processWsAsyncCnf(inMtCmd);
			break;

            default:
                //CMD1 not handled
			break;
		}
	}
	else
	{
		//We received a different type of message print log or report back
	}
}

void MtMac_processDataCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pDataCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_dataCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.Handle = *attrPtr;
        attrPtr++;
        aReqCmd.Timestamp = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.Timestamp2 = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.Retries = *attrPtr;
        attrPtr++;
        aReqCmd.LinkQuality = *attrPtr;
        attrPtr++;
        aReqCmd.Correlation = *attrPtr;
        attrPtr++;
        aReqCmd.RSSI = *attrPtr;
        attrPtr++;
        aReqCmd.FrameCounter = Util_parseUint32(attrPtr);

        mtMacCbs->pDataCnfCb(&aReqCmd);
	}
}

void MtMac_processDataInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pDataIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_dataInd_t aReqCmd;

        aReqCmd.SrcAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.SrcAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DstAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.DstAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.Timestamp = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.Timestamp2 = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.SrcPanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.DstPanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.LinkQuality = *attrPtr;
        attrPtr++;
        aReqCmd.Correlation = *attrPtr;
        attrPtr++;
        aReqCmd.RSSI = *attrPtr;
        attrPtr++;
        aReqCmd.DSN = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;
        attrPtr++;
        aReqCmd.FrameCounter = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.DataLength = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.IELength = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.DataPayload = attrPtr;
        attrPtr += aReqCmd.DataLength;
        aReqCmd.IEPayload = attrPtr;

        mtMacCbs->pDataIndCb(&aReqCmd);
	}
}

void MtMac_processPurgeCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pPurgeCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_purgeCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.Handle = *attrPtr;

        mtMacCbs->pPurgeCnfCb(&aReqCmd);
	}
}

void MtMac_processWsAsyncInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pWsAsyncIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_wsAsyncInd_t aReqCmd;

        aReqCmd.SrcAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.SrcAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DstAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.DstAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.Timestamp = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.Timestamp2 = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.SrcPanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.DstPanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.LinkQuality = *attrPtr;
        attrPtr++;
        aReqCmd.Correlation = *attrPtr;
        attrPtr++;
        aReqCmd.RSSI = *attrPtr;
        attrPtr++;
        aReqCmd.DSN = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;
        attrPtr++;
        aReqCmd.FrameCounter = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.FrameType = *attrPtr;
        attrPtr++;
        aReqCmd.DataLength = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.IELength = Util_parseUint16(attrPtr);
        attrPtr += 2;

        aReqCmd.DataPayload = attrPtr;
        attrPtr += aReqCmd.DataLength;
        aReqCmd.IEPayload = attrPtr;

        mtMacCbs->pWsAsyncIndCb(&aReqCmd);
	}
}

void MtMac_processSyncLossInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pSyncLossIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_syncLossInd_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.PanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.LogicalChannel = *attrPtr;
        attrPtr++;
        aReqCmd.ChannelPage = *attrPtr;
        attrPtr++;
        aReqCmd.PhyId = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pSyncLossIndCb(&aReqCmd);
	}
}

void MtMac_processAssociateInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pAssociateIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_associateInd_t aReqCmd;

        memcpy(aReqCmd.ExtendedAddress, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.Capabilities = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pAssociateIndCb(&aReqCmd);
	}
}

void MtMac_processAssociateCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pAssociateCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_associateCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.ShortAddress = Util_parseUint16(attrPtr);
        attrPtr += 2;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pAssociateCnfCb(&aReqCmd);
	}
}

void MtMac_processBeaconNotifyInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pBeaconNotifyIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_beaconNotifyInd_t aReqCmd;

        aReqCmd.BeaconType = *attrPtr;
        attrPtr++;
        aReqCmd.BSN = *attrPtr;
        attrPtr++;
        aReqCmd.Timestamp = Util_parseUint32(attrPtr);
        attrPtr += 4;
        aReqCmd.CoordAddressMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.CoordExtendedAddress, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.PanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.SuperframeSpec = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.LogicalChannel = *attrPtr;
        attrPtr++;
        aReqCmd.ChannelPage = *attrPtr;
        attrPtr++;
        aReqCmd.GTSPermit = *attrPtr;
        attrPtr++;
        aReqCmd.LinkQuality = *attrPtr;
        attrPtr++;
        aReqCmd.SecurityFailure = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;
        attrPtr++;
        aReqCmd.ShortAddrs = *attrPtr;
        attrPtr++;
        aReqCmd.ExtAddrs = *attrPtr;
        attrPtr++;
        aReqCmd.SDULength = *attrPtr;
        attrPtr++;

        aReqCmd.ShortAddrList = attrPtr;
        attrPtr += (2 * aReqCmd.ShortAddrs);
        aReqCmd.ExtAddrList = attrPtr;
        attrPtr += (8 * aReqCmd.ExtAddrs);
        aReqCmd.NSDU = attrPtr;

        mtMacCbs->pBeaconNotifyIndCb(&aReqCmd);
	}
}

void MtMac_processDisassociateInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pDisassociateIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_disassociateInd_t aReqCmd;

        memcpy(aReqCmd.ExtendedAddress, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DisassociateReason = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pDisassociateIndCb(&aReqCmd);
	}
}

void MtMac_processDisassociateCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pDisassociateCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_disassociateCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.DeviceAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.DeviceAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DevicePanId = Util_parseUint16(attrPtr);

        mtMacCbs->pDisassociateCnfCb(&aReqCmd);
	}
}

void MtMac_processOrphanInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pOrphanIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_orphanInd_t aReqCmd;

        memcpy(aReqCmd.ExtendedAddress, attrPtr, 8);
        attrPtr += 8;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pOrphanIndCb(&aReqCmd);
	}
}

void MtMac_processPollCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pPollCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_pollCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.FramePending = *attrPtr;

        mtMacCbs->pPollCnfCb(&aReqCmd);
	}
}

void MtMac_processPollInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pPollIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_pollInd_t aReqCmd;

        aReqCmd.AddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.DevAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.PanID = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.NoResponse = *attrPtr;

        mtMacCbs->pPollIndCb(&aReqCmd);
	}
}

void MtMac_processScanCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pScanCnfCb)
	{
	    uint8_t *attrPtr = inMtCmd->attrs;
	    MtMac_panDesc_t *tempPanDesc = NULL;
        MtMac_scanCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.ScanType = *attrPtr;
        attrPtr++;
        aReqCmd.ChannelPage = *attrPtr;
        attrPtr++;
        aReqCmd.PhyId = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.UnscannedChannels, attrPtr, 17);
        attrPtr += 17;
        aReqCmd.ResultListCount = *attrPtr;
        attrPtr++;
        if(aReqCmd.ScanType == ACTIVE_SCAN ||
           aReqCmd.ScanType == ACTIVE_ENHANCED_SCAN ||
           aReqCmd.ScanType == PASSIVE_SCAN)
        {
            uint8_t count = aReqCmd.ResultListCount;
            aReqCmd.Result.pPanDescriptor = (MtMac_panDesc_t*)malloc(count * sizeof(MtMac_panDesc_t));
            tempPanDesc = aReqCmd.Result.pPanDescriptor;

            while(count--)
            {
                tempPanDesc->CoordAddrMode = *attrPtr;
                attrPtr++;
                memcpy(tempPanDesc->CoordAddress, attrPtr, 8);
                attrPtr += 8;
                tempPanDesc->PanId = Util_parseUint16(attrPtr);
                attrPtr += 2;
                tempPanDesc->SuperframeSpec = Util_parseUint16(attrPtr);
                attrPtr += 2;
                tempPanDesc->LogicalChannel = *attrPtr;
                attrPtr++;
                tempPanDesc->ChannelPage = *attrPtr;
                attrPtr++;
                tempPanDesc->GTSPermit = *attrPtr;
                attrPtr++;
                tempPanDesc->LinkQuality = *attrPtr;
                attrPtr++;
                tempPanDesc->Timestamp = Util_parseUint32(attrPtr);
                attrPtr += 4;
                tempPanDesc->SecurityFailure = *attrPtr;
                attrPtr++;
                memcpy(tempPanDesc->KeySource, attrPtr, 8);
                attrPtr += 8;
                tempPanDesc->SecurityLevel = *attrPtr;
                attrPtr++;
                tempPanDesc->KeyIdMode = *attrPtr;
                attrPtr++;
                tempPanDesc->KeyIndex = *attrPtr;
                tempPanDesc++;
            }
            tempPanDesc = aReqCmd.Result.pPanDescriptor;
        }
        else
        {
            aReqCmd.Result.pEnergyDetect = attrPtr;
        }


        mtMacCbs->pScanCnfCb(&aReqCmd);
        if(tempPanDesc)
        {
            free(tempPanDesc);
        }
	}
}

void MtMac_processCommStatusInd(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pCommStatusIndCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_commStatusInd_t aReqCmd;

        aReqCmd.Status = *attrPtr;
        attrPtr++;
        aReqCmd.SrcAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.SrcAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DstAddrMode = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.DstAddr, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.DevicePanId = Util_parseUint16(attrPtr);
        attrPtr += 2;
        aReqCmd.Reason = *attrPtr;
        attrPtr++;
        memcpy(aReqCmd.KeySource, attrPtr, 8);
        attrPtr += 8;
        aReqCmd.SecurityLevel = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIdMode = *attrPtr;
        attrPtr++;
        aReqCmd.KeyIndex = *attrPtr;

        mtMacCbs->pCommStatusIndCb(&aReqCmd);
	}
}

void MtMac_processStartCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pStartCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_startCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;

        mtMacCbs->pStartCnfCb(&aReqCmd);
	}
}

void MtMac_processWsAsyncCnf(mtMsg_t *inMtCmd)
{
    if (mtMacCbs->pWsAsyncCnfCb)
	{

	    uint8_t *attrPtr = inMtCmd->attrs;
        MtMac_wsAsyncCnf_t aReqCmd;

        aReqCmd.Status = *attrPtr;

        mtMacCbs->pWsAsyncCnfCb(&aReqCmd);
	}
}


/*============== SREQs ==============*/

uint8_t MtMac_init(void)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_INIT;


    Mt_sendCmd(&cmdDesc);

    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_dataReq(MtMac_dataReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x23 + pData->DataLength + pData->IELength;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_DATA_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->DestAddressMode;
    sreqBuf++;
    memcpy(sreqBuf, pData->DestAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->DestPanId);
    sreqBuf += 2;
    *sreqBuf = pData->SrcAddressMode;
    sreqBuf++;
    *sreqBuf = pData->Handle;
    sreqBuf++;
    *sreqBuf = pData->TxOption;
    sreqBuf++;
    *sreqBuf = pData->Channel;
    sreqBuf++;
    *sreqBuf = pData->Power;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;
    sreqBuf++;
    Util_bufferUint32(sreqBuf, pData->IncludeFhIEs);
    sreqBuf += 4;
    Util_bufferUint16(sreqBuf, pData->DataLength);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->IELength);
    sreqBuf += 2;
    memcpy(sreqBuf, pData->DataPayload, pData->DataLength);
    sreqBuf += pData->DataLength;
    memcpy(sreqBuf, pData->IEPayload, pData->IELength);

//*sreqBuf = pData->DataPayload;//ATTRSIZE: DataLengthTODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

//*sreqBuf = pData->IEPayload;//ATTRSIZE: IELengthTODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_purgeReq(MtMac_purgeReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_PURGE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Handle;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_associateReq(MtMac_associateReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x1A;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_ASSOCIATE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->LogicalChannel;
    sreqBuf++;
    *sreqBuf = pData->ChannelPage;
    sreqBuf++;
    *sreqBuf = pData->PhyId;
    sreqBuf++;
    *sreqBuf = pData->CoordAddressMode;
    sreqBuf++;
    memcpy(sreqBuf, pData->CoordAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->CoordPanId);
    sreqBuf += 2;
    *sreqBuf = pData->CapabilityInformation;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_associateRsp(MtMac_associateRsp_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x16;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_ASSOCIATE_RSP;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    memcpy(sreqBuf, pData->ExtendedAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->AssocShortAddress);
    sreqBuf += 2;
    *sreqBuf = pData->AssocStatus;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_disassociateReq(MtMac_disassociateReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x18;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_DISASSOCIATE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->DeviceAddressMode;
    sreqBuf++;
    memcpy(sreqBuf, pData->DeviceAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->DevicePanId);
    sreqBuf += 2;
    *sreqBuf = pData->DisassociateReason;
    sreqBuf++;
    *sreqBuf = pData->TxIndirect;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_getReq(MtMac_getReq_t *pData, MtMac_getReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_GET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->AttributeID;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->AttrLen = cmdDesc.len - 1;
            memcpy(pRspData->Data, srspAttrBuf, pRspData->AttrLen);

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_setReq(MtMac_setReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x11;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->AttributeID;
    sreqBuf++;
    memcpy(sreqBuf, pData->AttributeValue, 16);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_securityGetReq(MtMac_securityGetReq_t *pData, MtMac_securityGetReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x03;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SECURITY_GET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->AttributeID;
    sreqBuf++;
    *sreqBuf = pData->Index1;
    sreqBuf++;
    *sreqBuf = pData->Index2;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Index1 = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Index2 = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->AttrLen = cmdDesc.len - 0x03;
            if(pRspData->Data)
            {
                memcpy(pRspData->Data, srspAttrBuf, pRspData->AttrLen);
            }
            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_securitySetReq(MtMac_securitySetReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 3 + pData->AttrLen;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SECURITY_SET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->AttributeID;
    sreqBuf++;
    *sreqBuf = pData->Index1;
    sreqBuf++;
    *sreqBuf = pData->Index2;
    sreqBuf++;
    memcpy(sreqBuf, pData->AttrValue, pData->AttrLen);

//*sreqBuf = pData->Attribute;//ATTRSIZE: ALTODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_updatePanidReq(MtMac_updatePanidReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x02;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_UPDATE_PANID_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint16(sreqBuf, pData->PanID);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_addDeviceReq(MtMac_addDeviceReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x1D;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_ADD_DEVICE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint16(sreqBuf, pData->PanID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->ShortAddr);
    sreqBuf += 2;
    memcpy(sreqBuf, pData->ExtAddr, 8);
    sreqBuf += 8;
    Util_bufferUint32(sreqBuf, pData->FrameCounter);
    sreqBuf += 4;
    *sreqBuf = pData->Exempt;
    sreqBuf++;
    *sreqBuf = pData->Unique;
    sreqBuf++;
    *sreqBuf = pData->Duplicate;
    sreqBuf++;
    *sreqBuf = pData->DataSize;
    sreqBuf++;
    memcpy(sreqBuf, pData->LookupData, 9);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_deleteDeviceReq(MtMac_deleteDeviceReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x08;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_DELETE_DEVICE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    memcpy(sreqBuf, pData->ExtAddr, 8);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_deleteAllDevicesReq(void)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_DELETE_ALL_DEVICES_REQ;


    Mt_sendCmd(&cmdDesc);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_deleteKeyReq(MtMac_deleteKeyReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_DELETE_KEY_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Index;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_readKeyReq(MtMac_readKeyReq_t *pData, MtMac_readKeyReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_READ_KEY_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Index;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->FrameCounter = Util_parseUint32(srspAttrBuf);

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_writeKeyReq(MtMac_writeKeyReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x20;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_WRITE_KEY_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->New;
    sreqBuf++;
    *sreqBuf = pData->Index;
    sreqBuf++;
    memcpy(sreqBuf, pData->Key, 16);
    sreqBuf += 16;
    Util_bufferUint32(sreqBuf, pData->FrameCounter);
    sreqBuf += 4;
    *sreqBuf = pData->DataSize;
    sreqBuf++;
    memcpy(sreqBuf, pData->LookupData, 9);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_orphanRsp(MtMac_orphanRsp_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = //[ERROR] MODIFY MANUALLY BAD LENGTH: 0x016;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_ORPHAN_RSP;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    memcpy(sreqBuf, pData->ExtendedAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->AssocShortAddress);
    sreqBuf += 2;
    *sreqBuf = pData->AssociatedMember;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_pollReq(MtMac_pollReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x16;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_POLL_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->CoordAddressMode;
    sreqBuf++;
    memcpy(sreqBuf, pData->CoordAddress, 8);
    sreqBuf += 8;
    Util_bufferUint16(sreqBuf, pData->CoordPanId);
    sreqBuf += 2;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_resetReq(MtMac_resetReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_RESET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SetDefault;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_scanReq(MtMac_scanReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x28;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SCAN_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->ScanType;
    sreqBuf++;
    *sreqBuf = pData->ScanDuration;
    sreqBuf++;
    *sreqBuf = pData->ChannelPage;
    sreqBuf++;
    *sreqBuf = pData->PhyId;
    sreqBuf++;
    *sreqBuf = pData->MaxResults;
    sreqBuf++;
    *sreqBuf = pData->PermitJoin;
    sreqBuf++;
    *sreqBuf = pData->LinkQuality;
    sreqBuf++;
    *sreqBuf = pData->RspFilter;
    sreqBuf++;
    *sreqBuf = pData->MpmScan;
    sreqBuf++;
    *sreqBuf = pData->MpmType;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->MpmDuration);
    sreqBuf += 2;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;
    sreqBuf++;
    memcpy(sreqBuf, pData->Channels, 17);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_startReq(MtMac_startReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    if(pData->IEIDList == NULL)
    {
        pData->NumIEs = 0;
    }
    cmdDesc.len = 0x2A + pData->NumIEs;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_START_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint32(sreqBuf, pData->StartTime);
    sreqBuf += 4;
    Util_bufferUint16(sreqBuf, pData->PanId);
    sreqBuf += 2;
    *sreqBuf = pData->LogicalChannel;
    sreqBuf++;
    *sreqBuf = pData->ChannelPage;
    sreqBuf++;
    *sreqBuf = pData->PhyId;
    sreqBuf++;
    *sreqBuf = pData->BeaconOrder;
    sreqBuf++;
    *sreqBuf = pData->SuperFrameOrder;
    sreqBuf++;
    *sreqBuf = pData->PanCoordinator;
    sreqBuf++;
    *sreqBuf = pData->BatteryLifeExt;
    sreqBuf++;
    *sreqBuf = pData->CoordRealignment;
    sreqBuf++;
    memcpy(sreqBuf, pData->RealignKeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->RealignSecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->RealignKeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->RealignKeyIndex;
    sreqBuf++;
    memcpy(sreqBuf, pData->BeaconKeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->BeaconSecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->BeaconKeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->BeaconKeyIndex;
    sreqBuf++;
    *sreqBuf = pData->StartFH;
    sreqBuf++;
    *sreqBuf = pData->EnhBeaconOrder;
    sreqBuf++;
    *sreqBuf = pData->OfsTimeSlot;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->NonBeaconOrder);
    sreqBuf += 2;
    *sreqBuf = pData->NumIEs;
    if(pData->IEIDList)
    {
        sreqBuf++;
        memcpy(sreqBuf, pData->IEIDList, pData->NumIEs);
    }


//*sreqBuf = pData->IEIDList;//ATTRSIZE: NumIEsTODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_syncReq(MtMac_syncReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x04;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SYNC_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->LogicalChannel;
    sreqBuf++;
    *sreqBuf = pData->ChannelPage;
    sreqBuf++;
    *sreqBuf = pData->TrackBeacon;
    sreqBuf++;
    *sreqBuf = pData->PhyId;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_setRxGainReq(MtMac_setRxGainReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_SET_RX_GAIN_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Mode;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_wsAsyncReq(MtMac_wsAsyncReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x1E;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_WS_ASYNC_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Operation;
    sreqBuf++;
    *sreqBuf = pData->FrameType;
    sreqBuf++;
    memcpy(sreqBuf, pData->KeySource, 8);
    sreqBuf += 8;
    *sreqBuf = pData->SecurityLevel;
    sreqBuf++;
    *sreqBuf = pData->KeyIdMode;
    sreqBuf++;
    *sreqBuf = pData->KeyIndex;
    sreqBuf++;
    memcpy(sreqBuf, pData->Channels, 17);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_fhEnableReq(void)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_FH_ENABLE_REQ;

    Mt_sendCmd(&cmdDesc);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_fhStartReq(void)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_FH_START_REQ;

    Mt_sendCmd(&cmdDesc);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_fhGetReq(MtMac_fhGetReq_t *pData, MtMac_fhGetReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x02;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_FH_GET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint16(sreqBuf, pData->AttributeID);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->AttrLen = cmdDesc.len - 0x01;
            if(pRspData->Data)
            {
                memcpy(pRspData->Data, srspAttrBuf, pRspData->AttrLen);
            }
            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtMac_fhSetReq(MtMac_fhSetReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 2 + pData->AttrLen;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_MAC;
    cmdDesc.cmd1 = MAC_FH_SET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint16(sreqBuf, pData->AttributeID);
    sreqBuf += 2;
    memcpy(sreqBuf, pData->Data, pData->AttrLen);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}


