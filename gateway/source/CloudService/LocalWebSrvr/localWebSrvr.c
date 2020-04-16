/******************************************************************************

 @file localWebSrvr.c

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

/******************************************************************************
 Includes
 *****************************************************************************/

/* Standard Include */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <mqueue.h>

/* TI-DRIVERS Header files */
#include <ti/drivers/net/wifi/simplelink.h>
#include <Common/commonDefs.h>
#include <Utils/uart_term.h>
#include <CloudService/cloudJson.h>
#include <CloudService/IBM/cloudServiceIBM.h>
#include "localWebSrvr.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define xHTTP_HANDLR_DEBUG


#define IBM_ORGID_MSK      0x01
#define IBM_DEVTYPE_MSK    0x02
#define IBM_DEVID_MSK      0x04
#define IBM_SECTOKEN_MSK   0x08
#define IBM_FULL_MSK       (IBM_ORGID_MSK | IBM_DEVTYPE_MSK | IBM_DEVID_MSK | IBM_SECTOKEN_MSK)


#define NETAPP_MAX_RX_FRAGMENT_LEN      SL_NETAPP_REQUEST_MAX_DATA_LEN
#define NETAPP_MAX_METADATA_LEN         (100)
#define NETAPP_MAX_ARGV_TO_CALLBACK SL_FS_MAX_FILE_NAME_LENGTH+50
#define NUMBER_OF_URI_SERVICES          (6)


const uint8_t pgNotFound[] = "<html>404 - Sorry page not found</html>";

const char *incomingDevCmdsLocal[] =
{
 "updateFanSpeed",
 "sendToggle",
 "updateDoorLock",
 "leakBuzzOff"
};
const CmdTypes incomingDevCmdTypesLocal[] =
{
 CmdType_FAN_DATA,
 CmdType_LED_DATA,
 CmdType_DOORLOCK_DATA,
 CmdType_LEAK_DATA
};

/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/
int32_t devsGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
int32_t nwkGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
int32_t cmdPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
int32_t actionPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
int32_t cloudGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
int32_t cloudPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest);
void NetAppRequestErrorResponse(SlNetAppResponse_t *pNetAppResponse);
void httpGetHandler(SlNetAppRequest_t *netAppRequest);
void httpPostHandler(SlNetAppRequest_t *netAppRequest);
int32_t httpCheckContentInDB(SlNetAppRequest_t *netAppRequest, uint8_t *requestIdx, uint8_t *argcCallback, uint8_t **argvCallback);
int32_t parseHttpRequestPayload(uint8_t requestIdx, uint8_t * pPayload, uint16_t payloadLen, uint8_t *argcCallback, uint8_t **argvCallback);
int32_t parseHttpRequestMetadata(uint8_t requestType, uint8_t * pMetadata, uint16_t metadataLen, uint8_t *requestIdx, uint8_t *argcCallback, uint8_t **argvCallback);
void convertHeaderType2Text (uint8_t httpHeaderType, uint8_t **httpHeaderText);
int32_t parseUrlEncoded(uint8_t requestIdx, uint8_t * pPhrase, uint16_t phraseLen, uint8_t *argcCallback, uint8_t **argvCallback);
uint16_t setElementType(uint8_t isValue, uint8_t requestIdx, uint8_t elementVal);
uint16_t prepareGetMetadata(int32_t parsingStatus, uint32_t contentLen, HttpContentTypeList contentTypeId);
uint16_t preparePostMetadata(int32_t parsingStatus);

/******************************************************************************
 Structures
 *****************************************************************************/


http_RequestObj_t    httpRequest[NUMBER_OF_URI_SERVICES] =
{
        {0, SL_NETAPP_REQUEST_HTTP_GET, "/devices", {{"devs"}}, devsGetCallback},
        {1, SL_NETAPP_REQUEST_HTTP_GET, "/nwk", {{"nwk"}}, nwkGetCallback},
        {2, SL_NETAPP_REQUEST_HTTP_POST, "/cmd", {{"cmd"}}, cmdPostCallback},
        {3, SL_NETAPP_REQUEST_HTTP_POST, "/action", {{"action"}}, actionPostCallback},
        {4, SL_NETAPP_REQUEST_HTTP_GET, "/cloud", {{"ipAddress"},
                                                   {"wifiConnection", {"false","true"}},
                                                   {"mqttConnection", {"false","true"}}},
                                                   cloudGetCallback},
        {5, SL_NETAPP_REQUEST_HTTP_POST, "/cloud", {{"cloud", {"IBM", "AWS"}},
                                                    {"org"},
                                                    {"type"},
                                                    {"id"},
                                                    {"password"}}, cloudPostCallback},
};
http_headerFieldType_t g_HeaderFields [] =
{
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_VERSION, WEB_SERVER_VERSION},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REQUEST_URI, WEB_SERVER_REQUEST_URI},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_QUERY_STRING, WEB_SERVER_QUERY_STRING},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_TYPE, WEB_SERVER_HEADER_CONTENT_TYPE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_LEN, WEB_SERVER_HEADER_CONTENT_LEN},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_LOCATION, WEB_SERVER_HEADER_LOCATION},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_SERVER, WEB_SERVER_HEADER_SERVER},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_USER_AGENT, WEB_SERVER_HEADER_USER_AGENT},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_COOKIE, WEB_SERVER_HEADER_COOKIE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_SET_COOKIE, WEB_SERVER_HEADER_SET_COOKIE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_UPGRADE, WEB_SERVER_HEADER_UPGRADE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REFERER, WEB_SERVER_HEADER_REFERER},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_ACCEPT, WEB_SERVER_HEADER_ACCEPT},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_ENCODING, WEB_SERVER_HEADER_CONTENT_ENCODING},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_DISPOSITION, WEB_SERVER_HEADER_CONTENT_DISPOSITION},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONNECTION, WEB_SERVER_HEADER_CONNECTION},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_ETAG, WEB_SERVER_HEADER_ETAG},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_DATE, WEB_SERVER_HEADER_DATE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_HEADER_HOST, WEB_SERVER_HEADER_HOST},
    {SL_NETAPP_REQUEST_METADATA_TYPE_ACCEPT_ENCODING, WEB_SERVER_HEADER_ACCEPT_ENCODING},
    {SL_NETAPP_REQUEST_METADATA_TYPE_ACCEPT_LANGUAGE, WEB_SERVER_HEADER_ACCEPT_LANGUAGE},
    {SL_NETAPP_REQUEST_METADATA_TYPE_CONTENT_LANGUAGE, WEB_SERVER_HEADER_CONTENT_LANGUAGE}
};

