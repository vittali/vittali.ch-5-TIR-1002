/******************************************************************************

 @file mtSys.c

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
#include "mtSys.h"
/*============== AREQs ==============*/

static void MtSys_processResetInd(mtMsg_t *inMtCmd);

/*============== Process AREQs ==============*/

static MtSys_callbacks_t *mtSysCbs;

void MtSys_RegisterCbs(MtSys_callbacks_t *pCbFuncs)
{
    mtSysCbs = pCbFuncs;
}

void MtSys_ProcessCmd(mtMsg_t *inMtCmd)
{

	if (mtSysCbs && (inMtCmd->cmd0 & MT_CMD_TYPE_MASK) == MT_CMD_AREQ)
	{
		switch (inMtCmd->cmd1)
		{
            case SYS_RESET_IND:
			    MtSys_processResetInd(inMtCmd);
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

void MtSys_processResetInd(mtMsg_t *inMtCmd)
{
    if (mtSysCbs->pResetIndCb)
	{
	    uint8_t *attrPtr = inMtCmd->attrs;
        MtSys_resetInd_t aReqCmd;

        aReqCmd.Reason = *attrPtr;
        attrPtr++;
        aReqCmd.Transport = *attrPtr;
        attrPtr++;
        aReqCmd.Product = *attrPtr;
        attrPtr++;
        aReqCmd.Major = *attrPtr;
        attrPtr++;
        aReqCmd.Minor = *attrPtr;
        attrPtr++;
        aReqCmd.Maint = *attrPtr;

        mtSysCbs->pResetIndCb(&aReqCmd);
	}
}


/*============== AREQs ==============*/

uint8_t MtSys_resetReq(MtSys_resetReq_t *pData)
{
    uint8_t srspStatus = MT_SUCCESS;
    mtMsg_t cmdDesc;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_AREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_RESET_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Type;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);

    return srspStatus;
}

uint8_t MtSys_pingReq(MtSys_pingReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_PING_REQ;


    Mt_sendCmd(&cmdDesc);

    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;
            srspStatus = MT_SUCCESS;
            pRspData->Capabilites = Util_parseUint16(srspAttrBuf);

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtSys_versionReq( MtSys_versionReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_VERSION_REQ;


    Mt_sendCmd(&cmdDesc);

    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            pRspData->Transport = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Product = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Major = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Minor = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Main = *srspAttrBuf;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtSys_nvCreateReq(MtSys_nvCreateReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x09;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_CREATE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);
    sreqBuf += 2;
    Util_bufferUint32(sreqBuf, pData->Length);

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

uint8_t MtSys_nvDeleteReq(MtSys_nvDeleteReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x05;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_DELETE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);

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

uint8_t MtSys_nvLengthReq(MtSys_nvLengthReq_t *pData, MtSys_nvLengthReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x05;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_LENGTH_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = MT_SUCCESS;
            pRspData->Length = Util_parseUint32(srspAttrBuf);

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtSys_nvReadReq(MtSys_nvReadReq_t *pData, MtSys_nvReadReqSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x08;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_READ_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->Offset);
    sreqBuf += 2;
    *sreqBuf = pData->Length;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Length = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Data = srspAttrBuf;
            //*srspAttrBuf = pRspData->Data;//ATTRSIZE: DL
//TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtSys_nvWriteReq(MtSys_nvWriteReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 8+pData->Length;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_WRITE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->Offset);
    sreqBuf += 2;
    *sreqBuf = pData->Length;
    sreqBuf++;
    memcpy(sreqBuf, pData->Data, pData->Length);
//*sreqBuf = pData->Data;//ATTRSIZE: DLTODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY

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

uint8_t MtSys_nvUpdateReq(MtSys_nvUpdateReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 6+pData->Length;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_UPDATE_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SysID;
    sreqBuf++;
    Util_bufferUint16(sreqBuf, pData->ItemID);
    sreqBuf += 2;
    Util_bufferUint16(sreqBuf, pData->SubId);
    sreqBuf += 2;
    *sreqBuf = pData->Length;
    sreqBuf++;
    memcpy(sreqBuf, pData->Data, pData->Length);

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

uint8_t MtSys_nvCompactReq(MtSys_nvCompactReq_t *pData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x02;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_SYS;
    cmdDesc.cmd1 = SYS_NV_COMPACT_REQ;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    Util_bufferUint16(sreqBuf, pData->Threshold);

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


