/*
 * Standard Library Includes
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 * AVR Includes (Defines and Primitives)
 */

#include <avr/io.h>
 
/*
 * AVR Library Includes
 */

#include "lib_uart.h"
 
/*
 * Local Application Includes
 */

#include "comms.h"

/*
 * Protocol Library Includes
 */

#include "llap.h"
 
/*
 * Utility Library Includes
 */

#include "util_memory_placement.h"

/*
 * Local Application Includes
 */
 
#include "latrinesensor.h"

/* 
 * Private Variables
 */

static char txrxBuffer[13]; // Use same buffer for transmit and receive

static uint8_t rxIndex;

static LLAP_DEVICE llapDevice;
static char * deviceName = "PitSensor";
static char * deviceType = "U00000001";
static char * fwVersion = "4.0";
static char * serialNum = "PROTO01";

/*
 * Private Function Prototypes
 */

static void uartCheck(void);
static void llapGenericHandler(LLAP_GENERIC_MSG_ENUM eMsgType, const char * genericStr, const char * msgBody);
static void llapApplicationHandler(const char * msgBody);
static void llapSendRequest(const char * msgBody);

/*
 * Public Function Defintions
 */

void COMMS_Init(void)
{
	rxIndex = 0;
	
	UART_Init(UART0, 4800, 14, 14, false);
	
	// Setup the LLAP device
	llapDevice.id[0] = '-';
	llapDevice.id[1] = '-';	
	llapDevice.devName = deviceName;
	llapDevice.devType = deviceType;
	llapDevice.fwVer = fwVersion;
	llapDevice.serNum = serialNum;
	llapDevice.msgBuffer = txrxBuffer;
	
	// Link LLAP object to local handler functions
	llapDevice.genericMsgHandler = llapGenericHandler;
	llapDevice.applicationMsgHandler = llapApplicationHandler;
	llapDevice.sendRequest = llapSendRequest;
	LLAP_StartDevice(&llapDevice);
}

void COMMS_Send(char * s)
{
	LLAP_SendOutgoingMessage(&llapDevice, s);
}

void COMMS_Check(void)
{ 
	uartCheck();
}

static void uartCheck(void)
{
	bool rx = false;
	bool tx = false;
	UART0_Task(&rx, &tx);
	
	if (rx)
	{
		txrxBuffer[rxIndex] = UART_GetChar(UART0, NULL);
				
		if (strlen(txrxBuffer) == LLAP_MESSAGE_LENGTH)
		{
			LLAP_HandleIncomingMessage(&llapDevice, txrxBuffer);
			rxIndex = 0;
		}
	}
}

static void llapGenericHandler(LLAP_GENERIC_MSG_ENUM eMsgType, const char * genericStr, const char * msgBody)
{
	(void)eMsgType;
	(void)genericStr;
	(void)msgBody;
}

static void llapApplicationHandler(const char * msgBody)
{
	if ((msgBody[0] == 'T') && (msgBody[1] == 'H'))
	{
		APP_HandleNewThresholdSetting(&msgBody[2]);
	}
}

static void llapSendRequest(const char * msgBody)
{
	UART_PutStr(UART0, (uint8_t*)msgBody);
}