http_contentTypeMapping_t g_ContentTypes [] =
{
    {HttpContentTypeList_TextHtml, TEXT_HTML, TEXT_HTML_MIME},
    {HttpContentTypeList_TextCSS, TEXT_CSS, TEXT_CSS_MIME},
    {HttpContentTypeList_TextXML, TEXT_XML, TEXT_XML_MIME},
    {HttpContentTypeList_ApplicationJson, APPLICATION_JSON, APPLICATION_JSON_MIME},
    {HttpContentTypeList_ImagePNG, IMAGE_PNG, IMAGE_PNG_MIME},
    {HttpContentTypeList_ImageGIF, IMAGE_GIF, IMAGE_GIF_MIME},
    {HttpContentTypeList_TextPlain, TEXT_PLAIN, TEXT_PLAIN_MIME},
    {HttpContentTypeList_TextCSV, TEXT_CSV, TEXT_CSV_MIME},
    {HttpContentTypeList_ApplicationJavascript, APPLICATION_JAVASCRIPT, APPLICATION_JAVASCRIPT_MIME},
    {HttpContentTypeList_ImageJPEG, IMAGE_JPEG, IMAGE_JPEG_MIME},
    {HttpContentTypeList_ApplicationPDF, APPLICATION_PDF, APPLICATION_PDF_MIME},
    {HttpContentTypeList_ApplicationZIP, APPLICATION_ZIP, APPLICATION_ZIP_MIME},
    {HttpContentTypeList_ShokewaveFlash, SHOCKWAVE_FLASH, SHOCKWAVE_FLASH_MIME},
    {HttpContentTypeList_AudioXAAC, AUDIO_X_AAC, AUDIO_X_AAC_MIME},
    {HttpContentTypeList_ImageXIcon, IMAGE_X_ICON, IMAGE_X_ICON_MIME},
    {HttpContentTypeList_TextVcard, TEXT_VCARD, TEXT_VCARD_MIME},
    {HttpContentTypeList_ApplicationOctecStream, APPLICATION_OCTEC_STREAM, APPLICATION_OCTEC_STREAM_MIME},
    {HttpContentTypeList_VideoAVI, VIDEO_AVI, VIDEO_AVI_MIME},
    {HttpContentTypeList_VideoMPEG, VIDEO_MPEG, VIDEO_MPEG_MIME},
    {HttpContentTypeList_VideoMP4, VIDEO_MP4, VIDEO_MP4_MIME},
    {HttpContentTypeList_UrlEncoded, FORM_URLENCODED, URL_ENCODED_MIME}
};

/******************************************************************************
 Local variables
 *****************************************************************************/
static uint8_t cloudInfoBitMask = 0;
static CloudIBM_Info_t ibmCloudInfo;

/******************************************************************************
 Global variables
 *****************************************************************************/

bool localWlanConnected = false;
bool cloudConnected = false;

mqd_t cloudMq;
mqd_t queDevUpdtMsgs;
mqd_t queNwkUpdtMsgs;
mqd_t *gatewayCliMq;


char *lastNwkUpdt;
char *lastDevUpdt;

uint8_t     gMetadataBuffer[NETAPP_MAX_METADATA_LEN];
uint8_t     gPayloadBuffer[NETAPP_MAX_RX_FRAGMENT_LEN];

uint8_t     gHttpPostBuffer[NETAPP_MAX_ARGV_TO_CALLBACK];
uint8_t     gHttpGetBuffer[NETAPP_MAX_ARGV_TO_CALLBACK];

/******************************************************************************
 Public Functions
 ****************************************************************************/
/*!
 Initialize local webserver

 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_init(const char *cloudSrvrMqName)
{
    mq_attr attr;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 20;
    attr.mq_msgsize = sizeof(msgQueue_t);
    cloudMq = mq_open(cloudSrvrMqName, O_WRONLY);// | O_NONBLOCK);
    queDevUpdtMsgs = mq_open("localWebDevQmsgs", O_CREAT | O_NONBLOCK, 0, &attr);
    attr.mq_maxmsg = 10;
    queNwkUpdtMsgs = mq_open("localWebNwkQmsgs", O_CREAT | O_NONBLOCK, 0, &attr);
}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_registerCliMq(mqd_t *pGatewayMq)
{
    gatewayCliMq = pGatewayMq;
}


/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleWlanDisconnect()
{
    localWlanConnected = false;
}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleWlanConnect()
{
    localWlanConnected = true;
}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleCloudDisconnect()
{
    cloudConnected = false;
}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleCloudConnect()
{
    cloudConnected = true;
}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleGatewayEvt(msgQueue_t *inEvtMsg)
{
    //queUpdtMsgs
//    msgQueue_t msgToQue;
//    msgToQue.event = inEvtMsg->event;
//    msgToQue.msgPtr = malloc(inEvtMsg->msgPtrLen);
//    msgToQue.msgPtrLen = inEvtMsg->msgPtrLen;
//    memcpy(msgToQue.msgPtr, inEvtMsg->msgPtr, msgToQue.msgPtrLen);
    if(inEvtMsg->event == CloudServiceEvt_DEV_UPDATE)
    {
        if(lastDevUpdt)
        {
            free(lastDevUpdt);
        }
        lastDevUpdt = malloc(inEvtMsg->msgPtrLen);
        memcpy(lastDevUpdt, inEvtMsg->msgPtr, inEvtMsg->msgPtrLen);
//        mq_send(queDevUpdtMsgs, (const char*)&msgToQue, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }
    else
    {
        if(lastNwkUpdt)
        {
            free(lastNwkUpdt);
        }
        lastNwkUpdt = malloc(inEvtMsg->msgPtrLen);
        memcpy(lastNwkUpdt, inEvtMsg->msgPtr, inEvtMsg->msgPtrLen);
        //mq_send(queNwkUpdtMsgs, (const char*)&msgToQue, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }

}

/*!


 Public function defined in localWebSrvr.h
 */
