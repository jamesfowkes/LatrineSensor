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

#if defined(UART)
	#include "lib_uart.h"
#elif defined(SWS)
	#include "lib_io.h"
	#include "lib_swserial.h"
#else
#error "UART_OPTION must be specified (swserial or uart)!"
#endif
/*
 * Protocol Library Includes
 */

#include "llap.h"
 
/*
 * Local Application Includes
 */

#include "comms.h"

/* 
 * Private Variables
 */

static LLAP_DEVICE llapDevice;
static COMMS_TYPE eCommsType;

static char deviceName[] = "PITSENSOR";
static char deviceType[] = "U00000001";
static char fwVersion[] = "1.00";
static char serialNum[] = "000000";

static char rxBuffer[13];
static char txBuffer[13];
static uint8_t rxIndex;

#if defined(SWS)
static bool s_bSWSerialInterrupt;
#endif

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

void COMMS_Init(COMMS_TYPE eType)
{
	eCommsType = eType;
	rxIndex = 0;
	s_bSWSerialInterrupt = false;
	
	LLAP_Init();
	
	// Setup the LLAP device
	llapDevice.id[0] = '-';
	llapDevice.id[1] = '-';	
	llapDevice.devName = deviceName;
	llapDevice.devType = deviceType;
	llapDevice.fwVer = fwVersion;
	llapDevice.serNum = serialNum;
	llapDevice.msgBuffer = txBuffer;
	
	// Link LLAP object to local handler functions
	llapDevice.genericMsgHandler = llapGenericHandler;
	llapDevice.applicationMsgHandler = llapApplicationHandler;
	llapDevice.sendRequest = llapSendRequest;
	
	LLAP_StartDevice(&llapDevice);
	
	switch (eCommsType)
	{
	case CT_UART:
		#if defined(UART)
		UART_Init(UART0, 4800, 14, 14, false);
		#elif defined(SWS)
		SWS_TxInit(IO_PORTA, 0); // TODO: set actual port/pin
		#endif
		break;
	default:
	//case CT_SPI: // Not supported
		break;
	}
}

void COMMS_Send(char * s)
{
	LLAP_SendOutgoingMessage(&llapDevice, s);
}

void COMMS_Check(void)
{ 
	switch (eCommsType)
	{
	case CT_UART:
		uartCheck();
		break;
	default:
	//case CT_SPI: // Not supported
		break;
	}
}

static void uartCheck(void)
{
	bool rx = false;
	
	#if defined(UART)
	bool tx = false;
	UART0_Task(&rx, &tx);
	#elif defined(SWS)
	rx = s_bSWSerialInterrupt;
	#endif
	
	if (rx)
	{
		#if defined(UART)
		rxBuffer[rxIndex] = UART_GetChar(UART0, NULL);
		#elif defined(SWS)
		(void)SWS_Receive(rxBuffer, sizeof(rxBuffer), true);
		#endif
		
		if (strlen(rxBuffer) == LLAP_MESSAGE_LENGTH)
		{
			LLAP_HandleIncomingMessage(&llapDevice, rxBuffer);
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
	(void)msgBody;
}

static void llapSendRequest(const char * msgBody)
{
	switch (eCommsType)
	{
	case CT_UART:
		#if defined(UART)
		UART_PutStr(UART0, (uint8_t*)msgBody);
		#elif defined(SWS)
		SWS_Transmit(msgBody, strlen(msgBody), NULL, 0);
		#endif
		break;
	default:
	//case CT_SPI: // Not supported
		break;
	}
}