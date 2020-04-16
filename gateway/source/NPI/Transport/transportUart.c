/*
 * transportUart.c
 *
 *  Created on: Feb 13, 2017
 *      Author: a0224683
 */

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>

 #include <Common/commonDefs.h>

 #include "mqueue.h"

// TI-Driver includes
#include <ti/drivers/UART.h>
#include "Board.h"


static UART_Handle uart;
static mqd_t *readMq = NULL;

/*********************************************************************
 * API FUNCTIONS
 */
static void transportReadCb(UART_Handle handle, void *buf, size_t count);

/*********************************************************************
 * @fn      transportOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
uint8_t transportOpen(void *port)
{
	UART_Params uartParams;
	int32_t ret = -1;
	Board_initUART();

	//if (uart == NULL)
	{
		/* Create a UART with data processing off. */
		UART_Params_init(&uartParams);
		uartParams.readMode = UART_MODE_CALLBACK;
		uartParams.writeMode = UART_MODE_BLOCKING;
		uartParams.readTimeout = BIOS_WAIT_FOREVER;
		uartParams.writeTimeout = BIOS_WAIT_FOREVER;
		uartParams.readCallback = transportReadCb;
		uartParams.writeCallback = NULL;
		uartParams.readReturnMode = UART_RETURN_FULL;
		uartParams.writeDataMode = UART_DATA_BINARY;
		uartParams.readDataMode = UART_DATA_BINARY;
		uartParams.readEcho = UART_ECHO_OFF;
		uartParams.baudRate = 115200;
		uartParams.dataLength = UART_LEN_8;
		uartParams.stopBits = UART_STOP_ONE;
		uartParams.parityType = UART_PAR_NONE;
		/*uartParams.writeDataMode    = UART_DATA_BINARY;
		    uartParams.readDataMode     = UART_DATA_BINARY;
		    uartParams.readReturnMode   = UART_RETURN_FULL;
		    uartParams.readEcho         = UART_ECHO_OFF;
		    uartParams.baudRate         = 115200;*/

		// init UART driver
		uart = UART_open(Board_UART1, &uartParams);
		if (uart != NULL)
		{
			// return success
			ret = 0;
		}
	}

	return ret;
}

void transportRegisterMq(mqd_t *mqHandle)
{
    readMq = mqHandle;
}

/*********************************************************************
 * @fn      transportClose
 *
 * @brief   closes the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void transportClose(void)
{
	if (uart != NULL)
	{
		// call TI-RTOS driver
		UART_close(uart);
	}

	return;
}

/*********************************************************************
 * @fn      transportWrite
 *
 * @brief   Write to the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
int32_t transportWrite(uint8_t* buf, uint8_t len)
{
    int32_t writenBytes = 0;
	if (uart != NULL)
	{
	    //TODO: check
		// call TI-RTOS driver function
	    writenBytes = UART_write(uart, (void*) buf, (size_t) len);
	}

	return writenBytes;
}

/*!
 * @brief   Reads from the the serial port.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
int32_t transportRead(uint8_t* buf, uint8_t len)
{
	int32_t ret = 0;
	int32_t bytes;

	if (uart != NULL)
	{
		// call TI-RTOS driver function
		bytes = UART_read(uart, (void*) buf, (size_t) len);
		if (bytes != UART_ERROR)
		{
			// return number of read bytes
			ret = bytes;
		}
	}

	return ret;
}

/*!
 * @brief   Callback after UART read has completed a read.
 *
 * @param   handle - UART Handle
 *
 * @param   buf - pointer to read buffer
 *
 * @param   count - Number of elements read
 *
 * @return  void
 */
void transportReadCb(UART_Handle handle, void *buf, size_t count)
{
    msgQueue_t npiReportReadMq;
    npiReportReadMq.event = NPIEvent_TRANSPRT_RX;
    npiReportReadMq.msgPtr = buf;
    npiReportReadMq.msgPtrLen = (int32_t)count;
    //incoming messages should have a higher priority
    // so NPI handles them before trying to send anything back to the CoP
    // TODO: Check for mq return value in case queue is full
    mq_send(*readMq, (char*)&npiReportReadMq, sizeof(msgQueue_t), MQ_HIGH_PRIOR);
}
