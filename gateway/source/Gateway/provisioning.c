/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*****************************************************************************

 Application Name     - Provisioning application
 Application Overview - This application demonstrates how to use the provisioning method
                        in order to establish connection to the AP.  The application
                        connects to an AP and ping's the gateway to verify the connection.

 Application Details  - Refer to 'Provisioning' README.html

*****************************************************************************/
//****************************************************************************
//
//! \addtogroup
//! @{
//
//****************************************************************************

/* Standard Include */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* TI-DRIVERS Header files */
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>

#include <Utils/uart_term.h>
#include <Common/commonDefs.h>

#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <time.h>

#include "provisioning.h"




/* USER's defines */

#define TIMER_TASK_STACK_SIZE                   (256*2)
#define SL_STOP_TIMEOUT                         (200)
#define PROVISIONING_INACTIVITY_TIMEOUT         (1200)     /* Provisioning inactivity timeout in seconds (20 min)*/
#define RECONNECTION_ESTABLISHED_TIMEOUT_SEC    (15)   /* 15 seconds */
#define CONNECTION_PHASE_TIMEOUT_SEC            (2)    /* 2 seconds */
#define PING_TIMEOUT_SEC                        (1)    /* 1 second */



#define SC_KEY_ENABLE       (0)
#define SECURED_AP_ENABLE   (0)


#define AP_SEC_TYPE         (SL_WLAN_SEC_TYPE_WPA_WPA2)
#define AP_SEC_PASSWORD     "1234567890"
#define SC_KEY              "1234567890123456"

/* For Debug - Start Provisioning for each reset, or stay connected */
/* Used mainly in case of provisioning robustness tests             */
#define FORCE_PROVISIONING   (0)

#define ASSERT_ON_ERROR(error_code) \
            do{\
            /* Handling the error-codes is specific to the application */ \
            if (error_code < 0) {UART_PRINT("ERROR! %d \n\r",error_code); \
            gLedDisplayState = LedState_ERROR; } \
            /* else, continue w/ execution */ \
            }while(0)\

#define ROLE_STA 0
#define ROLE_AP  2


/* Application's states */
typedef enum
{
    AppState_STARTING,
    AppState_WAIT_FOR_CONNECTION,
    AppState_WAIT_FOR_IP,

    AppState_PINGING_GW,

    AppState_PROVISIONING_IN_PROGRESS,
    AppState_PROVISIONING_WAIT_COMPLETE,

    AppState_ERROR,
    AppState_MAX

}AppState;


 /* Function pointer to the event handler */
typedef int32_t (*fptr_EventHandler)(void);

/* Entry in the lookup table */
typedef struct _Provisioning_TableEntry_t_
{
    fptr_EventHandler   p_evtHndl;  /* Pointer to the event handler */
    AppState            nextState;  /* Next state of the application */
}Provisioning_TableEntry_t;

/* LED State */
typedef enum
{
    LedState_CONNECTION,
    LedState_FACTORY_DEFAULT,
    LedState_ERROR
}LedState;

/****************************************************************************
                      LOCAL FUNCTION PROTOTYPES
****************************************************************************/

int32_t ReportError(void);
int32_t HandleConnection(void);
int32_t HandleDiscnctEvt(void);
int32_t DoNothing(void);
int32_t HandleProvisioningComplete(void);
int32_t HandleProvisioningTimeOut(void);
int32_t CheckLanConnection(void);
int32_t CheckInternetConnection(void);
int32_t HandleUserApplication(void);
int32_t HandleWaitForIp(void);
int32_t ProcessRestartRequest(void);
int32_t ProvisioningStart(void);
int32_t SendPingToGW(void);
int32_t StartConnection(void);
int16_t SignalEvent(GatewayEvent event);
int32_t returnToFactoryDefault(void);
void    StopAsyncEvtTimer(void);

void tasksReportWlanDisconnected(void);
void tasksReportWlanConnected(void);
void taskInitComplete(void);


/****************************************************************************
                      GLOBAL VARIABLES
****************************************************************************/
pthread_t gProvisioningThread = (pthread_t)NULL;




uint8_t gReboot = 0;
uint8_t  gStopInProgress = 0;
uint32_t gPingSent = 0;
uint32_t gPingSuccess = 0;
uint8_t  gRole = ROLE_STA;
uint8_t gIsWlanConnected = 0;

mqd_t    provSrvrMq;

uint16_t gLedCount = 0;
uint8_t  gLedState = 0;
uint32_t gErrledCount = 0;
uint8_t  gErrLedState = 0;
uint8_t  gWaitForConn = 0;
LedState gLedDisplayState = LedState_CONNECTION;

timer_t gTimer;

#define SSL_SERVER_KEY          "dummy-root-ca-cert-key"
#define SSL_SERVER_CERT         "dummy-root-ca-cert"


#define NUMBER_OF_URI_SERVICES          (6)

#define LED_TOGGLE_OTA_PROCESS_TIMEOUT          (100)   /* In msecs */
const char *Roles[] = {"STA","STA","AP","P2P"};
const char *WlanStatus[] = {"DISCONNECTED","SCANING","CONNECTING","CONNECTED"};

const uint8_t pageNotFound[] = "<html>404 - Sorry page not found</html>";




