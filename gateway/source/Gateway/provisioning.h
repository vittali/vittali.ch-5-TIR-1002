/*
 * provisioning.h
 *
 *  Created on: Feb 6, 2017
 *      Author: a0224683
 */

#ifndef GATEWAY_PROVISIONING_H_
#define GATEWAY_PROVISIONING_H_
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

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
extern void provisioningInit(const char *provSrvrMqName);

/*!
 * @brief
 *
 *
 * @param
 *
 * @return
 */
void provisioning_processQMsg(msgQueue_t *incomingMsg);


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* COLLECTOR_COLLECTOR_H_ */
