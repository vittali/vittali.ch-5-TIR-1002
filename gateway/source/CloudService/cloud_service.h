/*
 *   Copyright (C) 2016 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
 *
 *
 *  Created on: Feb 16, 2017
 *      Author: a0226081
 *
 *
 */



#ifndef __CLOUD_SERVICE_H__
#define __CLOUD_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void cloudServiceInit(const char *cloudMqName);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void cloudServiceCliMqReg(const char *cservClientMq);


#ifdef __cplusplus
}
#endif

#endif // __CLOUD_SERVICE_H__
