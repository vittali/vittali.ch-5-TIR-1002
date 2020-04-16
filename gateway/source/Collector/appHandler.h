/******************************************************************************

 @file appHandler.h

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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/******************************************************************************
 Typedefs
 *****************************************************************************/

/******************************************************************************
 Function Prototypes
 *****************************************************************************/
void appCliMqReg(mqd_t *pAppCliMq);


/*
 * Main application function.
 */
void APP_main(const char *appSrvMqName);

/*!
 * @brief  Csf module calls this function to initialize the application server
 *         interface
 *
 *
 * @param
 *
 * @return
 */
 void appsrv_Init(void *param);

/*!
 * @brief        Csf module calls this function to inform the applicaiton client
 *                     that the application has either started/restored the network
 *
 * @param
 *
 * @return
 */
 void appsrv_networkUpdate(bool restored, Llc_netInfo_t *networkInfo);//DONE

/*!
 * @brief        Csf module calls this function to inform the applicaiton clientr
 *                     that a device has joined the network
 *
 * @param
 *
 * @return
 */
 void appsrv_deviceUpdate(Llc_deviceListItem_t *pDevInfo);//DONE

 /*!
 * @brief        Csf module calls this function to inform the applicaiton client
 *                     that the device has responded to the configuration request
 *
 * @param
 *
 * @return
 */
 void appsrv_deviceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_configRspMsg_t *pMsg);

 /*!
 * @brief        Csf module calls this function to inform the applicaiton client
 *                     that a device is no longer active in the network
 *
 * @param
 *
 * @return
 */
 void appsrv_deviceNotActiveUpdate(ApiMac_deviceDescriptor_t *pDevInfo,
                                      bool timeout);
  /*!
 * @brief        Csf module calls this function to inform the applicaiton client
                         of the reported sensor data from a network device
 *
 * @param
 *
 * @return
 */
 void appsrv_deviceSensorDataUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                       Smsgs_sensorMsg_t *pMsg);

/*!
 * @brief        TBD
 *
 * @param
 *
 * @return
 */
 void appsrv_stateChangeUpdate(Cllc_states_t state);

