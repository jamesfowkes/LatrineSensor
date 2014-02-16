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

#define FAST_DETECT_TICK_MS			(100)
#define SLOW_IDLE_TICK_MS 			(2500)
#define LEVEL_TEST_TICK_MS			(1000)
#define AMBIENT_ADC_TICK_MS			(30000)
#define IR_PULSE_TIME_MS			(5)

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

/*
 * Main state machine pointers
 */

/* 
 * Private Variables
 */

static int8_t smIndex;

static LEVEL_TEST_MODE_ENUM elevelTestMode = LVL_MODE_SW;

static TMR8_TICK_CONFIG applicationTick;
static TMR8_TICK_CONFIG ambientADCTick;
static TMR8_TICK_CONFIG irPulseTick;

static uint16_t currentIRCount;

int main(void)
{
	DO_TEST_HARNESS_SETUP();
	
	/* Disable watchdog: not required for this application */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	
	smIndex = APPSM_SetupStateMachine();

	setupIO();
	setupTimers();
	
	//TS_Setup();
	//IR_Reset(IR_OUTFLOW);
	//IR_Reset(IR_LEVEL);
	
	COMMS_Init(CT_UART);
		
	sei();
	
	while (true)
	{
		DO_TEST_HARNESS_RUNNING();

		if (TMR8_Tick_TestAndClear(&applicationTick))
		{
			TEST_LED_TOGGLE;
			COMMS_Send("TESTMESSAGE");
			//SM_Event(smIndex, TIMER);
		}
		
		//if (TMR8_Tick_TestAndClear(&ambientADCTick))
		{
			//TS_StartConversion(SENSOR_AMBIENT);
		}

		if (TMR8_Tick_TestAndClear(&irPulseTick))
		{
			//IO_Toggle(IR_OUTFLOW_LED_PINS, IR_OUTFLOW_LED_PIN);
		}

		/*COMMS_Check();
		
		TS_Check();*/
				
		//TODO: sleepUntilInterrupt();
	}
}

static void setupIO(void)
{
	IO_SetMode(eIR_LEVEL_LED_PORT, IR_LEVEL_LED_PIN, IO_MODE_OUTPUT);
	IO_SetMode(eIR_OUTFLOW_LED_PORT, IR_OUTFLOW_LED_PIN, IO_MODE_OUTPUT);
}

static void setupTimers(void)
{
	TMR8_Tick_Init(3, 0);
	
	applicationTick.reload = SLOW_IDLE_TICK_MS;
	applicationTick.active = true;
	TMR8_Tick_AddTimerConfig(&applicationTick);
	
	ambientADCTick.reload = AMBIENT_ADC_TICK_MS;
	ambientADCTick.active = true;
	TMR8_Tick_AddTimerConfig(&ambientADCTick);

	irPulseTick.reload = IR_PULSE_TIME_MS;
	irPulseTick.active = true;
	TMR8_Tick_AddTimerConfig(&irPulseTick);
}

void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	TEST_LED_ON;
	TMR8_Tick_SetActive(&irPulseTick, true);
	PCINT_EnableInterrupt(IR_OUTFLOW_PCINT_NUMBER, true);
	IR_Reset(IR_OUTFLOW);
	TMR8_Tick_SetNewReloadValue(&applicationTick, FAST_DETECT_TICK_MS);
	//TS_StartConversion(SENSOR_OUTFLOW);
}

void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	switch(elevelTestMode)
	{
	case LVL_MODE_IR:
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
	TEST_LED_OFF;
	TMR8_Tick_SetActive(&irPulseTick, false);
	IO_Off(IR_OUTFLOW_LED_PORT, IR_OUTFLOW_LED_PIN);
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
	//	TS_StartConversion(SENSOR_OUTFLOW);
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
	
	/*uint16_t detectDurationMs = IR_GetOutflowSenseDurationMs();
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
	
	COMMS_Send(message);*/
	SM_Event(smIndex, SEND_COMPLETE);
}

ISR(IR_OUTFLOW_VECTOR)
{
	currentIRCount++;
}
