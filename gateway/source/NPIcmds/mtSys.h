/******************************************************************************

 @file mtSys.h

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

#define SYS_RESET_IND    0x80

/*============== AREQ Typedefs ==============*/

typedef struct
{
    uint8_t Reason;
    uint8_t Transport;
    uint8_t Product;
    uint8_t Major;
    uint8_t Minor;
    uint8_t Maint;
}MtSys_resetInd_t;


typedef void (*MtSys_resetIndFp_t)(MtSys_resetInd_t *pResetInd);

typedef struct
{
    MtSys_resetIndFp_t pResetIndCb;

}MtSys_callbacks_t;
void MtSys_RegisterCbs(MtSys_callbacks_t *pCbFuncs);

/*============== SREQ CMD1 DEFINES ==============*/

#define SYS_RESET_REQ    0x00
#define SYS_PING_REQ    0x01
#define SYS_VERSION_REQ    0x02
#define SYS_NV_CREATE_REQ    0x30
#define SYS_NV_DELETE_REQ    0x31
#define SYS_NV_LENGTH_REQ    0x32
#define SYS_NV_READ_REQ    0x33
#define SYS_NV_WRITE_REQ    0x34
#define SYS_NV_UPDATE_REQ    0x35
#define SYS_NV_COMPACT_REQ    0x36

/*============== SREQ Typedefs ==============*/

typedef struct
{
    uint8_t Type;
}MtSys_resetReq_t;

typedef struct
{
    uint16_t Capabilites;
}MtSys_pingReqSrsp_t;
typedef struct
{
    uint8_t Transport;
    uint8_t Product;
    uint8_t Major;
    uint8_t Minor;
    uint8_t Main;
}MtSys_versionReqSrsp_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
    uint32_t Length;
}MtSys_nvCreateReq_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
}MtSys_nvDeleteReq_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
}MtSys_nvLengthReq_t;

typedef struct
{
    uint32_t Length;
}MtSys_nvLengthReqSrsp_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
    uint16_t Offset;
    uint8_t Length;
}MtSys_nvReadReq_t;
typedef struct
{
    uint8_t Length;
    uint8_t *Data;//ATTRSIZE: DL
//TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
}MtSys_nvReadReqSrsp_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
    uint16_t Offset;
    uint8_t Length;
    uint8_t *Data;//ATTRSIZE: DL
//TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
}MtSys_nvWriteReq_t;

typedef struct
{
    uint8_t SysID;
    uint16_t ItemID;
    uint16_t SubId;
    uint8_t Length;
    uint8_t *Data;//ATTRSIZE: DL
//TODO: couldnt parse attr length check CoP guide PARSE COMMAND MANUALLY
}MtSys_nvUpdateReq_t;

typedef struct
{
    uint16_t Threshold;
}MtSys_nvCompactReq_t;


/*============== SREQ Declarations ==============*/

uint8_t MtSys_resetReq(MtSys_resetReq_t *pData);
uint8_t MtSys_pingReq(MtSys_pingReqSrsp_t *pRspData);
uint8_t MtSys_versionReq( MtSys_versionReqSrsp_t *pRspData);
uint8_t MtSys_nvCreateReq(MtSys_nvCreateReq_t *pData);
uint8_t MtSys_nvDeleteReq(MtSys_nvDeleteReq_t *pData);
uint8_t MtSys_nvLengthReq(MtSys_nvLengthReq_t *pData, MtSys_nvLengthReqSrsp_t *pRspData);
uint8_t MtSys_nvReadReq(MtSys_nvReadReq_t *pData, MtSys_nvReadReqSrsp_t *pRspData);
uint8_t MtSys_nvWriteReq(MtSys_nvWriteReq_t *pData);
uint8_t MtSys_nvUpdateReq(MtSys_nvUpdateReq_t *pData);
uint8_t MtSys_nvCompactReq(MtSys_nvCompactReq_t *pData);

