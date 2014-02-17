/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * AVR Includes (Defines and Primitives)
 */
#include "avr/io.h"
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

/*
 * AVR Library Includes
 */

#include "lib_io.h"
#include "lib_wdt.h"
#include "lib_sleep.h"
#include "lib_pcint.h"

#include "lib_clk.h"
#include "lib_tmr_common.h"
#include "lib_tmr8.h"
#include "lib_tmr8_tick.h"

/*
 * Generic Library Includes
 */

#include "statemachine.h"
#include "statemachinemanager.h" 

/*
 * Local Application Includes
 */

#include "app_test_harness.h"
#include "app_statemachine.h"
#include "tempsense.h"
#include "ircounter.h"
#include "comms.h"

/*
 * Defines and typedefs
 */

// Make sure these defines are kept in sync or bad things will happen.
#define IDLE_WDT_TIME_SECS		(2)
#define IDLE_WDT_TIME_SELECT	(WDTO_2S)

#define ACTIVE_WDT_TIME_MS		(15)
#define ACTIVE_WDT_TIME_SELECT	(WDTO_15MS)

#define SHORT_IR_DELAY_US			(200)

#define IR_OUTFLOW_VECTOR			PCINT1_vect // TODO: Set correct vector
#define IR_OUTFLOW_PCINT_NUMBER		10			// TODO: Set correct number

#define eIR_OUTFLOW_LED_PORT		IO_PORTB
#define IR_OUTFLOW_LED_PORT			PORTB
#define IR_OUTFLOW_LED_PINS			PINB
#define IR_OUTFLOW_LED_PIN			0

#define eIR_LEVEL_LED_PORT			IO_PORTB
#define IR_LEVEL_LED_PINS			PINB
#define IR_LEVEL_LED_PORT			PORTB
#define IR_LEVEL_LED_PIN			1

#define TEST_LED_ON					IO_On(IR_LEVEL_LED_PORT, IR_LEVEL_LED_PIN)
#define TEST_LED_OFF				IO_Off(IR_LEVEL_LED_PORT, IR_LEVEL_LED_PIN)
#define	TEST_LED_TOGGLE				IO_Toggle(IR_LEVEL_LED_PINS, IR_LEVEL_LED_PIN)

#define TEST_TOGGLE(x) 				for (uint8_t i=0; i < x; ++i) { TEST_LED_TOGGLE; }

enum level_test_mode_enum
{
	LVL_MODE_IR, // Test for full level is with a IR based system
	LVL_MODE_SW // Test for full level is with an switch based system
};
typedef enum level_test_mode_enum LEVEL_TEST_MODE_ENUM;

/*
 * Private Function Prototypes
 */

static void setupTimers(void);
static void setupIO(void);

static void testForDetection(void);
static void sleepUntilInterrupt(void);

/*
 * Main state machine pointers
 */

/* 
 * Private Variables
 */

static int8_t smIndex;

static LEVEL_TEST_MODE_ENUM elevelTestMode = LVL_MODE_SW;

static WDT_SLEEP_TICK idleTick;
static WDT_SLEEP_TICK activeTick;

static bool irTriggered;

int main(void)
{
	DO_TEST_HARNESS_SETUP();
	
	WD_DISABLE();
	
	smIndex = APPSM_SetupStateMachine();

	setupIO();
	setupTimers();
	
	//TS_Setup();
	IR_Reset(IR_OUTFLOW);
	IR_Reset(IR_LEVEL);
	
	COMMS_Init(CT_UART);
		
	sei();
	
	while (true)
	{
		DO_TEST_HARNESS_RUNNING();

		TS_Check();
		
		if (WDT_TestAndClear(&idleTick))
		{
			// Kicks the application out of idle
			TEST_LED_TOGGLE;
			TS_AmbientTimerTick(IDLE_WDT_TIME_SECS);
			SM_Event(smIndex, TIMER);
		}
		
		if (WDT_TestAndClear(&activeTick))
		{
			TS_OutflowTimerTick(ACTIVE_WDT_TIME_MS);
			
			// Briefly turn IR LED on, then off. 
			IO_On(IR_OUTFLOW_LED_PINS, IR_OUTFLOW_LED_PIN);
			_delay_us(SHORT_IR_DELAY_US);
			IO_Off(IR_OUTFLOW_LED_PINS, IR_OUTFLOW_LED_PIN);
			// Check if the pulse was registered
			testForDetection();
		}

		if ( TS_IsTimeForOutflowRead() )
		{
			TS_StartConversion(SENSOR_OUTFLOW);
		}
		else if ( TS_IsTimeForAmbientRead() )
		{
			TS_StartConversion(SENSOR_AMBIENT);
		}

		// TODO: Receive comms check
		
		sleepUntilInterrupt();
	}
}

