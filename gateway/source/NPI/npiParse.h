/******************************************************************************

 @file npiParse.h

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

#ifndef NPI_MT_MT_H_
#define NPI_MT_MT_H_
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

#include <mqueue.h>
// SOF (Start of Frame) indicator byte byte
#define MT_SOF                 (0xFE)

// The 3 MSB's of the 1st command field byte (Cmd0) are for command type
#define MT_CMD_TYPE_MASK       (0xE0)

// The 5 LSB's of the 1st command field byte (Cmd0) are for the subsystem
#define MT_SUBSYSTEM_MASK      (0x1F)

// maximum length of  frame
// (1 byte length + 2 bytes command + 0-250 bytes data)
#define MT_MAX_LEN                (256)

// RPC Frame field lengths
#define MT_SOF_LEN           (1)
#define MT_FCS_LEN           (1)

#define MT_UART_FRAME_START_IDX   (1)

#define MT_LEN_FIELD_LEN          (1)
#define MT_CMD0_FIELD_LEN         (1)
#define MT_CMD1_FIELD_LEN         (1)

#define MT_HDR_LEN                (MT_LEN_FIELD_LEN + MT_CMD0_FIELD_LEN + \
		                            MT_CMD1_FIELD_LEN)

#define MT_UART_HDR_LEN           (MT_SOF_LEN + MT_HDR_LEN)

#define MT_FAIL     0xFF

// Cmd0 Command Type
typedef enum
{
	MT_CMD_POLL = 0x00,  // POLL command
	MT_CMD_SREQ = 0x20,  // SREQ (Synchronous Request) command
	MT_CMD_AREQ = 0x40,  // AREQ (Acynchronous Request) command
	MT_CMD_SRSP = 0x60,  // SRSP (Synchronous Response)
	MT_CMD_EXNT = 0x80,  // EXTN Mask, this mask can be combined with (EXTN | SRSP) || (EXTN | AREQ)
} mtCmdType_t;

// Cmd0 Command Subsystem
// Note: This structure includes subsystems used in Zigbee but reserved in TI-15.4 Stack
typedef enum
{
	MT_ERR,   // Reserved.
	MT_SYS,    // SYS interface
	MT_MAC,
	MT_NWK,
	MT_AF,     // AF interface
	MT_ZDO,    // ZDO interface
	MT_SAPI,   // Simple API interface
	MT_UTIL,   // UTIL interface
	MT_DBG,
	MT_APP,
	MT_OTA,
	MT_ZNP,
	MT_SPARE_12,
	MT_SBL = 13, // 13 to be compatible with existing RemoTI - AKA MT_UBL
	MT_MAX // Maximum value, must be last (so 14-32 available, not yet assigned).
} mtSysType_t;

// Error codes in Attribute byte of SRSP packet
typedef enum
{
	MT_SUCCESS = 0,                 // success
	MT_ERR_SUBSYSTEM = 1,           // invalid subsystem
	MT_ERR_COMMAND_ID = 2,          // invalid command ID
	MT_ERR_PARAMETER = 3,           // invalid parameter
	MT_ERR_LENGTH = 4,              // invalid length
	MT_ERR_UNSUP_HEADER_TYPE = 5,   // Unsupported extended header type
	MT_ERR_MEM_ALLOC_FAIL = 6,      // Memory allocation failure
} mtErrorCode_t;

typedef enum
{
    MT_WAITING_SOF = 0,
	MT_WAITING_HEADER = 1,
	MT_WAITING_DATA = 2,
	MT_CALCULATING_FCS = 3,
} mtRxState_t;



typedef struct
{
    uint8_t len;
    uint8_t cmd0;
    uint8_t cmd1;
    uint8_t *attrs;
} mtMsg_t;


/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void mtRegisterClientMq(mqd_t *mqHandle);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void mtRegisterSrspMq(const char *mtMqName);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void mtRegisterServerMq(mqd_t *mqHandle);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void Mt_parseCmd(uint8_t *inCmd, int32_t cmdLen);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
uint32_t mtProcessInCmd(uint8_t * data, uint32_t len);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void MtMac_ProcessCmd(mtMsg_t *inMtCmd);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void MtSys_ProcessCmd(mtMsg_t *inMtCmd);


/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void Mt_sendCmd(mtMsg_t *cmdDesc);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
uint8_t Mt_rcvSrsp(mtMsg_t *cmdDesc);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif
#endif /* NPI_MT_MT_H_ */