/* Application lookup/transition table */
const Provisioning_TableEntry_t gTransitionTable[AppState_MAX][GatewayEvent_MAX] =
{
        /* AppState_STARTING */
    {
            /* Event: GatewayEvent_STARTED */      {StartConnection            , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_CONNECTED */    {HandleWaitForIp            , AppState_WAIT_FOR_IP             },
            /* Event: GatewayEvent_IP_ACQUIRED */  {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_DISCONNECT */   {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_PING_COMPLETE */{ReportError                , AppState_ERROR                   },
            /* GatewayEvent_PROVISIONING_STARTED */{ReportError                , AppState_ERROR                   },
            /* GatewayEvent_PROVISIONING_SUCCESS */{ReportError                , AppState_ERROR                   },
            /* GatewayEvent_PROVISIONING_STOPPED */{ReportError                , AppState_ERROR                   },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{ReportError              , AppState_ERROR                   },
            /* Event: GatewayEvent_TIMEOUT */      {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_RESTART */      {ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS}
    },
        /* AppState_WAIT_FOR_CONNECTION */
    {
            /* Event: GatewayEvent_STARTED */      {StartConnection            , AppState_WAIT_FOR_CONNECTION     },
            /** Event: GatewayEvent_CONNECTED */   {HandleWaitForIp            , AppState_WAIT_FOR_IP             },
            /* Event: GatewayEvent_IP_ACQUIRED */  {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_DISCONNECT */   {StartConnection            , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_PING_COMPLETE */{HandleProvisioningComplete , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_STARTED */{ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* GatewayEvent_PROVISIONING_SUCCESS */{DoNothing                  , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_STOPPED */{CheckLanConnection         , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{DoNothing                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_TIMEOUT */      {ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_ERROR */        {ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_RESTART */      {ProcessRestartRequest      , AppState_WAIT_FOR_CONNECTION     }
    },
        /* AppState_WAIT_FOR_IP */
    {
            /* Event: GatewayEvent_STARTED */      {ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_CONNECTED */    {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_IP_ACQUIRED */  {CheckLanConnection         , AppState_PINGING_GW              },
            /* Event: GatewayEvent_DISCONNECT */   {StartConnection            , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_PING_COMPLETE */{DoNothing                  , AppState_WAIT_FOR_IP             },
            /* GatewayEvent_PROVISIONING_STARTED */{ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* GatewayEvent_PROVISIONING_SUCCESS */{DoNothing                  , AppState_WAIT_FOR_IP             },
            /* GatewayEvent_PROVISIONING_STOPPED */{HandleProvisioningComplete , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{DoNothing                , AppState_WAIT_FOR_IP              },
            /* Event: GatewayEvent_TIMEOUT */      {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_RESTART */      {ProcessRestartRequest      , AppState_WAIT_FOR_CONNECTION     }
    },
        /* AppState_PINGING_GW */
    {
            /* Event: GatewayEvent_STARTED */      {DoNothing                  , AppState_PINGING_GW              },
            /* Event: GatewayEvent_CONNECTED */    {HandleWaitForIp            , AppState_WAIT_FOR_IP             },
            /* Event: GatewayEvent_IP_ACQUIRED */  {CheckInternetConnection    , AppState_PINGING_GW              },
            /* Event: GatewayEvent_DISCONNECT */   {HandleDiscnctEvt           , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_PING_COMPLETE */{DoNothing                  , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_STARTED */{ProvisioningStart          , AppState_PROVISIONING_IN_PROGRESS},
            /* GatewayEvent_PROVISIONING_SUCCESS */{DoNothing                  , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_STOPPED */{HandleUserApplication      , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{DoNothing                , AppState_PINGING_GW              },
            /*  Event: GatewayEvent_TIMEOUT */     {SendPingToGW                , AppState_PINGING_GW             },
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_RESTART */      {ProcessRestartRequest      , AppState_WAIT_FOR_CONNECTION     }
    },
        /* AppState_PROVISIONING_IN_PROGRESS  */
    {
            /* Event: GatewayEvent_STARTED */      {DoNothing                  , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_CONNECTED */    {HandleConnection           , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_IP_ACQUIRED */  {CheckInternetConnection    , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_DISCONNECT */   {HandleDiscnctEvt           , AppState_PROVISIONING_IN_PROGRESS},
            /* Event: GatewayEvent_PING_COMPLETE */{DoNothing                  , AppState_PROVISIONING_IN_PROGRESS},
            /* GatewayEvent_PROVISIONING_STARTED */{DoNothing                  , AppState_PROVISIONING_IN_PROGRESS},
            /* GatewayEvent_PROVISIONING_SUCCESS */{HandleProvisioningComplete , AppState_PROVISIONING_WAIT_COMPLETE},
            /* GatewayEvent_PROVISIONING_STOPPED */{HandleProvisioningComplete , AppState_PINGING_GW              },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{DoNothing                , AppState_WAIT_FOR_CONNECTION      },
            /* Event: GatewayEvent_TIMEOUT */      {ProcessRestartRequest      , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_ERROR                   },
            /* Event: GatewayEvent_RESTART */      {ProcessRestartRequest      , AppState_WAIT_FOR_CONNECTION     }
    },
        /* AppState_PROVISIONING_WAIT_COMPLETE */
    {
            /* Event: GatewayEvent_STARTED */      {DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* Event: GatewayEvent_CONNECTED */    {DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* Event: GatewayEvent_IP_ACQUIRED */  {DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* Event: GatewayEvent_DISCONNECT */   {HandleDiscnctEvt           , AppState_PROVISIONING_WAIT_COMPLETE},
            /* Event: GatewayEvent_PING_COMPLETE */{DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* GatewayEvent_PROVISIONING_STARTED */{DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* GatewayEvent_PROVISIONING_SUCCESS */{DoNothing                  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* GatewayEvent_PROVISIONING_STOPPED */{HandleUserApplication      , AppState_PINGING_GW                },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{DoNothing                , AppState_WAIT_FOR_CONNECTION        },
            /* Event: GatewayEvent_TIMEOUT */      {HandleProvisioningTimeOut  , AppState_PROVISIONING_WAIT_COMPLETE},
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_ERROR                     },
            /* Event: GatewayEvent_RESTART */      {ProcessRestartRequest      , AppState_PROVISIONING_WAIT_COMPLETE}
    },
        /* AppState_ERROR */
        /* we will restart connection for all errors */
    {
            /* Event: GatewayEvent_STARTED */      {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_CONNECTED */    {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_IP_ACQUIRED */  {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_DISCONNECT */   {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_PING_COMPLETE */{ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_STARTED */{ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_SUCCESS */{ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_STOPPED */{ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* GatewayEvent_PROVISIONING_WAIT_CONN */{ReportError              , AppState_WAIT_FOR_CONNECTION      },
            /* Event: GatewayEvent_TIMEOUT */      {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_ERROR */        {ReportError                , AppState_WAIT_FOR_CONNECTION     },
            /* Event: GatewayEvent_RESTART */      {ReportError                , AppState_WAIT_FOR_CONNECTION     }
    }
};

/* Application state's context */
AppState g_CurrentState;
char pDtemp[64], pTtemp[64];






/*****************************************************************************
                  SimpleLink Callback Functions
*****************************************************************************/

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
    /* Unused in this application */
}



//*****************************************************************************
//
//! \brief This function configures netapp settings
//!
//! \param[in]  None
//!
//! \return None
//!
//****************************************************************************
void SetNetAppHttp(int16_t *retVal, const uint8_t Option, const uint8_t OptionLen, const uint8_t *pOptionValue)
{
    *retVal =  sl_NetAppSet(SL_NETAPP_HTTP_SERVER_ID, Option, OptionLen, pOptionValue);
    if(*retVal != 0)
    {
        UART_PRINT("HTTP setting error: %d, option: %d \n\r", *retVal, Option);
    }
}


//*****************************************************************************
//
//! \brief  This function configures the HTTPS server
//!
//! \param  None
//!
//! \return NetApp error codes or 0 upon success.
//!
//*****************************************************************************
int32_t ConfigureHttpsServer(void)
{
    int16_t retVal = 0;
    uint8_t httpsPort[] = {0xBB, 0x01};  /* 0x1BB = 443 */
    uint8_t secondaryPort[] = {0x50, 0x00}; /* 0x050 = 80 */
    uint8_t secondaryPortEnable[] = {0x1};
    uint8_t securityMode = 1;

    UART_PRINT("ConfigureHttpsServer for secured mode...\n\r");

    /* Set the file names used for SSL key exchange */
    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_DEVICE_CERTIFICATE_FILENAME,
            strlen((char *)SSL_SERVER_CERT),
            (const uint8_t *)SSL_SERVER_CERT);

    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_PRIVATE_KEY_FILENAME,
            strlen((char *)SSL_SERVER_KEY),
            (const uint8_t *)SSL_SERVER_KEY);


    /* Activate SSL security on primary HTTP port and change it to
     443 (standard HTTPS port) */
    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_PRIMARY_PORT_SECURITY_MODE,
            sizeof(securityMode),
            &securityMode);

    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_PRIMARY_PORT_NUMBER,
            sizeof(httpsPort),
            httpsPort);

    /* Enable secondary HTTP port (can only be used for redirecting
     connections to the secure primary port) */
    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_SECONDARY_PORT_NUMBER,
            sizeof(secondaryPort),
            secondaryPort);

    SetNetAppHttp(&retVal,
            SL_NETAPP_HTTP_SECONDARY_PORT_ENABLE,
            sizeof(secondaryPortEnable),
            secondaryPortEnable);

    if(retVal >= 0)
    {
        retVal = sl_NetAppStop(SL_NETAPP_HTTP_SERVER_ID);
        UART_PRINT("[Provisioning App] HTTP Server Stopped\n\r");
        if(retVal >= 0)
        {
            retVal = sl_NetAppStart(SL_NETAPP_HTTP_SERVER_ID);
            UART_PRINT("[Provisioning App] HTTP Server Re-started\n\r");
        }
    }
    return retVal;
}


//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Id)
    {
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,3),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,2),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,1),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,0),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,0));
        SignalEvent(GatewayEvent_IP_ACQUIRED);
        break;

    case SL_NETAPP_EVENT_DHCPV4_LEASED:

        UART_PRINT("[NETAPP Event] IP Leased: %d.%d.%d.%d\r\n",
            SL_IPV4_BYTE(pNetAppEvent->Data.IpLeased.IpAddress,3),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpLeased.IpAddress,2),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpLeased.IpAddress,1),
            SL_IPV4_BYTE(pNetAppEvent->Data.IpLeased.IpAddress,0));
        SignalEvent(GatewayEvent_IP_ACQUIRED);
        break;

    case SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT:
        UART_PRINT("[NETAPP Event] IP acquired timeout \r\n");
        break;


    default:
        UART_PRINT("[NETAPP EVENT] Unhandled event [0x%x] \n\r", pNetAppEvent->Id);
        break;
    }
}

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{

    SlNetCfgIpV4Args_t ipV4 = {0};

    switch(pWlanEvent->Id)
    {
    case SL_WLAN_EVENT_CONNECT:
        UART_PRINT(" [Event] STA connected to AP - BSSID:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x, ",
            pWlanEvent->Data.Connect.Bssid[0],
            pWlanEvent->Data.Connect.Bssid[1],
            pWlanEvent->Data.Connect.Bssid[2],
            pWlanEvent->Data.Connect.Bssid[3],
            pWlanEvent->Data.Connect.Bssid[4],
            pWlanEvent->Data.Connect.Bssid[5]);

        /* set the string terminate */
        pWlanEvent->Data.Connect.SsidName[pWlanEvent->Data.Connect.SsidLen] = '\0';

        UART_PRINT("SSID:%s\r\n", pWlanEvent->Data.Connect.SsidName);
        SignalEvent(GatewayEvent_CONNECTED);
        break;

    case SL_WLAN_EVENT_DISCONNECT:
        UART_PRINT(" [Event] STA disconnected from AP (Reason Code = %d)\r\n", pWlanEvent->Data.Disconnect.ReasonCode);
        SignalEvent(GatewayEvent_DISCONNECT);
        break;

    case SL_WLAN_EVENT_STA_ADDED:
        UART_PRINT(" [Event] New STA Added (MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x)\r\n",
            pWlanEvent->Data.STAAdded.Mac[0],
            pWlanEvent->Data.STAAdded.Mac[1],
            pWlanEvent->Data.STAAdded.Mac[2],
            pWlanEvent->Data.STAAdded.Mac[3],
            pWlanEvent->Data.STAAdded.Mac[4],
            pWlanEvent->Data.STAAdded.Mac[5] );
        break;

    case SL_WLAN_EVENT_STA_REMOVED:
        UART_PRINT(" [Event] STA Removed (MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x)\r\n",
            pWlanEvent->Data.STAAdded.Mac[0],
            pWlanEvent->Data.STAAdded.Mac[1],
            pWlanEvent->Data.STAAdded.Mac[2],
            pWlanEvent->Data.STAAdded.Mac[3],
            pWlanEvent->Data.STAAdded.Mac[4],
            pWlanEvent->Data.STAAdded.Mac[5]);
        break;

    case SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED:
        UART_PRINT(" [Provisioning] Profile Added: SSID: %s\r\n", pWlanEvent->Data.ProvisioningProfileAdded.Ssid);
        if(pWlanEvent->Data.ProvisioningProfileAdded.ReservedLen > 0)
        {
            UART_PRINT(" [Provisioning] Profile Added: PrivateToken:%s\r\n", pWlanEvent->Data.ProvisioningProfileAdded.Reserved);
        }
        break;

    case SL_WLAN_EVENT_PROVISIONING_STATUS:
        {
            switch(pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus)
            {
            case SL_WLAN_PROVISIONING_GENERAL_ERROR:
            case SL_WLAN_PROVISIONING_ERROR_ABORT:
            case SL_WLAN_PROVISIONING_ERROR_ABORT_INVALID_PARAM:
            case SL_WLAN_PROVISIONING_ERROR_ABORT_HTTP_SERVER_DISABLED:
            case SL_WLAN_PROVISIONING_ERROR_ABORT_PROFILE_LIST_FULL:
            case SL_WLAN_PROVISIONING_ERROR_ABORT_PROVISIONING_ALREADY_STARTED:
                UART_PRINT(" [Provisioning] Provisioning Error status=%d\r\n",pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
                SignalEvent(GatewayEvent_ERROR);
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_NETWORK_NOT_FOUND:
                UART_PRINT(" [Provisioning] Profile confirmation failed: network not found\r\n");
                SignalEvent(GatewayEvent_PROVISIONING_STARTED);
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_CONNECTION_FAILED:
                UART_PRINT(" [Provisioning] Profile confirmation failed: Connection failed\r\n");
                SignalEvent(GatewayEvent_PROVISIONING_STARTED);
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_CONNECTION_SUCCESS_IP_NOT_ACQUIRED:
                UART_PRINT(" [Provisioning] Profile confirmation failed: IP address not acquired\r\n");
                SignalEvent(GatewayEvent_PROVISIONING_STARTED);
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS_FEEDBACK_FAILED:
                UART_PRINT(" [Provisioning] Profile Confirmation feedback failed (Connection Success, feedback to Smartphone app failed)\r\n");
                SignalEvent(GatewayEvent_PROVISIONING_STARTED);
                //SignalEvent(GatewayEvent_PROVISIONING_SUCCESS);
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS:
                UART_PRINT(" [Provisioning] Profile Confirmation Success!\r\n");
                SignalEvent(GatewayEvent_PROVISIONING_SUCCESS);
                break;

            case SL_WLAN_PROVISIONING_AUTO_STARTED:
                UART_PRINT(" [Provisioning] Auto-Provisioning Started\r\n");
                SignalEvent(GatewayEvent_RESTART);
                break;

            case SL_WLAN_PROVISIONING_STOPPED:
                UART_PRINT("\r\n Provisioning stopped:");
                UART_PRINT(" Current Role: %s\r\n",Roles[pWlanEvent->Data.ProvisioningStatus.Role]);
                if(ROLE_STA == pWlanEvent->Data.ProvisioningStatus.Role)
                {
                    UART_PRINT("WLAN Status: %s\r\n",WlanStatus[pWlanEvent->Data.ProvisioningStatus.WlanStatus]);

                    if(SL_WLAN_STATUS_CONNECTED == pWlanEvent->Data.ProvisioningStatus.WlanStatus)
                    {

                        UART_PRINT("Connected to SSID: %s\r\n",pWlanEvent->Data.ProvisioningStatus.Ssid);
                        gReboot = 1;
                        tasksReportWlanConnected();
                        SignalEvent(GatewayEvent_PROVISIONING_STOPPED);
                    }
                    else if (SL_WLAN_STATUS_SCANING == pWlanEvent->Data.ProvisioningStatus.WlanStatus)
                    {
                        gWaitForConn = 1;
                        SignalEvent(GatewayEvent_PROVISIONING_WAIT_CONN);
                    }
                }
                else
                {
                    SignalEvent(GatewayEvent_RESTART);
                }
                break;

            case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNCED:
                UART_PRINT(" [Provisioning] Smart Config Synced!\r\n");
                break;

            case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNC_TIMEOUT:
                UART_PRINT(" [Provisioning] Smart Config Sync Timeout!\r\n");
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT:
                UART_PRINT(" [Provisioning] Profile confirmation: WLAN Connected!\r\n");
                break;

            case SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED:
            {
                uint16_t ipLen = sizeof(SlNetCfgIpV4Args_t);
                /* Get IP and Gateway information */
                sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE,NULL,&ipLen,(uint8_t *)&ipV4);
                tasksReportWlanConnected();
                UART_PRINT(" [Provisioning] Profile confirmation: IP Acquired! [%d.%d.%d.%d]\r\n",  SL_IPV4_BYTE(ipV4.Ip,3),SL_IPV4_BYTE(ipV4.Ip,2),SL_IPV4_BYTE(ipV4.Ip,1),SL_IPV4_BYTE(ipV4.Ip,0));
                sl_WlanSetMode(ROLE_STA);
                sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP,0xFF,0,NULL, 0x0);
                SignalEvent(GatewayEvent_PROVISIONING_SUCCESS);
            }
                break;

            case SL_WLAN_PROVISIONING_EXTERNAL_CONFIGURATION_READY:
                UART_PRINT(" [Provisioning] External configuration is ready! \r\n");
                break;

            default:
                UART_PRINT(" [Provisioning] Unknown Provisioning Status: %d\r\n",pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
                break;
            }
        }
        break;

    default:
        UART_PRINT(" [Event] - WlanEventHandler has received %d !!!!\r\n",pWlanEvent->Id);
        break;
    }
}

//*****************************************************************************
//
//! \brief The Function Handles the Fatal errors
//!
//! \param[in]  slFatalErrorEvent - Pointer to Fatal Error Event info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{

    switch (slFatalErrorEvent->Id)
    {
    case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
        {
            UART_PRINT("[ERROR] - FATAL ERROR: Abort NWP event detected: "
                "AbortType=%d, AbortData=0x%x\n\r",
                slFatalErrorEvent->Data.DeviceAssert.Code,
                slFatalErrorEvent->Data.DeviceAssert.Value);
        }
        break;

    case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
        {
            UART_PRINT("[Provisioning ERROR] - FATAL ERROR: Driver Abort detected\n\r");
            SignalEvent(GatewayEvent_RESTART);
        }
        break;

    case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
        {
            UART_PRINT("[ERROR] - FATAL ERROR: No Cmd Ack detected "
                "[cmd opcode = 0x%x] \n\r",
                slFatalErrorEvent->Data.NoCmdAck.Code);
        }
        break;

    case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
        {
            UART_PRINT("[ERROR] - FATAL ERROR: Sync loss detected n\r");
        }
        break;

    case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
        {
            UART_PRINT("[ERROR] - FATAL ERROR: Async event timeout detected "
                "[event opcode =0x%x]  \n\r",
                slFatalErrorEvent->Data.CmdTimeout.Code);
        }
        break;

    default:
        UART_PRINT("[ERROR] - FATAL ERROR: Unspecified error detected \n\r");
        break;
    }
}



//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]  pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /* Most of the general errors are not FATAL are are to be handled
    appropriately by the application */
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
        pDevEvent->Data.Error.Code,
        pDevEvent->Data.Error.Source);
}

//*****************************************************************************
//
//! \brief This function handles socket events indication
//!
//! \param[in]  pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    /* This application doesn't work with socket - Events are not expected */
    switch( pSock->Event )
    {
    case SL_SOCKET_TX_FAILED_EVENT:
        switch( pSock->SocketAsyncEvent.SockTxFailData.Status)
        {
        case SL_ERROR_BSD_ECLOSE:
            UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                "failed to transmit all queued packets\n\r",
                pSock->SocketAsyncEvent.SockTxFailData.Sd);
            break;
        default:
            UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , "
                "reason (%d) \n\n",
                pSock->SocketAsyncEvent.SockTxFailData.Sd,
                pSock->SocketAsyncEvent.SockTxFailData.Status);
            break;
        }
        break;

    default:
        UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
        break;
    }
}


//*****************************************************************************
//                 Local Functions
//*****************************************************************************


//*****************************************************************************
//
//! \brief The interrupt handler for the async-evt timer
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void AsyncEvtTimerHandler(sigval arg)
{
    SignalEvent(GatewayEvent_TIMEOUT);
}

//*****************************************************************************
//
//! \brief Call Host Timer
//!
//! \param  Duration in mSec
//!
//! \return none
//!
//*****************************************************************************
void StartAsyncEvtTimer(uint32_t duration)
{
    struct itimerspec value;

    /* Start timer */
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = 0;
    value.it_value.tv_sec = duration;
    value.it_value.tv_nsec = 0;

    timer_settime(gTimer, 0, &value, NULL);
}

//*****************************************************************************
//
//! \brief Stop Timer
//!
//! \param  None
//!
//! \return none
//!
//*****************************************************************************
void StopAsyncEvtTimer(void)
{
    struct itimerspec value;

    /* Stop timer */
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_nsec = 0;
    value.it_value.tv_sec = 0;
    value.it_value.tv_nsec = 0;

    timer_settime(gTimer, 0, &value, NULL);
}

//*****************************************************************************
//
//! \brief Error Handler - Print and restart connection
//!
//! \param  None
//!
//! \return none
//!
//*****************************************************************************
int32_t ReportError(void)
{
    UART_PRINT("[Provisioning] Provisioning Application Error - Restarting \r\n ");
//    gIsWlanConnected = 0;
    tasksReportWlanDisconnected();
    sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP, 0 , 0, NULL, (uint32_t)NULL);
    StartAsyncEvtTimer(CONNECTION_PHASE_TIMEOUT_SEC);

    return 0;
}

//*****************************************************************************
//
//! \brief Restart request - disconnected and open timer to re-open provisioning for non connection
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t ProcessRestartRequest(void)
{
//    gIsWlanConnected = 0;
    tasksReportWlanDisconnected();
    StartAsyncEvtTimer(CONNECTION_PHASE_TIMEOUT_SEC);

    return 0;
}
//*****************************************************************************
//
//! \brief Get AP security parameters
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t GetSecureStatus(void)
{
    uint8_t sec_type;
    uint16_t len = 1;
    uint16_t  config_opt = SL_WLAN_AP_OPT_SECURITY_TYPE;
    sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt, &len, (_u8* )&sec_type);
    if (sec_type == AP_SEC_TYPE)
    {
        return 1;
    }
    return 0;
}

//*****************************************************************************
//
//! \brief Set AP security parameters
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t SetSecuredAP(const uint8_t sec_en)
{
    //set secured AP parameters
     uint8_t  val = AP_SEC_TYPE;
     uint8_t  open = SL_WLAN_SEC_TYPE_OPEN ;
     uint8_t  password[65];
     uint16_t  len = strlen((char *)AP_SEC_PASSWORD);
     if (sec_en)
     {
         sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SECURITY_TYPE, 1, (uint8_t *)&val);
         memset(password, 0, sizeof(password));
         memcpy(password, (char *)AP_SEC_PASSWORD, len);
         sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_PASSWORD, len, (uint8_t *)password);
         UART_PRINT(" Setting AP secured parameters\n\r");
     }
     else
     {
        sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SECURITY_TYPE, 1, (uint8_t *)&open);
     }
     return 0;
}

//*****************************************************************************
//
//! \brief  Start connection - wait for connection
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t StartConnection(void)
{
//    gIsWlanConnected = 0;
    tasksReportWlanDisconnected();
    StartAsyncEvtTimer(CONNECTION_PHASE_TIMEOUT_SEC);

    return 0;
}

//*****************************************************************************
//
//! \brief  Connection event arriver, open timer for IP ACQUIRED
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleConnection(void)
{
//    gIsWlanConnected = 1;
    tasksReportWlanConnected();
    return 0;
}

//*****************************************************************************
//
//! \brief  Provisioning completed succesfully, connection established
//!         Open timer to make sure application nagotiation completed as well
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleProvisioningComplete(void)
{
    gStopInProgress = 0;
    UART_PRINT("[Provisioning] Provisioning Application Ended Successfully \r\n ");
    //StartAsyncEvtTimer(RECONNECTION_ESTABLISHED_TIMEOUT_SEC);
    return 0;
}

//*****************************************************************************
//
//! \brief  Application side function
//!         Ping to Gateway and open timer for next ping
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t SendPingToGW(void)
{
    return 0;
}

//*****************************************************************************
//
//! \brief  Application side function
//!          Print AP parameters, and open timer to start real application
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleUserApplication(void)
{
    if(gReboot)
    {
        sl_Stop(SL_STOP_TIMEOUT);
        gRole = sl_Start(NULL, NULL, NULL);
        MAP_PRCMHibernateIntervalSet(330);
        MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
        MAP_PRCMHibernateEnter();
    }
    tasksReportWlanConnected();
//    gIsWlanConnected = 1;
    UART_PRINT("[Provisioning] User Application Started \r\n ");

    gWaitForConn = 0;
    /* as long as the device connected, run user application */

    /* Get IP and Gateway information */
    uint16_t len = sizeof(SlNetCfgIpV4Args_t);
    uint16_t ConfigOpt = 0;   /* return value could be one of the following: SL_NETCFG_ADDR_DHCP / SL_NETCFG_ADDR_DHCP_LLA / SL_NETCFG_ADDR_STATIC  */
    SlNetCfgIpV4Args_t ipV4 = {0};

    sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE,&ConfigOpt,&len,(uint8_t *)&ipV4);

    UART_PRINT("\tDHCP is %s \r\n\tIP \t%d.%d.%d.%d \r\n\tMASK \t%d.%d.%d.%d \r\n\tGW \t%d.%d.%d.%d \r\n\tDNS \t%d.%d.%d.%d\n\r",
        (ConfigOpt == SL_NETCFG_ADDR_DHCP) ? "ON" : "OFF",
        SL_IPV4_BYTE(ipV4.Ip,3),SL_IPV4_BYTE(ipV4.Ip,2),SL_IPV4_BYTE(ipV4.Ip,1),SL_IPV4_BYTE(ipV4.Ip,0),
        SL_IPV4_BYTE(ipV4.IpMask,3),SL_IPV4_BYTE(ipV4.IpMask,2),SL_IPV4_BYTE(ipV4.IpMask,1),SL_IPV4_BYTE(ipV4.IpMask,0),
        SL_IPV4_BYTE(ipV4.IpGateway,3),SL_IPV4_BYTE(ipV4.IpGateway,2),SL_IPV4_BYTE(ipV4.IpGateway,1),SL_IPV4_BYTE(ipV4.IpGateway,0),
        SL_IPV4_BYTE(ipV4.IpDnsServer,3),SL_IPV4_BYTE(ipV4.IpDnsServer,2),SL_IPV4_BYTE(ipV4.IpDnsServer,1),SL_IPV4_BYTE(ipV4.IpDnsServer,0));

    /* Reset Over All Ping Statistics*/
    gPingSent = 0;
    gPingSuccess = 0;

    StartAsyncEvtTimer(PING_TIMEOUT_SEC);


    return 0;
}

//*****************************************************************************
//
//! \brief   Wait for IP, open timer for error
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleWaitForIp(void)
{
    StartAsyncEvtTimer(CONNECTION_PHASE_TIMEOUT_SEC * 3);
    return 0;
}

//*****************************************************************************
//
//! \brief  Disconnect received, disconnect and open timer for re-connect
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleDiscnctEvt(void)
{
//    gIsWlanConnected = 0;
    tasksReportWlanDisconnected();
    StartAsyncEvtTimer(RECONNECTION_ESTABLISHED_TIMEOUT_SEC);
    return 0;
}

//*****************************************************************************
//
//! \brief  Connection established, check if force provisioning (robustnes test) or move to
//!         user application.
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t CheckLanConnection(void)
{
    /* Force Provisioning Application to run */
    if ((FORCE_PROVISIONING)&&(!gWaitForConn))
    {
        UART_PRINT("[App] Force Provisioning Start\r\n");
        returnToFactoryDefault();
        SignalEvent(GatewayEvent_PROVISIONING_STARTED);
    }
    else
    {
        /* Handle User application */
        SignalEvent(GatewayEvent_PROVISIONING_STOPPED);
    }
    return 0;
}

//*****************************************************************************
//
//! \brief   IP Acquired, open timer to make sure process completed
//!         user application.
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t CheckInternetConnection(void)
{
    /* Connect to cloud if enabled */
    return 0;
}

//*****************************************************************************
//
//! \brief   Do Nothing
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t DoNothing(void)
{
    return 0;
}

//*****************************************************************************
//
//! \brief   Handle Provisioning timeout, check if connection success
//!          Continue if success, or start over if fail
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t HandleProvisioningTimeOut(void)
{
    /* Connection failed, restart */
    SignalEvent(GatewayEvent_RESTART);
    return 0;
}

//*****************************************************************************
//
//! \brief  This function signals the application events
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int16_t SignalEvent(GatewayEvent event)
{
    //
    //signal provisioning task about SL Event
    //
    msgQueue_t mqEvt = {event, NULL, 0};
    mq_send(provSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);

    return 0;
}


//*****************************************************************************
//
//! \brief  This function handles init-complete event from SL
//!
//! \param  status - Mode the device is configured or error if initialization failed
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkInitCallback(uint32_t status, SlDeviceInitInfo_t *DeviceInitInfo)
{
    if ((int32_t)status == SL_ERROR_RESTORE_IMAGE_COMPLETE)
    {
        UART_PRINT("\r\n**********************************\r\nReturn to Factory Default been Completed\r\nPlease RESET the Board\r\n**********************************\r\n");
        gLedDisplayState = LedState_FACTORY_DEFAULT;
        while(1);
    }

    ASSERT_ON_ERROR((int32_t)status);


    UART_PRINT("Device started in %s role\n\r", (0 == status) ? "Station" :\
        ((2 == status) ? "AP" : "P2P"));

    if(ROLE_STA == status)
    {
        gRole = ROLE_STA;
//        gIsWlanConnected = 1;
        //tasksReportWlanConnected();
        SignalEvent(GatewayEvent_STARTED);
    }
    else if (ROLE_AP == status)
    {
        gRole = ROLE_AP;
//        gIsWlanConnected = 0;
        tasksReportWlanDisconnected();
        SignalEvent(GatewayEvent_RESTART);
    }
    else
    {
        SignalEvent(GatewayEvent_ERROR);
    }
}

//*****************************************************************************
//
//! \brief  In case external confirmation (cloud) failed, need to abort
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
void _AbortProvExternalConfirmation(void)
{
    int16_t retVal;

    UART_PRINT("Aborting provisioning external confirmation...\r\n");
    retVal = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_ABORT_EXTERNAL_CONFIRMATION, 0, 0, NULL, 0);
    UART_PRINT("Abort retVal=%d\r\n", retVal);
}

//*****************************************************************************
//
//! \brief  Delete all profiles and return to factory default
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t returnToFactoryDefault(void)
{
    int32_t slRetVal;
    SlFsRetToFactoryCommand_t RetToFactoryCommand;
    int32_t ExtendedError;
    unsigned char event;
    Provisioning_TableEntry_t   *pEntry = NULL;
    msgQueue_t incomingMsg;

    RetToFactoryCommand.Operation = SL_FS_FACTORY_RET_TO_DEFAULT;
    slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_RESTORE, 0, NULL , (uint8_t *)&RetToFactoryCommand , sizeof(SlFsRetToFactoryCommand_t), NULL, 0 , NULL );

    /* if command was not executed because provisioning is currently running, send
     * PRVISIONING_STOP command, and wait for the PROVISIONING_STOPPED event
     * (which confirms that the provisioning process was successfully stopped)
     * before trying again */

    if(SL_RET_CODE_PROVISIONING_IN_PROGRESS == slRetVal)
    {
        UART_PRINT(" [ERROR] Provisioning is already running, stopping current session...\r\n");
        gStopInProgress = 1;
        slRetVal = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP,0 , 0, NULL, (uint32_t)NULL);
        ASSERT_ON_ERROR(slRetVal);

        while(gStopInProgress)
        {
            incomingMsg.event = CommonEvent_INVALID_EVENT;
            incomingMsg.msgPtr = NULL;
            incomingMsg.msgPtrLen = 0;
            mq_receive(provSrvrMq, (char*) &incomingMsg, sizeof(msgQueue_t), NULL);

            event = (unsigned char)incomingMsg.event;

            StopAsyncEvtTimer();

            /* Find Next event entry */
            pEntry = (Provisioning_TableEntry_t *)&gTransitionTable[g_CurrentState][event];

            if (NULL != pEntry->p_evtHndl)
            {
                if (pEntry->p_evtHndl() < 0)
                {
                    UART_PRINT("Event handler failed..!! \n\r");
                    while(1);
                }
            }

            /* Change state according to event */
            if (pEntry->nextState != g_CurrentState)
            {
                g_CurrentState = pEntry->nextState;
            }
        }

        SignalEvent(GatewayEvent_RESTART);

        slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_RESTORE, 0, NULL , (uint8_t *)&RetToFactoryCommand , sizeof(SlFsRetToFactoryCommand_t), NULL, 0 , NULL );
        ASSERT_ON_ERROR(slRetVal);
    }
    if ((_i32)slRetVal < 0)
    {
        /* Pay attention, for this function the slRetVal is composed from Signed RetVal & extended error */
        slRetVal = (_i16)slRetVal>> 16;
        ExtendedError = (_u16)slRetVal& 0xFFFF;
        return ExtendedError;
    }

//    gIsWlanConnected = 0;
    tasksReportWlanDisconnected();
    /* Reset */
    sl_Stop(SL_STOP_TIMEOUT);
    gRole = sl_Start(NULL, NULL, NULL);



    return slRetVal;
}

//*****************************************************************************
//
//! \brief  Start Provisioning example
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
int32_t ProvisioningStart(void)
{
    int32_t  retVal = 0;
    int32_t  status = 0;
    uint8_t  configOpt = 0;
    uint16_t configLen = 0;
    uint8_t  simpleLinkMac[SL_MAC_ADDR_LEN];
    uint16_t macAddressLen;
    uint8_t  provisioningCmd;
    SlDeviceVersion_t ver = {0};

    tasksReportWlanDisconnected();
    ConfigureHttpsServer();
    status = GetSecureStatus();
    if ( (SECURED_AP_ENABLE) || (status != SECURED_AP_ENABLE))
    /*Will enter only if previous and present Secure AP are different or Secure AP is 1*/
    {
        SetSecuredAP(SECURED_AP_ENABLE);
        if (ROLE_AP == gRole)
        {
            /* Reset device to start secured AP */
            sl_Stop(SL_STOP_TIMEOUT);
            gRole = sl_Start(NULL, NULL, NULL);
        }
    }

    /* Get device's info */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DeviceGet(SL_DEVICE_GENERAL, &configOpt, &configLen, (uint8_t *)(&ver));

    if(SL_RET_CODE_PROVISIONING_IN_PROGRESS == retVal)
    {
        UART_PRINT(" [ERROR] Provisioning is already running, stopping current session...\r\n");
        SignalEvent(GatewayEvent_ERROR);
        return 0;
    }

    UART_PRINT("\n\r CHIP %d\r\n MAC  31.%d.%d.%d.%d\r\n PHY  %d.%d.%d.%d\r\n NWP  %d.%d.%d.%d\r\n ROM  %d\r\n HOST %d.%d.%d.%d\r\n",
        ver.ChipId,
        ver.FwVersion[0],ver.FwVersion[1],
        ver.FwVersion[2],ver.FwVersion[3],
        ver.PhyVersion[0],ver.PhyVersion[1],
        ver.PhyVersion[2],ver.PhyVersion[3],
        ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
        ver.RomVersion,
        SL_MAJOR_VERSION_NUM,SL_MINOR_VERSION_NUM,SL_VERSION_NUM,SL_SUB_VERSION_NUM);

    macAddressLen = sizeof(simpleLinkMac);
    sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET,NULL,&macAddressLen,(unsigned char *)simpleLinkMac);
    UART_PRINT(" MAC address: %x:%x:%x:%x:%x:%x\r\n\r\n",simpleLinkMac[0],simpleLinkMac[1],simpleLinkMac[2],simpleLinkMac[3],simpleLinkMac[4],simpleLinkMac[5]);

    /* start provisioning */
    provisioningCmd = SL_WLAN_PROVISIONING_CMD_START_MODE_APSC;

    if(provisioningCmd <= SL_WLAN_PROVISIONING_CMD_START_MODE_APSC_EXTERNAL_CONFIGURATION)
    {
        UART_PRINT("\r\n Starting Provisioning! mode=%d (0-AP, 1-SC, 2-AP+SC, 3-AP+SC+WAC)\r\n\r\n", provisioningCmd);
        if(provisioningCmd == 2)
        {
            taskInitComplete();
        }
    }
    else
    {
        UART_PRINT("\r\n Provisioning Command = %d \r\n\r\n",provisioningCmd);
    }



    if( SC_KEY_ENABLE)
    {
        uint8_t key[SL_WLAN_SMART_CONFIG_KEY_LENGTH];

        memcpy(key, (char *)SC_KEY, SL_WLAN_SMART_CONFIG_KEY_LENGTH);
        retVal = sl_WlanProvisioning(provisioningCmd, ROLE_STA, PROVISIONING_INACTIVITY_TIMEOUT,(char *)&key, SL_WLAN_PROVISIONING_CMD_FLAG_EXTERNAL_CONFIRMATION);
    }
    else
    {
        retVal = sl_WlanProvisioning(provisioningCmd, ROLE_STA, PROVISIONING_INACTIVITY_TIMEOUT, NULL, SL_WLAN_PROVISIONING_CMD_FLAG_EXTERNAL_CONFIRMATION);
    }
    if(retVal < 0)
    {
        UART_PRINT(" Provisioning Command Error, num:%d\r\n",retVal);
    }

    return 0;
}

//*****************************************************************************
//
//! \brief  Start the main task, initialize SimpleLink device
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
void provisioning_processQMsg(msgQueue_t *incomingMsg)
{
    unsigned char      event;
    Provisioning_TableEntry_t         *pEntry = NULL;

    event = (unsigned char)incomingMsg->event;
    StopAsyncEvtTimer();

    /* Find Next event entry */
    pEntry = (Provisioning_TableEntry_t *)&gTransitionTable[g_CurrentState][event];

    if (NULL != pEntry->p_evtHndl)
    {
        if (pEntry->p_evtHndl() < 0)
        {
            UART_PRINT("Event handler failed..!! \n\r");
            while(1);
        }
    }

    /* Change state according to event */
    if (pEntry->nextState != g_CurrentState)
    {
        g_CurrentState = pEntry->nextState;
    }

}


//*****************************************************************************
//
//! \brief  Register the Mqueue used for Provisioning
//!
//! \param  provSrvrMqName name for the Mq to be used
//!
//! \return none
//!
//*****************************************************************************
void provisioningInit(const char *provSrvrMqName)
{
    int32_t            iRetVal = 0;
    sigevent           sev;
    pthread_attr_t     timerThreadAttr;
    provSrvrMq = mq_open(provSrvrMqName, O_WRONLY | O_NONBLOCK);

    if (provSrvrMq == NULL)
    {
        UART_PRINT("Prov Message Queue create fail\n\r");
    }

    pthread_attr_init(&timerThreadAttr);
    timerThreadAttr.stacksize = TIMER_TASK_STACK_SIZE;

    /* Create Timer */
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &AsyncEvtTimerHandler;
    sev.sigev_notify_attributes = &timerThreadAttr;

    timer_create(CLOCK_MONOTONIC, &sev, &gTimer);


    /* Initialize Simple Link */
    if  ((iRetVal = sl_Start(NULL, NULL, (P_INIT_CALLBACK)SimpleLinkInitCallback)) < 0)
    {
        UART_PRINT("sl_Start Failed\r\n");
        if (iRetVal == SL_ERROR_RESTORE_IMAGE_COMPLETE)
        {
            UART_PRINT("\r\n**********************************\r\nReturn to Factory Default has been Completed\r\nPlease RESET the Board\r\n**********************************\r\n");
            gLedDisplayState = LedState_FACTORY_DEFAULT;
        }
        while(1);
    }
}

//*****************************************************************************
//
//! \brief  Reports to all other tasks that the Wlan is disconnected
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void tasksReportWlanDisconnected(void)
{
    msgQueue_t mqEvt = {CommonEvent_WLAN_DISCONNECTED, NULL, 0};
//    if(gIsWlanConnected)
//    {
//        UART_PRINT("\n\r [Provisioning] Wlan Disconnected! Restarting SLink Task\n\r");
//        sl_Stop(SL_STOP_TIMEOUT);
//        sl_Start(NULL, NULL, (P_INIT_CALLBACK)SimpleLinkInitCallback);
//    }
    gIsWlanConnected = 0;
    mq_send(provSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}

//*****************************************************************************
//
//! \brief  Report to other tasks that the gateway and provissioning initialization has completed
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void taskInitComplete(void)
{
    msgQueue_t mqEvt = {GatewayEvent_INIT_COMPLETE, NULL, 0};
    mq_send(provSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
}
//*****************************************************************************
//
//! \brief  Reports to all other tasks that the Wlan is connected
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void tasksReportWlanConnected(void)
{
    if(!gIsWlanConnected)
    {
        msgQueue_t mqEvt = {CommonEvent_WLAN_CONNECTED, NULL, 0};
        gIsWlanConnected = 1;
        mq_send(provSrvrMq, (char*)&mqEvt, sizeof(msgQueue_t), MQ_LOW_PRIOR);
    }
}