void LocalWebSrvr_handleHttpEvt(msgQueue_t *inEvtMsg)
{
    SlNetAppRequest_t *netAppRequest;
    netAppRequest = (SlNetAppRequest_t*) inEvtMsg->msgPtr;

    if ((netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_GET) || (netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_DELETE ))
    {
       if (netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_GET)
       {
//           UART_PRINT("[Gateway Task] HTTP GET Request\n\r");
       }
       else
       {
//           UART_PRINT("[Gateway Task] HTTP DELETE Request\n\r");
       }

       httpGetHandler(netAppRequest);
    }

    else if ((netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_POST) || (netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_PUT))
    {

       if (netAppRequest->Type == SL_NETAPP_REQUEST_HTTP_POST)
       {
//           UART_PRINT("[Gateway Task] HTTP POST Request\n\r");
       }
       else
       {
//           UART_PRINT("[Gateway Task] HTTP PUT Request\n\r");
       }

//       UART_PRINT("[Gateway Task] Data received, len = %d, flags= %x\n\r", netAppRequest->requestData.PayloadLen, netAppRequest->requestData.Flags);

       httpPostHandler(netAppRequest);
    }

    if (netAppRequest->requestData.MetadataLen > 0)
    {
       free (netAppRequest->requestData.pMetadata);
    }
    if (netAppRequest->requestData.PayloadLen > 0)
    {
       free (netAppRequest->requestData.pPayload);
    }


}

/******************************************************************************
 Local Functions
 ****************************************************************************/
//*****************************************************************************
//
//! \brief This function is registered as netapp request callback
//!
//! \param[in]  pNetAppRequest        netapp request structure
//!
//! \param[out]  pNetAppResponse    netapp response structure
//!
//! \return none
//!
//****************************************************************************
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse)
{
    SlNetAppRequest_t *netAppRequest;
    int32_t msgqRetVal;

//    UART_PRINT("[Provisioning Task] NetApp Request Received - AppId = %d, Type = %d, Handle = %d\n\r", pNetAppRequest->AppId, pNetAppRequest->Type, pNetAppRequest->Handle);

    if ((pNetAppRequest->Type == SL_NETAPP_REQUEST_HTTP_GET) || (pNetAppRequest->Type == SL_NETAPP_REQUEST_HTTP_DELETE) ||
        (pNetAppRequest->Type == SL_NETAPP_REQUEST_HTTP_POST) || (pNetAppRequest->Type == SL_NETAPP_REQUEST_HTTP_PUT))
    {
//        UART_PRINT("[Provisioning Task] Request Type: %d\n\r", pNetAppRequest->Type);

        /* Prepare pending response */
        pNetAppResponse->Status = SL_NETAPP_RESPONSE_PENDING;
        pNetAppResponse->ResponseData.pMetadata = NULL;
        pNetAppResponse->ResponseData.MetadataLen = 0;
        pNetAppResponse->ResponseData.pPayload = NULL;
        pNetAppResponse->ResponseData.PayloadLen = 0;
        pNetAppResponse->ResponseData.Flags = 0;
    }
    else
    {
//        UART_PRINT("[Provisioning Task] Request Type Invalid\n\r");

        NetAppRequestErrorResponse(pNetAppResponse);
        return;
    }

    netAppRequest = (SlNetAppRequest_t *) malloc (sizeof(SlNetAppRequest_t));
    if (NULL == netAppRequest)
    {
//        UART_PRINT("[Provisioning Task] Error Allocating Memory for netAppRequest\n\r");
        NetAppRequestErrorResponse(pNetAppResponse);
        return;
    }

    netAppRequest->AppId = pNetAppRequest->AppId;
    netAppRequest->Type = pNetAppRequest->Type;
    netAppRequest->Handle = pNetAppRequest->Handle;
    netAppRequest->requestData.Flags = pNetAppRequest->requestData.Flags;

    /* Copy Metadata */
    if (pNetAppRequest->requestData.MetadataLen > 0)
    {
//        UART_PRINT("[Provisioning Task] netAppRequest->MetaData Detected\n\r");
        netAppRequest->requestData.pMetadata = (uint8_t *) malloc (pNetAppRequest->requestData.MetadataLen);
        if (NULL == netAppRequest->requestData.pMetadata)
        {
//            UART_PRINT("[Provisioning Task] Error Allocating Memory for netAppRequest->MetaData\n\r");
            NetAppRequestErrorResponse(pNetAppResponse);
            return;
        }

        sl_Memcpy(netAppRequest->requestData.pMetadata, pNetAppRequest->requestData.pMetadata, pNetAppRequest->requestData.MetadataLen);
        netAppRequest->requestData.MetadataLen = pNetAppRequest->requestData.MetadataLen;
    }
    else
    {
//        UART_PRINT("[Provisioning Task] No netAppRequest->MetaData Detected\n\r");
        netAppRequest->requestData.MetadataLen = 0;
    }

    /* Copy the payload */
    if (pNetAppRequest->requestData.PayloadLen > 0)
    {
//        UART_PRINT("[Provisioning Task] netAppRequest->Payload Detected\n\r");
        netAppRequest->requestData.pPayload = (uint8_t *) malloc (pNetAppRequest->requestData.PayloadLen);
        if (NULL == netAppRequest->requestData.pPayload)
        {
//            UART_PRINT("[Provisioning Task] Error Allocating Memory for netAppRequest->Payload\n\r");
            NetAppRequestErrorResponse(pNetAppResponse);

            return;
        }
        sl_Memcpy(netAppRequest->requestData.pPayload, pNetAppRequest->requestData.pPayload, pNetAppRequest->requestData.PayloadLen);
        netAppRequest->requestData.PayloadLen = pNetAppRequest->requestData.PayloadLen;
    }
    else
    {
//        UART_PRINT("[Provisioning Task] No netAppRequest->Payload Detected\n\r");
        netAppRequest->requestData.PayloadLen = 0;
    }

    msgQueue_t mqEvt = {CloudServiceEvt_LOCAL_SERVR_HTTP, netAppRequest, sizeof(SlNetAppRequest_t)};
    msgqRetVal = mq_send(cloudMq, (char*) &mqEvt, sizeof(msgQueue_t), 0);

    if(msgqRetVal < 0)
    {
        UART_PRINT("[PROVISIONING task] could not send element to msg queue\n\r");
        while (1);
    }

}

//*****************************************************************************
//
//! \brief This function prepare error netapp response in case memory could not be allocated
//!
//! \param[in]  pNetAppResponse    netapp response structure
//!
//! \return none
//!
//****************************************************************************
void NetAppRequestErrorResponse(SlNetAppResponse_t *pNetAppResponse)
{
    UART_PRINT("[Link local task] could not allocate memory for netapp request\n\r");

    /* Prepare error response */
    pNetAppResponse->Status = SL_NETAPP_RESPONSE_NONE;
    pNetAppResponse->ResponseData.pMetadata = NULL;
    pNetAppResponse->ResponseData.MetadataLen = 0;
    pNetAppResponse->ResponseData.pPayload = NULL;
    pNetAppResponse->ResponseData.PayloadLen = 0;
    pNetAppResponse->ResponseData.Flags = 0;
}

