/******************************************************************************

 @file  nvoctp.c

 @brief NV definitions for CC26xx devices - On-Chip Two-Page Flash Memory

 Group: WCS, LPC, BTS
 Target Device: CC13xx

 ******************************************************************************

 Copyright (c) 2014-2016, Texas Instruments Incorporated
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
 Release Date: 2016-11-21 18:05:36
 *****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>  /* for string support */
#include <stdlib.h>
#include <stdio.h>   /* for snprintf() */

#include "nvoctp.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <ti/drivers/net/wifi/simplelink.h>

//*****************************************************************************
// Constants and Definitions
//*****************************************************************************

#define PATHFORMAT "nvs_%llx"
#define NV_MAXFILESIZE 0x0100

//*****************************************************************************
// Macros
//*****************************************************************************


//*****************************************************************************
// Global Variables
//*****************************************************************************

//*****************************************************************************
// Global Functions
//*****************************************************************************


//*****************************************************************************
// Local Variables
//*****************************************************************************

static Semaphore_Struct  writeSem;
static bool isInitialized = false;

//*****************************************************************************
// Local Functions
//*****************************************************************************
static uint64_t NVOCTP_createFileNameFromNVIDs(NVINTF_itemID_t id);
static int32_t NVOCTP_initNvApi(void *param);
static int32_t NVOCTP_createItemApi(NVINTF_itemID_t id, uint32_t bLen, void *pBuf);
static int32_t NVOCTP_deleteItemApi(NVINTF_itemID_t id);
static int32_t NVOCTP_readItemApi(NVINTF_itemID_t id, uint16_t bOfs, uint16_t bLen, void *pBuf);
static int32_t NVOCTP_writeItemApi(NVINTF_itemID_t id, uint16_t bLen, void *pBuf);


//*****************************************************************************
// API Functions - NV driver
//*****************************************************************************

/**
 * @fn      NVOCTP_loadApiPtrs
 *
 * @brief   Global function to return function pointers for NV driver API that
 *          are supported by this module, NULL for functions not supported.
 *
 * @param   pfn - pointer to caller's structure of NV function pointers
 *
 * @return  none
 */

static uint64_t NVOCTP_createFileNameFromNVIDs(NVINTF_itemID_t id)
{
    uint64_t filename = 0;

    filename = ((uint64_t)id.systemID << 32) | ((uint64_t)id.itemID << 16)
                | ((uint64_t)id.itemID);

    return filename;
}

void NVOCTP_loadApiPtrs(NVINTF_nvFuncts_t *pfn)
{
    // Load caller's structure with pointers to the NV API functions
    pfn->initNV      = &NVOCTP_initNvApi;
    pfn->compactNV   = NULL;
    pfn->createItem  = &NVOCTP_createItemApi;
    pfn->deleteItem  = &NVOCTP_deleteItemApi;
    pfn->readItem    = &NVOCTP_readItemApi;
    pfn->writeItem   = &NVOCTP_writeItemApi;
    pfn->writeItemEx = NULL;
    pfn->getItemLen  = NULL;
}

