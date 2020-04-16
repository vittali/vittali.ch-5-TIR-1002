/******************************************************************************

 @file cloudServiceIBM.h

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
#ifndef CLOUDSERVICE_IBM_CLOUDSERVICEIBM_H_
#define CLOUDSERVICE_IBM_CLOUDSERVICEIBM_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    /*! organization id*/
    char *orgId;
    /*! device Type*/
    char *devType;
    /*! Id of the device*/
    char *devId;
    /*!*/
    char *securityToken;
} CloudIBM_Info_t;

struct publishMsgHeader
{
    /*!*/
    uint32_t        topicLen;
    /*!*/
    uint32_t        payLen;
    /*!*/
    bool            retain;
    /*!*/
    bool            dup;
    /*!*/
    unsigned char   qos;
};

/*!
 * @brief initialize mqtt connection to IBM service
 *
 *
 * @param
 *
 * @return
 */
extern void cloudIBM_init(const char *cloudSrvrMqName);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void cloudIBM_registerCliMq(mqd_t *pGatewayMq);

/*!
 * @brief   Register cloud connection info
 *
 *
 * @param
 *
 * @return
 */
extern void cloudIBM_registerCloudInfo(CloudIBM_Info_t *cInfo);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void cloudIBM_start( void );

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleWlanDisconnect(void);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleWlanConnect(void);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleCloudDisconnect(void);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleCloudConnect(void);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleGatewayEvt(msgQueue_t *inEvtMsg);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void CloudIBM_handleCloudEvt(msgQueue_t *inEvtMsg);


#ifdef __cplusplus
}
#endif
#endif /* CLOUDSERVICE_IBM_CLOUDSERVICEIBM_H_ */
