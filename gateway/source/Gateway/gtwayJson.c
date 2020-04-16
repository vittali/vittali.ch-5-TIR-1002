/******************************************************************************

 @file gtwayJson.c

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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Common/commonDefs.h>
#include <Utils/util.h>

#define DEV_LIST_CHAR_LEN   285
#define NWK_UPDT_CHAR_LEN   270
#define DEV_OBJ_CHAR_LEN    100
#define DEV_UPDT_CHAR_LEN   130
#define TIMESTAMP_CHAR_LEN  18 + 26 // "last_reported": 18 chars, + 26 of actual date and time info

//*****************************************************************************
//                 Constant VARIABLES
//*****************************************************************************
const char *secStrs[2] = {"0", "1"};
const char *modeStrs[2] = {"beacon", "frequency hopping"};
const char *stateStrs[7] = {"waiting", "starting", "restoring", "started", "restored", "open", "close"};
const char *activeStrs[2] = {"false", "true"};

const char jsonDevList[] = "{\"name\":\"%s\",\"active\":\"true\",\"rssi\":-30,\"short_addr\":\"0x%04X\",\"ext_addr\":\"0x%x%08x\"}";
//#if defined(USE_IBM_CLOUD)
//const char *jsonNwkUpdateCmd ="{\"d\":{\"name\":\"%s\",\"channels\":\"%d\",\"pan_id\":\"0x%04X\",\"short_addr\":\"0x%04X\",\"ext_addr\":\"0x%08X%08X\",\"security_enabled\":\"%s\",\"mode\":\"%s\",\"state\":\"%s\",\"devices\":[%s]}}";
//const char *jsonDevUpdateCmd ="{\"d\":{\"active\":\"%s\",\"ext_addr\":\"0x%08X%08X\",\"rssi\":\"%d\",\"smart_objects\":{%s}}}";
//#elif defined(USE_AWS_CLOUD)
const char *jsonNwkUpdateCmd ="{\"name\":\"%s\",\"channels\":\"%d\",\"pan_id\":\"0x%04X\",\"short_addr\":\"0x%04X\",\"ext_addr\":\"0x%x%08x\",\"security_enabled\":%s,\"mode\":\"%s\",\"state\":\"%s\",\"devices\":[%s]}";
const char *jsonDevUpdateCmd ="{\"active\":\"%s\",\"short_addr\":\"0x%04X\",\"ext_addr\":\"0x%x%08x\",\"rssi\":\"%d\",\"smart_objects\":{%s}}";
//#endif
const char *jsonDevObjects = "\"%s\":{\"%d\":{\"oid\":\"%s\",\"iid\":\"0\",\"sensorValue\":%d,\"units\":\"%s\"}}";
const char *jsonTimeStamp = "\"%s\":\"%.24s\"";

char* formatNwkJson(nwk_t *nwkInfo, dev_t *devList)
{
    int devListStrLen = (nwkInfo->devCount*DEV_LIST_CHAR_LEN * sizeof(char)) + 1;
    int nwkUpdtStrLen = devListStrLen + (NWK_UPDT_CHAR_LEN * sizeof(char));
    uint32_t loBytes, hiBytes, nwkLoBytes, nwkHiBytes;
    uint8_t *tempExtAddr;
    char *nwkString;
    char *devListString;
    char *tempPtr;
    nwkString = (char*) malloc(nwkUpdtStrLen);
    devListString = (char*) malloc(devListStrLen);
    devListString[0] = '\0';
    tempPtr = devListString;

    tempExtAddr = nwkInfo->extAddr;
    nwkLoBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr++);
    nwkHiBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr);
    for(int devIdx = 0; devIdx < nwkInfo->devCount; devIdx++)
    {
        tempExtAddr = devList[devIdx].extAddr;
        loBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr++);
        hiBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr);
        sprintf(tempPtr, jsonDevList, devList[devIdx].name, devList[devIdx].shortAddr, hiBytes, loBytes);
        if(devIdx < (nwkInfo->devCount - 1))
        {
            strcat(tempPtr, ",");
        }
        tempPtr += strlen(tempPtr);
    }


    sprintf(nwkString, jsonNwkUpdateCmd, nwkInfo->name, nwkInfo->channel, nwkInfo->panId,
            nwkInfo->shortAddr, nwkHiBytes, nwkLoBytes,
            secStrs[(int)nwkInfo->security_enable], modeStrs[(int)nwkInfo->mode], stateStrs[(int)nwkInfo->state], devListString);

    free(devListString);
    return nwkString;
}

char* formatDevJson(dev_t *device, char *timeStamp)
{
    int objListStrLen = TIMESTAMP_CHAR_LEN + (device->objectCount*DEV_OBJ_CHAR_LEN * sizeof(char)) + 1;
    int devUpdtStrLen = objListStrLen + (DEV_UPDT_CHAR_LEN * sizeof(char));
    char *devString;
    char *objListString;
    char *tempPtrStr;
    uint32_t loBytes, hiBytes;
    uint8_t *tempExtAddr;
    devString = (char*) malloc(devUpdtStrLen);
    objListString = (char*) malloc(objListStrLen);
    tempPtrStr = objListString;
    objListString[0] = '\0';
    for(int objIdx = 0; objIdx < device->objectCount; objIdx++)
    {
        //sprintf(tempPtrStr, jsonDevObjects, device->object[objIdx].type, objIdx, device->object[objIdx].sensorVal);
        sprintf(tempPtrStr, jsonDevObjects, device->object[objIdx].type, 0, device->object[objIdx].type, device->object[objIdx].sensorVal, device->object[objIdx].unit);
        //if(objIdx < (device->objectCount - 1))
        {
            strcat(tempPtrStr, ",");
        }
        tempPtrStr += strlen(tempPtrStr);
    }
    sprintf(tempPtrStr, jsonTimeStamp, TIME_STAMP, timeStamp);
    tempExtAddr = device->extAddr;
    loBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr++);
    hiBytes = Util_buildUint32(*tempExtAddr++, *tempExtAddr++,*tempExtAddr++,*tempExtAddr);
    sprintf(devString, jsonDevUpdateCmd, activeStrs[(int)device->active], device->shortAddr, hiBytes, loBytes, device->rssi, objListString);
    free(objListString);

    return devString;
}



