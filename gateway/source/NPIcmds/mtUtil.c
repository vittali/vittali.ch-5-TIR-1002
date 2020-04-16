/******************************************************************************

 @file mtUtil.c

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
#include "mtUtil.h"

/*============== AREQs ==============*/


/*============== Process AREQs ==============*/

static MtUtil_callbacks_t *mtUtilCbs;

void MtUtil_RegisterCbs(MtUtil_callbacks_t *pCbFuncs)
{
    mtUtilCbs = pCbFuncs;
}

void MtUtil_ProcessCmd(mtMsg_t *inMtCmd)
{

	if (mtUtilCbs && (inMtCmd->cmd0 & MT_CMD_TYPE_MASK) == MT_CMD_AREQ)
	{
		switch (inMtCmd->cmd1)
		{

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


/*============== SREQs ==============*/

uint8_t MtUtil_callbackSubCmd(MtUtil_callbackSubCmd_t *pData, MtUtil_callbackSubCmdSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x05;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_UTIL;
    cmdDesc.cmd1 = UTIL_CALLBACK_SUB_CMD;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->SubsystemId;
    sreqBuf++;
    Util_bufferUint32(sreqBuf, pData->Enables);

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;

            srspStatus = *srspAttrBuf;
            srspAttrBuf++;
            pRspData->Enables = Util_parseUint32(srspAttrBuf);

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

uint8_t MtUtil_utilGetExtAddr(MtUtil_utilGetExtAddr_t *pData, MtUtil_utilGetExtAddrSrsp_t *pRspData)
{
    uint8_t srspStatus = MT_FAIL;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x01;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_UTIL;
    cmdDesc.cmd1 = MT_UTIL_GET_EXT_ADDR;

    uint8_t *sreqBuf;
    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
    sreqBuf = cmdDesc.attrs;

    *sreqBuf = pData->Type;

    Mt_sendCmd(&cmdDesc);
    free(cmdDesc.attrs);
    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;


            pRspData->Type = *srspAttrBuf;
            srspAttrBuf++;
            memcpy(pRspData->ExtAddress, srspAttrBuf, 8);
            srspStatus = MT_SUCCESS;

            free(cmdDesc.attrs);
        }
    }

    return srspStatus;
}

//uint8_t MtUtil_utilLoopback(MtUtil_utilLoopback_t *pData, MtUtil_utilLoopbackSrsp_t *pRspData)
//{
//    uint8_t srspStatus = MT_FAIL;
//    mtMsg_t cmdDesc;
//    uint8_t *srspAttrBuf;
//
//    cmdDesc.len = //[ERROR] MODIFY MANUALLY BAD LENGTH: 0x05+DL;
//    cmdDesc.cmd0 = MT_CMD_SREQ | MT_UTIL;
//    cmdDesc.cmd1 = MT_UTIL_LOOPBACK;
//
//    uint8_t *sreqBuf;
//    cmdDesc.attrs = (uint8_t*)malloc(cmdDesc.len);
//    sreqBuf = cmdDesc.attrs;
//
//    *sreqBuf = pData->Repeats;
//    sreqBuf++;
//    Util_bufferUint32(sreqBuf, pData->Interval);
//    sreqBuf += 4;
//
////*sreqBuf = pData->Data;//ATTRSIZE: DL
////TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
//
//    Mt_sendCmd(&cmdDesc);
//    free(cmdDesc.attrs);
//    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
//    {
//        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
//        {
//            srspAttrBuf = cmdDesc.attrs;
//
//            srspStatus = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->Repeats = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->Interval = Util_parseUint32(srspAttrBuf);
//            srspAttrBuf += 4;
//            pRspData->Data = srspAttrBuf;
//            srspAttrBuf += pRspData->DL;
//            //*srspAttrBuf = pRspData->Data;//ATTRSIZE: DL
////TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
//            pRspData->AREQ: = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->Length = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->= = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->0x05+DL = *srspAttrBuf;
//            srspAttrBuf++;
//            pRspData->Cmd0 = Util_parseUint32(srspAttrBuf);
//            srspAttrBuf += 4;
//            pRspData->= = srspAttrBuf;
//            //*srspAttrBuf = pRspData->=;//ATTRSIZE: DL
////TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
//
//            free(cmdDesc.attrs);
//        }
//    }
//
//    return srspStatus;
//}

uint8_t MtUtil_utilRandom(void)
{
    uint16_t numberRet = 0;
    mtMsg_t cmdDesc;
    uint8_t *srspAttrBuf;

    cmdDesc.len = 0x00;
    cmdDesc.cmd0 = MT_CMD_SREQ | MT_UTIL;
    cmdDesc.cmd1 = MT_UTIL_RANDOM;


    Mt_sendCmd(&cmdDesc);

    if(Mt_rcvSrsp(&cmdDesc) == MT_SUCCESS)
    {
        if(cmdDesc.len > 0 && cmdDesc.attrs != NULL)
        {
            srspAttrBuf = cmdDesc.attrs;
            numberRet = Util_parseUint16(srspAttrBuf);

            free(cmdDesc.attrs);
        }
    }

    return (uint8_t)numberRet;
}