//*****************************************************************************
//
//! \brief This function parse and execute HTTP GET requests
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return None
//!
//****************************************************************************
void httpGetHandler(SlNetAppRequest_t *netAppRequest)
{
    int32_t status;
    uint8_t requestIdx;

    uint8_t    argcCallback;
    uint8_t     *argvArray;
    uint8_t     **argvCallback = &argvArray;

    argvArray = gHttpGetBuffer;

    status = httpCheckContentInDB(netAppRequest, &requestIdx, &argcCallback, argvCallback);

    if (status >= 0)
    {
        httpRequest[requestIdx].serviceCallback(requestIdx, &argcCallback, argvCallback, netAppRequest);
    }
}

//*****************************************************************************
//
//! \brief This function parse and execute HTTP POST/PUT requests
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return None
//!
//****************************************************************************
void httpPostHandler(SlNetAppRequest_t *netAppRequest)
{
    uint16_t metadataLen;
    int32_t status;
    uint8_t requestIdx;

    uint8_t    argcCallback;
    uint8_t     *argvArray;
    uint8_t     **argvCallback = &argvArray;

    argvArray = gHttpPostBuffer;

    status = httpCheckContentInDB(netAppRequest,&requestIdx,&argcCallback, argvCallback);

    if (status < 0)
    {
        metadataLen = preparePostMetadata(status);

        sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);
    }
    else
    {
        httpRequest[requestIdx].serviceCallback(requestIdx, &argcCallback, argvCallback, netAppRequest);
    }
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t devsGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{
    UART_PRINT("[Dev GET Handler] Callback Called: \n\r");
    uint16_t metadataLen;
    uint16_t devStatLen = strlen (lastDevUpdt);

    UART_PRINT("[Dev GET Handler] Dev Data: %s\n\r", lastDevUpdt);
    metadataLen = prepareGetMetadata(0, devStatLen, HttpContentTypeList_UrlEncoded);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, (SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION | SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA));
    UART_PRINT("[Link local task] Metadata Sent, len = %d \n\r", metadataLen);

    sl_NetAppSend (netAppRequest->Handle, devStatLen, (unsigned char*)lastDevUpdt, 0); /* mark as last segment */
    UART_PRINT("[Link local task] Data Sent, len = %d\n\r", devStatLen);

    return 0;
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t nwkGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{
    UART_PRINT("[NWK GET Handler] Callback Called: \n\r");
    uint16_t metadataLen;
    uint16_t nwkStatLen = strlen (lastNwkUpdt);

    metadataLen = prepareGetMetadata(0, nwkStatLen, HttpContentTypeList_UrlEncoded);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, (SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION | SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA));
    UART_PRINT("[NWK GET Handler] Metadata Sent; %s, len = %d \n\r", gMetadataBuffer, metadataLen);

    sl_NetAppSend (netAppRequest->Handle, nwkStatLen, (unsigned char*)lastNwkUpdt, 0); /* mark as last segment */
    UART_PRINT("[NWK GET Handler] Data Sent: %s, len = %d\n\r", lastNwkUpdt, nwkStatLen);

    return 0;
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t cmdPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{

    UART_PRINT("\n\r\n\r [Command POST Handler] Callback Called: \n\r\n\r");

    uint8_t *argvArray, ptemp;
    uint16_t metadataLen, elementType;
    deviceCmd_t *inCommand;
    msgQueue_t queueElementSend;

    inCommand = malloc(sizeof(deviceCmd_t));
    inCommand->cmdType = 0xFFFF;

    argvArray = *argvCallback;

    while (*argcCallback > 0)
    {
        elementType = setElementType(1, requestIdx, CONTENT_LEN_TYPE);
        if ( *((uint16_t *)argvArray) != elementType)       /* content length is irrelevant for POST */
        {
            if (*(argvArray + 1) & 0x80)    /* means it is the value, not the parameter */
            {
                switch(ptemp)
                {
                    case 0:
                        queueElementSend.event = GatewayEvent_PERMIT_JOIN;
                        inCommand->cmdType = CmdType_PERMIT_JOIN;
                        if(0 == memcmp((const char*) (argvArray + ARGV_VALUE_OFFSET), "true", strlen((const char*) (argvArray + ARGV_VALUE_OFFSET))))
                        {
                            inCommand->data = 1;
                        }
                        if(0 == memcmp((const char*) (argvArray + ARGV_VALUE_OFFSET), "false", strlen((const char*) (argvArray + ARGV_VALUE_OFFSET))))
                        {
                            inCommand->data = 0;
                        }
                        break;
                    default:
                        break;
                }
            }
            else{
                ptemp = *(argvArray + ARGV_VALUE_OFFSET);
            }
        }
        (*argcCallback)--;
        argvArray += ARGV_LEN_OFFSET;       /* skip the type */
        argvArray += *argvArray;    /* add the length */
        argvArray++;        /* skip the length */
    }

    queueElementSend.msgPtr = inCommand;
    mq_send(*gatewayCliMq, (char*) &queueElementSend, sizeof(msgQueue_t), MQ_LOW_PRIOR);

    metadataLen = preparePostMetadata(0);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);

    return 0;
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t actionPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{
    UART_PRINT("\n\r\n\r[Action POST Handler] Callback Called: \n\r\n\r");
    uint8_t *argvArray, ptemp;
    uint16_t metadataLen, elementType;
    char *extractedJson;
    deviceCmd_t *inCommand;
    inCommand = malloc(sizeof(deviceCmd_t));
    inCommand->cmdType = 0xFFFF;

    msgQueue_t queueElementSend;

    argvArray = *argvCallback;

    while (*argcCallback > 0)
    {
        elementType = setElementType(1, requestIdx, CONTENT_LEN_TYPE);
        if ( *((uint16_t *)argvArray) != elementType)       /* content length is irrelevant for POST */
        {
            if (*(argvArray + 1) & 0x80)    /* means it is the value, not the parameter */
            {
                switch(ptemp)
                {
                    case 0:
                        inCommand->shortAddr = 0x0000;
                        extractedJson = jsonParseIn((char*) (argvArray + ARGV_VALUE_OFFSET), "dstAddr");
                        if(!extractedJson)
                        {
                            extractedJson = jsonParseIn((char*) (argvArray + ARGV_VALUE_OFFSET), "ext_addr");
                            inCommand->shortAddr = 0xFFFF;
                        }


                        if(extractedJson)
                        {
                            UART_PRINT(" [CS_AWS] ExtractedJson: %s\n\r", extractedJson);
                            int shortAddr;
                            inCommand->cmdType = CmdType_DEVICE_DATA;
                            queueElementSend.event = GatewayEvent_DEVICE_CMD;
                            if(inCommand->shortAddr == 0x0000)
                            {
                                sscanf(extractedJson, "%x", &shortAddr);
                                inCommand->shortAddr = shortAddr;
                            }
                            else if(inCommand->shortAddr == 0xFFFF)
                            {
                                *(uint64_t*)inCommand->extAddr = strtoull (extractedJson, NULL, 16);
                            }
                            free(extractedJson);

                            extractedJson = jsonParseIn((char*) (argvArray + ARGV_VALUE_OFFSET), "devActionType");
                            if(extractedJson)
                            {
                                for(uint8_t cmds = 0; cmds < sizeof(incomingDevCmdsLocal); cmds++)
                                {
                                    if(strncmp(extractedJson, incomingDevCmdsLocal[cmds], strlen(incomingDevCmdsLocal[cmds])) == 0)
                                    {
                                        inCommand->cmdType = incomingDevCmdTypesLocal[cmds];
                                        break;
                                    }
                                }
                                free(extractedJson);
                            }



                            extractedJson = jsonParseIn((char*) (argvArray + ARGV_VALUE_OFFSET), "value");
                            if(extractedJson)
                            {
                                inCommand->data = atoi(extractedJson);
                                free(extractedJson);
                            }
                    //        extractedJson = jsonParseIn(tmpBuff, "toggleLED");
                    //        if(extractedJson)
                    //        {
                    //            inCommand->cmdType = CmdType_LED_DATA;
                    //            free(extractedJson);
                    //        }
                        }
                        break;
                    default:
                        break;
                }
            }
            else
            {
                ptemp = *(argvArray + ARGV_VALUE_OFFSET);
            }
        }
        (*argcCallback)--;
        argvArray += ARGV_LEN_OFFSET;       /* skip the type */
        argvArray += *argvArray;    /* add the length */
        argvArray++;        /* skip the length */
    }

    queueElementSend.msgPtr = inCommand;

    mq_send(*gatewayCliMq, (char*) &queueElementSend, sizeof(msgQueue_t), 0);



    metadataLen = preparePostMetadata(0);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);

    return 0;
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t cloudGetCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{
    UART_PRINT("\n\r\n\r[Cloud GET Handler] Callback Called: \n\n\r\r");

    char *pPayload;
    uint16_t metadataLen;
    uint16_t ipLen = sizeof(SlNetCfgIpV4Args_t);
    int32_t status = 0;
    SlNetCfgIpV4Args_t ipV4 = {0};

    pPayload = (char*)gPayloadBuffer;

    sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, NULL, &ipLen,(uint8_t *)&ipV4);
    sprintf(pPayload, "{\"ipAddress\": \"%d.%d.%d.%d\", \"wifiConnection\": %d, \"mqttConnection\": %d}",
            SL_IPV4_BYTE(ipV4.Ip,3),SL_IPV4_BYTE(ipV4.Ip,2),SL_IPV4_BYTE(ipV4.Ip,1),SL_IPV4_BYTE(ipV4.Ip,0),
            localWlanConnected, cloudConnected);
    sl_Memcpy(pPayload, gMetadataBuffer, strlen((const char *)gMetadataBuffer));

    UART_PRINT("[Cloud GET Handler] Payload: %s\n\r\n\r" , pPayload);

    status = 0;

    if (status != 0)
    {
        strcpy((char *)gPayloadBuffer, (const char *)pgNotFound);
    }


    metadataLen = prepareGetMetadata(status, strlen((const char *)gPayloadBuffer), HttpContentTypeList_UrlEncoded);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, (SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION | SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA));
    UART_PRINT("[Link local task] Metadata Sent, len = %d \n\r", metadataLen);

    sl_NetAppSend (netAppRequest->Handle, strlen ((const char *)gPayloadBuffer), gPayloadBuffer, 0); /* mark as last segment */
    UART_PRINT("[Link local task] Data Sent, len = %d\n\r", strlen ((const char *)gPayloadBuffer));

    return status;
}

