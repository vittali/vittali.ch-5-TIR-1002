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
 *
 *
 *  Created on: Feb 16, 2017
 *      Author: a0226081
 *
 *
 */

/*****************************************************************************

 Application Name     -   MQTT Client
 Application Overview -   The device is running a MQTT client which is
                           connected to the online broker. The Red LED on the
                           device can be controlled from a web client by
                           publishing msg on appropriate topics. Similarly,
                           message are being published on pre-configured topics
                           every second.

 Application Details  - Refer to 'MQTT Client' README.html

 *****************************************************************************/
//*****************************************************************************
//
//! \addtogroup mqtt_server
//! @{
//
//*****************************************************************************
/* Standard includes                                                          */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Hardware includes                                                          */
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>

/* Driverlib includes                                                         */
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/timer.h>

/* TI-Driver includes                                                         */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

/* Simplelink includes                                                        */
#include <ti/drivers/net/wifi/simplelink.h>

/* Common interface includes                                                  */

#include <Utils/uart_term.h>
#include <Common/commonDefs.h>

/* Application includes                                                       */
#include <Board.h>
#include <pthread.h>
#include <mqueue.h>
#include <time.h>
#include <unistd.h>
#if defined(USE_IBM_CLOUD)
#include "IBM/cloudServiceIBM.h"
#elif defined(USE_AWS_CLOUD)
#include "AWS/cloudServiceAWS.h"
#endif
#include "LocalWebSrvr/localWebSrvr.h"
#include "cloud_service.h"

//*****************************************************************************
//                          LOCAL DEFINES
//*****************************************************************************


#define MQTT_INIT_STATE         (0x04)


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************

void *cloudService_thread(void *pvParameters);



//*****************************************************************************
//                 GLOBAL VARIABLES
//*****************************************************************************



/* Receive task handle                                                        */

#define CLOUDTHREADSIZE          4096


/* Message Queue                                                              */
mqd_t cloudSrvrMq;
mqd_t registeredMq;
pthread_t cloudServiceThreadHandle = (pthread_t) NULL;


void *cloudService_thread(void *pvParameters)
{
    msgQueue_t queueElemRecv;

    for (;;)
    {
        queueElemRecv.event = CommonEvent_INVALID_EVENT;
        queueElemRecv.msgPtr = NULL;
        queueElemRecv.msgPtrLen = 0;
        /* waiting for signals                                                */
        mq_receive(cloudSrvrMq, (char*) &queueElemRecv, sizeof(msgQueue_t), NULL);

        switch (queueElemRecv.event)
        {
        case CommonEvent_WLAN_DISCONNECTED:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleWlanDisconnect();
#elif defined(USE_AWS_CLOUD)
            CloudAWS_handleWlanDisconnect();
#endif
            LocalWebSrvr_handleWlanDisconnect();
            break;
        case CommonEvent_WLAN_CONNECTED:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleWlanConnect();
#elif defined(USE_AWS_CLOUD)
            CloudAWS_handleWlanConnect();
#endif
            LocalWebSrvr_handleWlanConnect();
            break;
        case CommonEvent_CLOUD_SERV_DISCONNECTED:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleCloudDisconnect();
#elif defined(USE_AWS_CLOUD)
            CloudAWS_handleCloudDisconnect();
#endif
            LocalWebSrvr_handleCloudDisconnect();
            break;
        case CommonEvent_CLOUD_SERV_CONNECTED:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleCloudConnect();
#elif defined(USE_AWS_CLOUD)
//For future feature handling AWS cloud connection from local fEnd
#endif
            LocalWebSrvr_handleCloudConnect();
            break;
        case CloudServiceEvt_NWK_UPDATE:
        case CloudServiceEvt_DEV_UPDATE:
        case CloudServiceEvt_STATE_CNF_EVT:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleGatewayEvt(&queueElemRecv);
#elif defined(USE_AWS_CLOUD)
            CloudAWS_handleGatewayEvt(&queueElemRecv);
#endif
            LocalWebSrvr_handleGatewayEvt(&queueElemRecv);
           break;
        case CloudServiceEvt_CLOUD_IN_MQTT:
#if defined(USE_IBM_CLOUD)
            CloudIBM_handleCloudEvt(&queueElemRecv);
#elif defined(USE_AWS_CLOUD)
            CloudAWS_handleCloudEvt(&queueElemRecv);
#endif
            break;
        case CloudServiceEvt_LOCAL_SERVR_HTTP:
            LocalWebSrvr_handleHttpEvt(&queueElemRecv);
            break;
        case CloudServiceEvt_CLOUDINFO_RX:
#if defined(USE_IBM_CLOUD)
            cloudIBM_registerCloudInfo((CloudIBM_Info_t*)queueElemRecv.msgPtr);
            cloudIBM_start();
#elif defined(USE_AWS_CLOUD)
            cloudAWS_start();
#endif
            queueElemRecv.msgPtr = NULL; // this is to bypass freeing this pointer since we need the info in IBM cloud service
            break;
        default:
            break;
        }
        if(queueElemRecv.msgPtr)
        {
            free(queueElemRecv.msgPtr);
        }
    }
}

void cloudServiceCliMqReg(const char *cservClientMq)
{
    registeredMq = mq_open(cservClientMq, O_WRONLY | O_NONBLOCK);
#if defined(USE_IBM_CLOUD)
    cloudIBM_registerCliMq(&registeredMq);
#elif defined(USE_AWS_CLOUD)
    cloudAWS_registerCliMq(&registeredMq);
#endif
    LocalWebSrvr_registerCliMq(&registeredMq);

}
//*****************************************************************************
//
//!  MQTT Start
//!
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************
void cloudServiceInit(const char *cloudSrvrMqName)
{
    pthread_attr_t pAttrs;
    struct sched_param priParam;
    int32_t retc = 0;
    mq_attr attr;
    unsigned mode = 0;

    /* sync object for inter thread communication                             */
    attr.mq_maxmsg = 20;
    attr.mq_msgsize = sizeof(msgQueue_t);
    cloudSrvrMq = mq_open(cloudSrvrMqName, O_CREAT, mode, &attr);


    if (cloudSrvrMq == NULL)
    {
        UART_PRINT("[Cloud Service] MQTT Message Queue create fail\n\r");
        return;
    }

    /* Set priority and stack size attributes                                 */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = CLOUDSRV_TASK_PRI;
    retc = pthread_attr_setschedparam(&pAttrs, &priParam);
    retc |= pthread_attr_setstacksize(&pAttrs, CLOUDTHREADSIZE);
    retc |= pthread_attr_setdetachstate(&pAttrs, PTHREAD_CREATE_DETACHED);
    retc |= pthread_create(&cloudServiceThreadHandle, &pAttrs, cloudService_thread, NULL);
    if (retc != 0)
    {
        UART_PRINT("[Cloud Service] MQTT thread create fail\n\r");
    }
#if defined(USE_IBM_CLOUD)
    cloudIBM_init(cloudSrvrMqName);
#elif defined(USE_AWS_CLOUD)
    cloudAWS_init(cloudSrvrMqName);
#endif
    LocalWebSrvr_init(cloudSrvrMqName);
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
