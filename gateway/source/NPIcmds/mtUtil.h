/******************************************************************************

 @file mtUtil.h

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


/*============== AREQ Typedefs ==============*/



typedef struct
{

}MtUtil_callbacks_t;
void MtUtil_RegisterCbs(MtUtil_callbacks_t *pCbFuncs);
void MtUtil_ProcessCmd(mtMsg_t *inMtCmd);

/*============== SREQ CMD1 DEFINES ==============*/

#define UTIL_CALLBACK_SUB_CMD    0x06
#define MT_UTIL_GET_EXT_ADDR    0xEE
#define MT_UTIL_LOOPBACK    0x10
#define MT_UTIL_RANDOM    0x12

/*============== SREQ Typedefs ==============*/

typedef struct
{
    uint8_t SubsystemId;
    uint32_t Enables;
}MtUtil_callbackSubCmd_t;
typedef struct
{
    uint32_t Enables;
}MtUtil_callbackSubCmdSrsp_t;

typedef struct
{
    uint8_t Type;
}MtUtil_utilGetExtAddr_t;
typedef struct
{
    uint8_t Type;
    uint8_t ExtAddress[8];
}MtUtil_utilGetExtAddrSrsp_t;

typedef struct
{
    uint8_t Repeats;
    uint32_t Interval;
    uint8_t *Data;//ATTRSIZE: DL
//TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
}MtUtil_utilLoopback_t;
//typedef struct
//{
//    uint32_t Interval;
//    uint8_t *Data;//ATTRSIZE: DL
////TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
//    uint8_t AREQ:;
//    uint8_t Length;
//    uint8_t =;
//    uint8_t 0x05+DL;
//    uint32_t Cmd0;
//    uint8_t *=;//ATTRSIZE: DL
////TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
//}MtUtil_utilLoopbackSrsp_t;



/*============== SREQ Declarations ==============*/

uint8_t MtUtil_callbackSubCmd(MtUtil_callbackSubCmd_t *pData, MtUtil_callbackSubCmdSrsp_t *pRspData);
uint8_t MtUtil_utilGetExtAddr(MtUtil_utilGetExtAddr_t *pData, MtUtil_utilGetExtAddrSrsp_t *pRspData);
//uint8_t MtUtil_utilLoopback(MtUtil_utilLoopback_t *pData, MtUtil_utilLoopbackSrsp_t *pRspData);
uint8_t MtUtil_utilRandom(void);

