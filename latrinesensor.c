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
#include "avr/wdt.h"
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

/*
 * AVR Library Includes
 */

#include "lib_io.h"
#include "lib_pcint.h"

#include "lib_clk.h"
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

#define FAST_DETECT_TICK_MS			(100)
#define SLOW_IDLE_TICK_MS 			(2500)
#define LEVEL_TEST_TICK_MS			(1000)

#define AMBIENT_ADC_TICK_MS			(30000)

#define IR_OUTFLOW_VECTOR			PCINT0_vect // TODO: Set correct vector
#define IR_OUTFLOW_PCINT_NUMBER		0			// TODO: Set correct number

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

/*
 * Main state machine pointers
 */

/* 
 * Private Variables
 */

static uint8_t smIndex;

static LEVEL_TEST_MODE_ENUM elevelTestMode = LVL_MODE_SW;

static TMR8_TICK_CONFIG applicationTick;
static TMR8_TICK_CONFIG ambientADCTick;

static uint16_t currentIRCount;

int main(void)
{
	DO_TEST_HARNESS_SETUP();
	
	/* Disable watchdog: not required for this application */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	
	smIndex = APPSM_SetupStateMachine();
	setupTimers();
	
	TS_Setup();
	IR_Reset(IR_OUTFLOW);
	IR_Reset(IR_LEVEL);
	
	COMMS_Init(CT_UART);
	
	PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, true);
		
	sei();
	
	while (true)
	{
	
		DO_TEST_HARNESS_RUNNING();
		
		if (TMR8_Tick_TestAndClear(&applicationTick))
		{
			SM_Event(smIndex, TIMER);
		}
		
		if (TMR8_Tick_TestAndClear(&ambientADCTick))
		{
			TS_StartConversion(SENSOR_AMBIENT);
		}
	
		COMMS_Check();
		
		TS_Check();
				
		//TODO: sleepUntilInterrupt();
	}
}

static void setupTimers(void)
{
	TMR8_Tick_Init(2, 0);
	
	applicationTick.reload = SLOW_IDLE_TICK_MS;
	applicationTick.active = true;
	TMR8_Tick_AddTimerConfig(&applicationTick);
	
	ambientADCTick.reload = AMBIENT_ADC_TICK_MS;
	ambientADCTick.active = true;
	TMR8_Tick_AddTimerConfig(&ambientADCTick);
}

void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	//TODO: enable PCINT interrupt
	IR_Reset(IR_OUTFLOW);
	TMR8_Tick_SetNewReloadValue(&applicationTick, FAST_DETECT_TICK_MS);
	TS_StartConversion(SENSOR_OUTFLOW);
}

void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	switch(elevelTestMode)
	{
	case LVL_MODE_IR:
		//TODO: enable IR output
		PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, false);
		TMR8_Tick_SetNewReloadValue(&applicationTick, LEVEL_TEST_TICK_MS);
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

void stopCounting(void)
{
	//TODO: Disable IR output
	PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, false);
	TMR8_Tick_SetNewReloadValue(&applicationTick, SLOW_IDLE_TICK_MS);
	SM_Event(smIndex, IR_SensorHasTriggered(IR_OUTFLOW) ? DETECT: NO_DETECT);
}

void testCount(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	bool countingStopped = IR_UpdateCount(IR_OUTFLOW, currentIRCount);
	currentIRCount = 0;
	
	if (countingStopped)
	{
		// Nothing was detected this time, so send a detect/no detect event 
		// final result to the state machine
		stopCounting();
	}
	else
	{
		// Something was detected, so keep counting and monitoring outflow temperature
		TS_StartConversion(SENSOR_OUTFLOW);
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
	
	uint16_t detectDurationMs = IR_GetOutflowSenseDurationMs();
	char message[] = "FLSH00000";
	
	// uint16_t to string conversion:
	message[4] = detectDurationMs / 10000;
	detectDurationMs -= (message[4] * 10000);
	message[5] = detectDurationMs / 1000;
	detectDurationMs -= (message[5] * 1000);
	message[6] = detectDurationMs / 100;
	detectDurationMs -= (message[6] * 100);
	message[7] = detectDurationMs / 10;
	detectDurationMs -= (message[7] * 10);
	message[8] = detectDurationMs;
	
	message[4] += '0';
	message[5] += '0';
	message[6] += '0';
	message[7] += '0';
	message[8] += '0';
	
	COMMS_Send(message);
}

ISR(IR_OUTFLOW_VECTOR)
{
	currentIRCount++;
}