//*****************************************************************************
//
//! \brief This is a generic device service callback function for HTTP GET
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  argcCallback        count of input params to the service callback
//!
//! \param[in]  argvCallback        set of input params to the service callback
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t cloudPostCallback(uint8_t requestIdx, uint8_t *argcCallback, uint8_t **argvCallback, SlNetAppRequest_t *netAppRequest)
{

    UART_PRINT("[Cloud POST Handler] Callback Called: \n\r");
    uint8_t *argvArray, ptemp;
    uint16_t metadataLen, elementType;

    argvArray = *argvCallback;

    while (*argcCallback > 0)
    {

        elementType = setElementType(1, requestIdx, CONTENT_LEN_TYPE);

        if ( *((uint16_t *)argvArray) != elementType)       /* content length is irrelevant for POST */
        {
            if (*(argvArray + 1) & 0x80)    /* means it is the value, not the parameter */
            {
                char *tempStr;
                tempStr = (char*) (argvArray + ARGV_VALUE_OFFSET);
                switch(ptemp)
                {
                    case ibmOrg:
                        if(cloudInfoBitMask & IBM_ORGID_MSK)
                        {
                            free(ibmCloudInfo.orgId);
                        }
                        ibmCloudInfo.orgId = (char*)malloc(strlen(tempStr)+1);
                        memcpy(ibmCloudInfo.orgId, tempStr, strlen(tempStr));
                        ibmCloudInfo.orgId[strlen(tempStr)] = '\0';
                        UART_PRINT("[Cloud POST Handler] Org ID: %s\n\r\n\r" , ibmCloudInfo.orgId);
                        cloudInfoBitMask |= IBM_ORGID_MSK;
                        break;
                    case deviceType:
                        if(cloudInfoBitMask & IBM_DEVTYPE_MSK)
                        {
                            free(ibmCloudInfo.devType);
                        }
                        ibmCloudInfo.devType = (char*)malloc(strlen(tempStr)+1);
                        memcpy(ibmCloudInfo.devType, tempStr, strlen(tempStr));
                        ibmCloudInfo.devType[strlen(tempStr)] = '\0';
                        cloudInfoBitMask |= IBM_DEVTYPE_MSK;
                        UART_PRINT("[Cloud POST Handler] dev Type: %s\n\r\n\r" , ibmCloudInfo.devType);
                        break;
                    case deviceId:
                        if(cloudInfoBitMask & IBM_DEVID_MSK)
                        {
                            free(ibmCloudInfo.devId);
                        }
                        ibmCloudInfo.devId = (char*)malloc(strlen(tempStr)+1);
                        memcpy(ibmCloudInfo.devId, tempStr, strlen(tempStr));
                        ibmCloudInfo.devId[strlen(tempStr)] = '\0';
                        cloudInfoBitMask |= IBM_DEVID_MSK;
                        UART_PRINT("[Cloud POST Handler] devId: %s\n\r\n\r" , ibmCloudInfo.devId);
                        break;
                    case password:
                        if(cloudInfoBitMask & IBM_SECTOKEN_MSK)
                        {
                            free(ibmCloudInfo.securityToken);
                        }
                        ibmCloudInfo.securityToken = (char*)malloc(strlen(tempStr)+1);
                        memcpy(ibmCloudInfo.securityToken, tempStr, strlen(tempStr));
                        ibmCloudInfo.securityToken[strlen(tempStr)] = '\0';
                        cloudInfoBitMask |= IBM_SECTOKEN_MSK;
                        UART_PRINT("[Cloud POST Handler] Password: %s\n\r\n\r" , ibmCloudInfo.securityToken);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                ptemp = *(argvArray + ARGV_VALUE_OFFSET);
            }
        }
        (*argcCallback)--;
        argvArray += ARGV_LEN_OFFSET;       /* skip the type */
        argvArray += *argvArray;    /* add the length */
        argvArray++;        /* skip the length */
    }

    if(cloudInfoBitMask & IBM_FULL_MSK)
    {
        CloudIBM_Info_t *qmsgClInfo = (CloudIBM_Info_t*)malloc(sizeof(CloudIBM_Info_t));
        memcpy(qmsgClInfo, &ibmCloudInfo, sizeof(CloudIBM_Info_t));
        msgQueue_t mqEvt = {CloudServiceEvt_CLOUDINFO_RX, qmsgClInfo, sizeof(CloudIBM_Info_t)};
        cloudInfoBitMask = 0;
        mq_send(cloudMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }
    metadataLen = preparePostMetadata(0);

    sl_NetAppSend (netAppRequest->Handle, metadataLen, gMetadataBuffer, SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);

    return 0;
}

//*****************************************************************************
//
//! \brief This function checks that the content requested via HTTP message exists
//!
//! \param[in] netAppRequest        netapp request structure
//!
//! \param[out]  requestIdx         request index to indicate the message
//!
//! \param[out]  argcCallback       count of input params to the service callback
//!
//! \param[out]  argvCallback       set of input params to the service callback
//!
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t    httpCheckContentInDB(SlNetAppRequest_t *netAppRequest, uint8_t *requestIdx, uint8_t *argcCallback, uint8_t **argvCallback)
{
    int32_t    status = -1;

    if (netAppRequest->AppId != SL_NETAPP_HTTP_SERVER_ID)
    {
        return status;
    }

    status = parseHttpRequestMetadata(netAppRequest->Type, netAppRequest->requestData.pMetadata, netAppRequest->requestData.MetadataLen, requestIdx, argcCallback, argvCallback);

    if ((0 == status) && (netAppRequest->requestData.PayloadLen != 0) && (netAppRequest->Type != SL_NETAPP_REQUEST_HTTP_PUT))    /* PUT does not contain parseable data - only POST does */
    {
        status = parseHttpRequestPayload(*requestIdx, netAppRequest->requestData.pPayload, netAppRequest->requestData.PayloadLen, argcCallback, argvCallback);
    }

    return status;
}

//*****************************************************************************
//
//! \brief This function prepares metadata for HTTP GET requests
//!
//! \param[in] parsingStatus        validity of HTTP GET request
//!
//! \param[in] contentLen           content length in respond to  HTTP GET request
//!
//! \return metadataLen
//!
//****************************************************************************
uint16_t prepareGetMetadata(int32_t parsingStatus, uint32_t contentLen, HttpContentTypeList contentTypeId)
{
    char *contentType;
    uint8_t *pMetadata;
    uint16_t metadataLen;

    contentType = g_ContentTypes[contentTypeId].contentTypeText;

    pMetadata = gMetadataBuffer;

    /* http status */
    *pMetadata = (uint8_t) SL_NETAPP_REQUEST_METADATA_TYPE_STATUS;
    pMetadata++;
    *(uint16_t *)pMetadata = (uint16_t) 2;
    pMetadata+=2;

    if (parsingStatus < 0)
    {
        *(uint16_t *)pMetadata = (uint16_t) SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND;
    }
    else
    {
        *(uint16_t *)pMetadata = (uint16_t) SL_NETAPP_HTTP_RESPONSE_200_OK;
    }

    pMetadata+=2;

    /* Content type */
    *pMetadata = (uint8_t) SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_TYPE;
    pMetadata++;
     (*(uint16_t *)pMetadata) = (uint16_t) strlen ((const char *)contentType);
    pMetadata+=2;
    sl_Memcpy (pMetadata, contentType, strlen((const char *)contentType));
    pMetadata+=strlen((const char *)contentType);

    /* Content len */
    *pMetadata = SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_LEN;
    pMetadata++;
    *(uint16_t *)pMetadata = (uint16_t) 4;
    pMetadata+=2;
    *(uint32_t *)pMetadata = (uint32_t) contentLen;

    metadataLen = 5 + 7 + strlen ((const char *)contentType) + 3;

    return metadataLen;
}

//*****************************************************************************
//
//! \brief This function prepares metadata for HTTP POST/PUT requests
//!
//! \param[in] parsingStatus        validity of HTTP POST/PUT request
//!
//! \return metadataLen
//!
//****************************************************************************
uint16_t preparePostMetadata(int32_t parsingStatus)
{
    uint8_t *pMetadata;
    uint16_t metadataLen;

    pMetadata = gMetadataBuffer;

    /* http status */
    *pMetadata = (uint8_t) SL_NETAPP_REQUEST_METADATA_TYPE_STATUS;
    pMetadata++;
    *(uint16_t *)pMetadata = (uint16_t) 2;
    pMetadata+=2;

    if (parsingStatus < 0)
    {
        *(uint16_t *)pMetadata = (uint16_t) SL_NETAPP_HTTP_RESPONSE_404_NOT_FOUND;
    }
    else
    {
        *(uint16_t *)pMetadata = (uint16_t) SL_NETAPP_HTTP_RESPONSE_204_OK_NO_CONTENT;        /* no need for content so browser stays on the same page */
    }

    pMetadata+=2;

    metadataLen = 5;

    return metadataLen;
}

//*****************************************************************************
//
//! \brief This function scan netapp request and parse the payload
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  pPayload            pointer to HTTP payload
//!
//! \param[in]  payloadLen          HTTP payload length
//!
//! \param[out]  argcCallback       count of input params to the service callback
//!
//! \param[out]  argvCallback       set of input params to the service callback
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t parseHttpRequestPayload(uint8_t requestIdx, uint8_t * pPayload, uint16_t payloadLen, uint8_t *argcCallback, uint8_t **argvCallback)
{
    int32_t status = -1;

    status = parseUrlEncoded(requestIdx, pPayload, payloadLen, argcCallback, argvCallback);

    if (status != 0)
    {
#ifdef HTTP_HANDLR_DEBUG
        UART_PRINT ("[HTTP Handler] query string in payload section is not valid/known\n\r");
#endif
    }

    return status;
}
//*****************************************************************************
//
//! \brief This function scan netapp request and parse the payload
//!
//! \param[in]  requestIdx         request index to indicate the message
//!
//! \param[in]  pPhrase            pointer to HTTP metadata payload
//!
//! \param[in]  payloadLen         HTTP metadata or payload length
//!
//! \param[out]  argcCallback      count of input params to the service callback
//!
//! \param[out]  argvCallback      set of input params to the service callback
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t parseUrlEncoded(uint8_t requestIdx, uint8_t * pPhrase, uint16_t phraseLen, uint8_t *argcCallback, uint8_t **argvCallback)
{
    uint8_t *token;
    uint8_t    characteristic, value, isValueExpected, loopIdx;
    int32_t status;
    uint8_t *argvArray;
    uint8_t remainingLen, actualLen;
    uint16_t elementType;

    argvArray = *argvCallback;

    if (*argcCallback > 0)        /* it means parameters already exist - fast forward to the end of argv */
    {
        loopIdx = *argcCallback;
        while (loopIdx > 0)
        {
            argvArray += ARGV_LEN_OFFSET;        /* skip the type */
            argvArray += *argvArray;    /* add the length */
            argvArray++;                /* skip the length */

            loopIdx--;
        }
    }

    /* check if values are expected per characteristic     */
    /* it is a 2 steps procedure:                        */
    /*     1) check that '=' sign exists                    */
    /*     2) check that the value is not NULL                */
    /* if not, return                                    */
    isValueExpected = 0;
    token = (uint8_t *)strchr((char *)pPhrase, '=');
    if (token != NULL)
    {
        if ( (*(token + 1) == '&') || (*(token + 1) ==  '\0') )    /* it means no value supplied */
        {
            return -1;
        }
        else
        {
            isValueExpected = 1;
        }
    }

    /* Parse payload list */
    token = (uint8_t *)strtok((char *)pPhrase, "=&");

    if (NULL == token)        /* it means there is no url encoded data */
    {
        return 0;
    }

    while(token && ((pPhrase + phraseLen) > token))
    {
        status = -1;
        characteristic = 0;

        /* run over all possible characteristics, if exist */
        while (httpRequest[requestIdx].charValues[characteristic].characteristic != NULL)
        {
            if(!strncmp((const char *)token, (const char *)httpRequest[requestIdx].charValues[characteristic].characteristic, strlen((const char *)httpRequest[requestIdx].charValues[characteristic].characteristic)))
            {
                status = 0;

                /* found a characteristic. save its index number */
                (*argcCallback)++;
                elementType = setElementType(0, requestIdx, characteristic);
                sl_Memcpy ((uint8_t*)argvArray, (uint8_t*)&elementType, ARGV_LEN_OFFSET);
                argvArray += ARGV_LEN_OFFSET;
                *argvArray++ = 1;        /* length field */
                *argvArray++ = characteristic;
                remainingLen = (uint8_t)(phraseLen - (uint8_t)(token - pPhrase) - strlen((const char *)token) - 1);    /* remaining length is for cases where the last value is of string type */
#ifdef HTTP_HANDLR_DEBUG
                UART_PRINT ("[HTTP Handler] characteristic is: %s\n\r", (int8_t *)httpRequest[requestIdx].charValues[characteristic].characteristic);
#endif
                break;
            }
            else
            {
                characteristic++;
            }
        }

        if (-1 == status)        /* it means the characteristics is not valid/known */
        {
            return status;
        }

        token = (uint8_t *)strtok(NULL, "=&");

        if (isValueExpected)
        {
            status = -1;
            value = 0;

            if (NULL == httpRequest[requestIdx].charValues[characteristic].value[value])        /* it means any value is OK */
            {
                status = 0;

                /* found a string value. copy its content */
                (*argcCallback)++;
                elementType = setElementType(1, requestIdx, value);
                sl_Memcpy ((uint8_t*)argvArray, (uint8_t*)&elementType, ARGV_LEN_OFFSET);
                argvArray += ARGV_LEN_OFFSET;
                if (strlen((const char *)token) > remainingLen)
                    actualLen = remainingLen;
                else
                    actualLen = strlen((const char *)token);

                *argvArray++ = (actualLen + 1);
                sl_Memcpy(argvArray, token, actualLen);
                argvArray += actualLen;
                *argvArray++ =  '\0';
#ifdef HTTP_HANDLR_DEBUG
                UART_PRINT ("[HTTP Handler] value is: %s\n\r", (int8_t *)(argvArray - actualLen - 1));
#endif
            }
            else
            {

                /* run over all possible values, if exist */
                while (httpRequest[requestIdx].charValues[characteristic].value[value] != NULL)
                {
                    if(!strncmp((const char *)token, (const char *)httpRequest[requestIdx].charValues[characteristic].value[value], strlen((const char *)httpRequest[requestIdx].charValues[characteristic].value[value])))
                    {
                        status = 0;

                        /* found a value. save its index number */
                        (*argcCallback)++;
                        elementType = setElementType(1, requestIdx, value);
                        sl_Memcpy ((uint8_t*)argvArray, (uint8_t*)&elementType, ARGV_LEN_OFFSET);
                        argvArray += ARGV_LEN_OFFSET;
                        *argvArray++ = 1;            /* length field */
                        *argvArray++ = value;
#ifdef HTTP_HANDLR_DEBUG
                        UART_PRINT ("[HTTP Handler] value is: %s\n\r", (int8_t *)httpRequest[requestIdx].charValues[characteristic].value[value]);
#endif
                        break;
                    }
                    else
                    {
                        value++;
                    }
                }

                if (-1 == status)        /* it means the value is not valid/known */
                {
                    return status;
                }
            }

            token = (uint8_t *)strtok(NULL, (const char *)"=&");
        }
    }

    return status;
}

//*****************************************************************************
//
//! \brief This function maps header type to its string value
//!
//! \param[in]  httpHeaderType        http header type
//!
//! \param[out]  httpHeaderText       http header text
//!
//! \return none
//!
//****************************************************************************
void convertHeaderType2Text (uint8_t httpHeaderType, uint8_t **httpHeaderText)
{
    int i;
    *httpHeaderText = NULL;

    for (i = 0; i < sizeof (g_HeaderFields)/sizeof(http_headerFieldType_t); i++)
    {
        if (g_HeaderFields[i].headerType == httpHeaderType)
        {
            *httpHeaderText = (uint8_t *)(g_HeaderFields[i].headerText);
            break;
        }
    }
}

//*****************************************************************************
//
//! \brief this function composes an element type from metadata/payload (TLV structure)
//!
//! \param[in]  isAnswer            states whether this is a value or a parameter
//!
//! \param[in]  requestIdx          request index to indicate the message
//!
//! \param[in]  elementVal          value of element
//!
//! \return element type
//!
//****************************************************************************
uint16_t setElementType(uint8_t isValue, uint8_t requestIdx, uint8_t elementVal)
{
    uint16_t elementType;

    elementType = elementVal;
    elementType |= (((isValue<<7) | (requestIdx & 0x7F)) << 8);

    return elementType;
}

//*****************************************************************************
//
//! \brief This function scan netapp request and parse the metadata
//!
//! \param[in]  requestType          HTTP method (GET, POST, PUT or DEL)
//!
//! \param[in]  pMetadata            pointer to HTTP metadata
//!
//! \param[in]  metadataLen          HTTP metadata length
//!
//! \param[out]  requestIdx          request index to indicate the message
//!
//! \param[out]  argcCallback        count of input params to the service callback
//!
//! \param[out]  argvCallback        set of input params to the service callback
//!
//! \return 0 on success else negative
//!
//****************************************************************************
int32_t parseHttpRequestMetadata(uint8_t requestType, uint8_t * pMetadata, uint16_t metadataLen, uint8_t *requestIdx, uint8_t *argcCallback, uint8_t **argvCallback)
{
    uint8_t *pTlv;
    uint8_t *pEnd;

    int32_t status = -1;
    uint8_t    loopIdx;
    uint8_t type;
    uint16_t len;
    uint32_t value;
    uint8_t *typeText;
    uint8_t    nullTerminator;
    uint8_t *argvArray;
    uint16_t elementType;

    argvArray = *argvCallback;

    *requestIdx = 0xFF;
    pTlv = pMetadata;
    pEnd = pMetadata + metadataLen;

    if (metadataLen < 3)
    {
#ifdef HTTP_HANDLR_DEBUG
        UART_PRINT("[HTTP Handler] Metadata parsing error\n\r");
#endif
        return -1;
    }
#ifdef HTTP_HANDLR_DEBUG
    UART_PRINT("[HTTP Handler] Metadata:\n\r");
#endif
    while (pTlv < pEnd)
    {
        type = *pTlv;
        pTlv++;
        len = *(uint16_t *)pTlv;
        pTlv+=2;

        convertHeaderType2Text(type, &typeText);

        if (typeText != NULL)
        {
#ifdef HTTP_HANDLR_DEBUG
            UART_PRINT ("[HTTP Handler] %s ", typeText);
#endif
        }

        switch (type)
        {
            case SL_NETAPP_REQUEST_METADATA_TYPE_STATUS:
                /* there are browsers that seem to send many 0 type for no reason */
                /* in this case, do not print anything */
                break;

            case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_LEN:
                if (0 == status)    /* it means there is a content length and URI is OK. Add it to the argv */
                {
                    if (*argcCallback > 0)        /* it means parameters already exist from query type */
                    {
                        loopIdx = *argcCallback;
                        while (loopIdx > 0)
                        {
                            argvArray += ARGV_LEN_OFFSET;        /* skip the type */
                            argvArray += *argvArray;    /* add the length */
                            argvArray++;                /* skip the length */

                            loopIdx--;
                        }
                    }

                    (*argcCallback)++;
                    elementType = setElementType(1, *requestIdx, CONTENT_LEN_TYPE);    /* add content type */
                    sl_Memcpy ((uint8_t*)argvArray, (uint8_t*)&elementType, ARGV_LEN_OFFSET);
                    argvArray += ARGV_LEN_OFFSET;
                    *argvArray++ = len;    /* add content length */
                    sl_Memcpy ((uint8_t*)argvArray, pTlv, len);
                    sl_Memcpy ((uint8_t*)&value, pTlv, len);
#ifdef HTTP_HANDLR_DEBUG
                    UART_PRINT ("%d\n\r", (uint32_t)value);
#endif
                }

                break;

            case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REQUEST_URI:
                *argcCallback = 0;    /* this is the 1st stop in every http method. zero out the character counter argument */

                for (loopIdx = 0; loopIdx < NUMBER_OF_URI_SERVICES; loopIdx++)
                {
                    if ((strncmp((const char *)pTlv, (const char *)httpRequest[loopIdx].service, strlen((const char *)httpRequest[loopIdx].service))) == 0)
                    {
                        if (requestType == httpRequest[loopIdx].httpMethod)
                        {
                            status = 0;
                            *requestIdx = httpRequest[loopIdx].requestIdx;
#ifdef HTTP_HANDLR_DEBUG
                            UART_PRINT ("%s\n\r", httpRequest[loopIdx].service);
#endif
                            break;
                        }
                    }
                }

                if (status != 0)
                {
#ifdef HTTP_HANDLR_DEBUG
                    UART_PRINT ("unknown service\n\r");
#endif
                }

                break;

            case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_QUERY_STRING:
                if (0 == status)
                {
                    status = parseUrlEncoded(*requestIdx, pTlv, len, argcCallback, argvCallback);

                    if (status != 0)
                    {
#ifdef HTTP_HANDLR_DEBUG
                        UART_PRINT ("query string in metadata section is not valid/known\n\r");
#endif
                    }
                }

                break;

            default:
                nullTerminator = *(pTlv + len);
                *(pTlv + len) =  '\0';
#ifdef HTTP_HANDLR_DEBUG
                UART_PRINT("%s\n\r", pTlv);
#endif
                *(pTlv + len) = nullTerminator;

                break;
        }
        pTlv+=len;
    }

    return status;
}





//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent    - Contains the relevant event information
//! \param[in]  pServerResponse - Should be filled by the user with the
//!                               relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent,
    SlNetAppHttpServerResponse_t *pHttpResponse)
{

    /* Unused in this application */
}
