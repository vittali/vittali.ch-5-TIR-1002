/******************************************************************************

 @file comTransport.h

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

#ifndef NPI_TRANSPORT_COMTRANSPORT_H_
#define NPI_TRANSPORT_COMTRANSPORT_H_
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

//#define NPI_USE_UART
//#define xNPI_USE_SPI

typedef struct
{
    void* portName;     // this is a void pointer so the same attribute definition can be used in different platforms
	uint16_t mode;         /* BLOCKING, NON_BLOCKING, ...*/
	uint32_t readTimeout; // read timeout for calls in blocking mode
} transportAttrs_t;



uint8_t transportOpen(void *port);
void transportClose(void);
int32_t transportWrite(uint8_t* buf, uint8_t len);
int32_t transportRead(uint8_t* buf, uint8_t len);
// register a message queue so transport can report back the data it read from a non_blocking read
void transportRegisterMq(mqd_t *mqHandle);



//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif
#endif /* NPI_TRANSPORT_COMTRANSPORT_H_ */