static void sleepUntilInterrupt(void)
{
	/* Decide how deep the sleep should be.
	Depends on which state application is in 
	and if ADC is converting */
	
	if ( TS_ConversionStarted() )
	{
		SLEEP_Sleep(SLEEP_MODE_ADC, false);
	}
	else
	{
		if ( (STATES)SM_GetState(smIndex) != IDLE )
		{
			// Sleep for long idle tick, turn off everything except the watchdog timer
			WDT_Sleep(&idleTick, SLEEP_MODE_PWR_SAVE, true);
			WD_DISABLE();
		}
		else
		{
			// Sleep for short active tick, turn off everything except the watchdog timer
			WDT_Sleep(&activeTick, SLEEP_MODE_PWR_SAVE, true);
			WD_DISABLE();
		}
	}
}

static void setupIO(void)
{
	IO_SetMode(eIR_LEVEL_LED_PORT, IR_LEVEL_LED_PIN, IO_MODE_OUTPUT);
	IO_SetMode(eIR_OUTFLOW_LED_PORT, IR_OUTFLOW_LED_PIN, IO_MODE_OUTPUT);
}

static void setupTimers(void)
{
	idleTick.time = IDLE_WDT_TIME_SELECT;
	activeTick.time = ACTIVE_WDT_TIME_SELECT;
}

void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	TEST_LED_ON;
	PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, true);
	IR_Reset(IR_OUTFLOW);
}

void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	switch(elevelTestMode)
	{
	case LVL_MODE_IR:
		PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, false);
		IR_Reset(IR_LEVEL);
		break;
	case LVL_MODE_SW:
		// TODO: read level sense switch and send event
		break;
	}
}

void testPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	SM_Event(smIndex, IR_SensorHasTriggered(IR_LEVEL) ?  PIT_FULL : PIT_NOT_FULL);
}

static void testForDetection(void)
{
	bool countingStopped = IR_UpdateCount(IR_OUTFLOW, irTriggered ? 1 : 0);
	irTriggered = false;
	
	if (countingStopped)
	{
		// Nothing was detected this time, so send a detect/no detect event 
		// final result to the state machine
		TEST_LED_OFF;
		PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, false);
		SM_Event(smIndex, IR_SensorHasTriggered(IR_OUTFLOW) ? DETECT: NO_DETECT);
	}
}

void sendPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	COMMS_Send("PITFULL");
}

void sendData(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	uint8_t detectDurationSecs = (IR_GetOutflowSenseDurationMs() + 500U) / 1000U;
	
	char message[] = "aAA25000----";

	// temperature to string conversion (outflow):
	message[3] = 0; //TODO 
	message[4] = 0; //TODO 
	
	// temperature to string conversion (ambient):
	message[5] = 0; //TODO 
	message[6] = 0; //TODO 
	
	// uint8_t duration to string conversion:
	message[7] = detectDurationSecs / 100U;
	detectDurationSecs -= (message[7] * 100U);
	message[8] = detectDurationSecs / 10U;
	detectDurationSecs -= (message[8] * 10U);
	message[9] = detectDurationSecs;
	
	message[3] += '0';
	message[4] += '0';
	message[5] += '0';
	message[6] += '0';
	message[7] += '0';
	message[8] += '0';
	message[9] += '0';
	
	COMMS_Send(message);
	SM_Event(smIndex, SEND_COMPLETE);
}

ISR(IR_OUTFLOW_VECTOR)
{
	irTriggered = true;
}