/******************************************************************************
 * @fn      NVOCTP_initNvApi
 *
 * @brief   API function to initialize the specified NV Flash pages
 *
 * @param   param - pointer to caller's structure of NV init parameters
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static int32_t NVOCTP_initNvApi(void *param)
{
    (void)param;
    if (!isInitialized) {
        Semaphore_construct(&writeSem, 1, NULL);

        // initialize file system? not sure if we have to do that

        isInitialized = true;
    }

    return NVINTF_SUCCESS;
}

/******************************************************************************
 * @fn      NVOCTP_createItemApi
 *
 * @brief   API function to create a new NV item in Flash memory
 *
 * @param   id - NV item type identifier
 * @param   len - length of NV data block
 * @param   buf - pointer to caller's initialization data buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static int32_t NVOCTP_createItemApi(NVINTF_itemID_t id,
                                    uint32_t bLen,
                                    void *pBuf)
{
    // writeItemApi does create if file does not exist
    return NVOCTP_writeItemApi(id, bLen, pBuf);
}

/******************************************************************************
 * @fn      NVOCTP_deleteItemApi
 *
 * @brief   API function to delete an existing NV item from Flash memory
 *
 * @param   id - NV item type identifier
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static int32_t NVOCTP_deleteItemApi(NVINTF_itemID_t id)
{
    uint64_t filename = NVOCTP_createFileNameFromNVIDs(id);
    _i32 status;
    char filenameBuf[32] = {0};
    long fsHandle;
    unsigned long token = 0;
    int32_t retval = NVINTF_FAILURE;

    snprintf(filenameBuf, sizeof(filenameBuf), PATHFORMAT, filename);

    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    fsHandle = sl_FsOpen((unsigned char *)filenameBuf, SL_FS_READ,
                           &token);

    do
    {
        if (fsHandle > 0)
        {
            // file exists, close and delete it
            status = sl_FsClose(fsHandle, NULL, 0, 0);
            if (status == 0) {
                status = sl_FsDel((unsigned char *)filenameBuf, 0);
            }
            else
            {
                // close fail
                retval = status;
                break;
            }
        }
        else
        {
            // file does not exist
            retval = fsHandle;
            break;
        }

        if (status == 0)
        {
            retval = NVINTF_SUCCESS;
        }
        else
        {
            // delete fail
            retval = status;
        }

    } while(0);

    Semaphore_post(Semaphore_handle(&writeSem));
    return retval;
}

/******************************************************************************
 * @fn      NVOCTP_readItemApi
 *
 * @brief   API function to read data from an NV item
 *
 * @param   id   - NV item type identifier
 * @param   bOfs - offset into NV data block
 * @param   bLen - length of NV data to return
 * @param   pBuf - pointer to caller's read data buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static int32_t NVOCTP_readItemApi(NVINTF_itemID_t id,
                                  uint16_t bOfs,
                                  uint16_t bLen,
                                  void *pBuf)
{
    uint64_t filename = NVOCTP_createFileNameFromNVIDs(id);
    char filenameBuf[32] = {0};
    long fsHandle;
    long bytesRead;
    unsigned long token;
    _i32 status;
    uint8_t retval = NVINTF_FAILURE;

    snprintf(filenameBuf, sizeof(filenameBuf), PATHFORMAT, filename);

    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    fsHandle = sl_FsOpen((unsigned char *)filenameBuf, SL_FS_READ,
                               &token);

    if(fsHandle > 0)
    {
        bytesRead = sl_FsRead(fsHandle, bOfs, pBuf, bLen);

        if(bytesRead > 0)
        {
            retval = NVINTF_SUCCESS;
        }
        else
        {
            // read fail
            retval = bytesRead;
        }

        status = sl_FsClose(fsHandle, NULL, 0, 0);
        if(status < 0)
        {
            // close fail
            retval = status;
        }
    }
    else
    {
        // file does not exist
        retval = fsHandle;
    }

    Semaphore_post(Semaphore_handle(&writeSem));
    return retval;
}

/******************************************************************************
 * @fn      NVOCTP_writeItemApi
 *
 * @brief   API function to write data NV item, create if not already existing
 *
 * @param   id   - NV item type identifier
 * @param   bLen - data buffer length to write into NV block
 * @param   pBuf - pointer to caller's data buffer to write
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static int32_t NVOCTP_writeItemApi(NVINTF_itemID_t id,
                                   uint16_t bLen,
                                   void *pBuf)
{
    uint64_t filename = NVOCTP_createFileNameFromNVIDs(id);
    char filenameBuf[32] = {0};
    long fsHandle;
    long bytesRead;
    long bytesWritten;
    unsigned long token;
    int32_t retval = NVINTF_FAILURE;
    _i32 status;

    uint8_t *srcBuf = (uint8_t *)malloc(bLen + 1);
    memset(srcBuf, 0, bLen + 1);

    snprintf(filenameBuf, sizeof(filenameBuf), PATHFORMAT, filename);

    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    // do while(0) so I can prematurely exit, perform cleanup, and return with error code
    do
    {

        fsHandle = sl_FsOpen((unsigned char *)filenameBuf, SL_FS_READ,
                                   &token);

        if(fsHandle < 0) // negative error code on failures, defined in errors.h
        {
            fsHandle = sl_FsOpen((unsigned char *)filenameBuf,
                                 SL_FS_CREATE|SL_FS_OVERWRITE|SL_FS_CREATE_MAX_SIZE(bLen),
                                  &token);

            if (fsHandle < 0)
            {
                // file creation failed for some reason
                retval = fsHandle;
                break;
            }
        }
        else
        {
            /* copy file contents into copyBlock for modifying */
           bytesRead = sl_FsRead(fsHandle, 0, srcBuf,
                                 bLen);

           if(bytesRead < 0)
           {
               // read fail
               status = sl_FsClose(fsHandle, NULL, 0, 0);
               if(status < 0)
               {
                   // sl_FsClose failed
                   retval = status;
               }
               else
               {
                   // bytesRead will be an error code from sl_FsRead in this case
                   retval = bytesRead;
               }
               break;
           }

           /* Close the file since we will need to open it again for writing */
           status = sl_FsClose(fsHandle, NULL, 0, 0);
           if(status < 0)
           {
               // sl_FsClose failed
               retval = status;
               break;
           }


           if (bytesRead == bLen)
           {
               fsHandle = sl_FsOpen((unsigned char *)filenameBuf, SL_FS_OVERWRITE,
                                          &token);
           }
           else
           {
               // sl_FsRead failed
               retval = bytesRead;
               break;
           }

           if (fsHandle < 0)
           {
               // sl_FsOpen failed
               retval = fsHandle;
               break;
           }
        }

       memcpy((void *)(srcBuf + 0/*offset*/), pBuf, bLen);

       bytesWritten = sl_FsWrite(fsHandle, 0, srcBuf, bLen);

       if (bytesWritten != bLen)
       {
           // sl_FsWrite failed
           status = sl_FsClose(fsHandle, NULL, 0, 0);
           if(status < 0)
           {
               // sl_FsClose failed
               retval = status;
           }
           else
           {
               // error from sl_FsWrite above
               retval = bytesWritten;
           }
           break;
       }
       else
       {
           sl_FsClose(fsHandle, NULL, 0, 0);
           retval = NVINTF_SUCCESS;
       }

   } while(0);

    free(srcBuf);
    Semaphore_post(Semaphore_handle(&writeSem));
    return retval;
}

