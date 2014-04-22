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

/*
 * AVR Library Includes
 */

#include "lib_io.h"
#include "lib_delay.h"
#include "lib_pcint.h"
#include "lib_wdt.h"
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
#include "tempsense.h"
#include "flush_counter.h"
#include "filter.h"
#include "comms.h"

/*
 * Defines and typedefs
 */

enum states
{
	IDLE,
	SENDING1,
	SENDING2,
	SENDING3,
	LEVEL_TEST,
	MAX_STATES
};
typedef enum states STATES;

enum events
{
	TIMER,
	TEST_LEVEL,
	COMPLETE,
	DETECT,
	NO_DETECT,
	PIT_FULL,
	PIT_NOT_FULL,
	SEND_COMPLETE,
	MAX_EVENTS
};
typedef enum events EVENTS;

// Make sure these defines are kept in sync or bad things will happen.
#define IDLE_TIME_MS	(1000)
#define COMMS_TIME_MS	(120)

#define OUTFLOW_PCINT_VECTOR		PCINT1_vect
#define OUTFLOW_PCINT_NUMBER		10

#define eDETECT_CIRCUIT_RESET_PORT	IO_PORTB
#define DETECT_CIRCUIT_RESET_PORT	PORTB
#define DETECT_CIRCUIT_RESET_PINS	PINB
#define DETECT_CIRCUIT_RESET_PIN	0

#define eGENERIC_OUTPUT_PORT		IO_PORTB
#define GENERIC_OUTPUT_PORT			PORTB
#define GENERIC_OUTPUT_PINS			PINB
#define GENERIC_OUTPUT_PIN			1

#define eSETUP_PORT					IO_PORTB
#define SETUP_PINS					PINB
#define	SETUP_PIN0					0
#define	SETUP_PIN1					1

#define TEST_LED_ON					IO_On(GENERIC_OUTPUT_PORT, GENERIC_OUTPUT_PIN)
#define TEST_LED_OFF				IO_Off(GENERIC_OUTPUT_PORT, GENERIC_OUTPUT_PIN)
#define	TEST_LED_TOGGLE				IO_Toggle(GENERIC_OUTPUT_PINS, GENERIC_OUTPUT_PIN)

#define TEST_TOGGLE(x) 				for (uint8_t toggle_count=0; toggle_count < x; ++toggle_count) { TEST_LED_TOGGLE; }

enum test_mode_enum
{
	TEST_MODE_NONE,
	TEST_MODE_STATE
};
typedef enum test_mode_enum TEST_MODE_ENUM;

/*
 * Private Function Prototypes
 */

static void setupTimers(void);
static void setupIO(void);

static void writeTemperatureToMessage(char * msg, TEMPERATURE_SENSOR eSensor);

static void runNormalApplication(void);

static void readTestMode(void);

static int8_t setupStateMachine(void);

/*
 * Main state machine pointers
 */

/* 
 * Private Variables
 */

static int8_t smIndex;

static TMR8_TICK_CONFIG idleTick;
static TMR8_TICK_CONFIG commsTick;

static TMR8_TICK_CONFIG * pNextTick;

static TEST_MODE_ENUM testMode;

static volatile uint16_t s_flushPulseCount;

int main(void)
{
	DO_TEST_HARNESS_SETUP();
	
	WD_DISABLE();
	
	setupIO();
	readTestMode();
		
	setupTimers();
	
	smIndex = setupStateMachine();
	
	TS_Setup();
	Filter_Init();
	
	Flush_Reset();
		
	COMMS_Init();
		
	sei();
	
	runNormalApplication();
}	
	
static void runNormalApplication(void)
{
	while (true)
	{
		{
			DO_TEST_HARNESS_RUNNING();

			TS_Check();
			
			if (TMR8_Tick_TestAndClear(&idleTick))
			{
				// Test the 
				TS_AmbientTimerTick(IDLE_TIME_MS/1000);
				SM_Event(smIndex, TIMER);
			}
			
			if (TMR8_Tick_TestAndClear(&commsTick))
			{
				SM_Event(smIndex, TIMER);		
			}
			
			if ( TS_IsTimeForOutflowRead() )
			{
				TS_StartConversion(SENSOR_OUTFLOW);
			}
			else if ( TS_IsTimeForAmbientRead() )
			{
				TS_StartConversion(SENSOR_AMBIENT);
			}

			if (testMode == TEST_MODE_STATE)
			{
				uint8_t state = (uint8_t)SM_GetState(smIndex) + 1;
				
				while(state--)
				{
					TEST_LED_OFF;
					TEST_LED_ON;
					TEST_LED_OFF;
				}
			}
		}
	}
}

static void setupIO(void)
{
	IO_SetMode(eDETECT_CIRCUIT_RESET_PORT, DETECT_CIRCUIT_RESET_PIN, IO_MODE_OUTPUT);
	IO_SetMode(eGENERIC_OUTPUT_PORT, GENERIC_OUTPUT_PIN, IO_MODE_OUTPUT);
	
	IO_SetMode(eSETUP_PORT, SETUP_PIN0, IO_MODE_INPUT);
	IO_SetMode(eSETUP_PORT, SETUP_PIN1, IO_MODE_INPUT);
}

static void readTestMode(void)
{
	testMode = 0;
	
	if (IO_Read(SETUP_PINS, SETUP_PIN0) == false)
	{
		testMode += 1;
	}
	
	if (IO_Read(SETUP_PINS, SETUP_PIN1) == false)
	{
		testMode += 2;
	}
}

static void setupTimers(void)
{
	idleTick.msTick = IDLE_TIME_MS;
	commsTick.msTick = COMMS_TIME_MS;
	pNextTick = &idleTick; 
}

void testAndResetCount(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	bool isFlushing = Filter_NewValue(s_flushPulseCount);
	s_flushPulseCount = 0;
	
	bool countingStopped = Flush_UpdateCount(IDLE_TIME_MS, isFlushing);
	
	if (countingStopped)
	{
		// Nothing was detected this time, so send a detect/no detect event 
		// final result to the state machine
		SM_Event(smIndex, Flush_SensorHasTriggered() ? DETECT: NO_DETECT);
	}
}

void wakeMaster(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	COMMS_Send("WAKE");
	SM_Event(smIndex, SEND_COMPLETE);
}

void startWakeTimer(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	pNextTick = &commsTick;
}

void sendData(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	uint32_t detectDurationSecs = (Flush_GetOutflowSenseDurationMs() + 500U) / 1000U;
	
	char message[] = "aAAFEOOAADDD";
	
	writeTemperatureToMessage(&message[5], SENSOR_OUTFLOW);
	writeTemperatureToMessage(&message[7], SENSOR_AMBIENT);
	
	// uint8_t duration to string conversion:
	if (detectDurationSecs < 999)
	{
		message[9] = detectDurationSecs / 100U;
		detectDurationSecs -= (message[9] * 100U);
		message[10] = detectDurationSecs / 10U;
		detectDurationSecs -= (message[10] * 10U);
		message[11] = detectDurationSecs;
		
		message[9] += '0';
		message[10] += '0';
		message[11] += '0';
	}
	else
	{
		message[9] = '?';
		message[10] = '?';
		message[11] = '?';
	}
	
	COMMS_Send(message);
	SM_Event(smIndex, SEND_COMPLETE);
}

static void writeTemperatureToMessage(char * msg, TEMPERATURE_SENSOR eSensor)
{
	TENTHSDEGC temp = TS_GetTemperature(eSensor);
	
	if (temp > 0)
	{
		temp = (temp + 5) / 10; // Only care about integer degrees
		
		if ((temp < 100) && (temp > 0))
		{
			msg[0] = temp / 10;
			temp -= (msg[0] * 10);
			msg[1] = temp;
			
			msg[0] += '0';
			msg[1] += '0';
		}
		else if (temp >= 100)
		{
			msg[0] = '?';
			msg[1] = '?';
		}
	}
	else
	{
		msg[0] = '<';
		msg[1] = '0';
	}
}

void onIdleState(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	pNextTick = &idleTick;
}

void onStateChange(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
}

#ifdef TEST_HARNESS
static void onStateChange(SM_STATEID old, SM_STATEID new, SM_EVENT e);
#endif

static const SM_STATE stateIdle = {IDLE, NULL, onIdleState};
static const SM_STATE stateSending1 = {SENDING1, NULL, onStateChange};
static const SM_STATE stateSending2 = {SENDING2, NULL, onStateChange};
static const SM_STATE stateSending3 = {SENDING3, NULL, onStateChange};
static const SM_STATE stateLevelTest = {LEVEL_TEST, NULL, onStateChange};

static const SM_ENTRY sm[] = {
	{&stateIdle,		DETECT,			wakeMaster,		&stateSending1	},
		
	{&stateSending1,	SEND_COMPLETE,	startWakeTimer,	&stateSending2	},
	{&stateSending2,	TIMER,			sendData,		&stateSending3	},
	{&stateSending3,	SEND_COMPLETE,	NULL,			&stateIdle		},
	
	{NULL,				(STATES)0,		NULL,			NULL}
};

static int8_t setupStateMachine(void)
{
	int8_t smIndex= -1;
	
	SMM_Config(1, 5);
	smIndex = SM_Init(&stateIdle, MAX_EVENTS, MAX_STATES, sm);
	
	if (smIndex >= 0)
	{
		SM_SetActive(smIndex, true);
	}

	return smIndex;
}

#ifdef TEST_HARNESS
static void onStateChange(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	char * states[] = { "IDLE", "COUNTING", "SENDING", "LEVEL_TEST"};
	char * events[] = { "TIMER", "TEST_LEVEL", "COMPLETE", "DETECT", "NO_DETECT", "PIT_FULL", "PIT_NOT_FULL", "SEND_COMPLETE"};

	printf("Entering state %s from %s with event %s\n", states[new], states[old], events[e]);
}
#endif

ISR(OUTFLOW_PCINT_VECTOR)
{
	s_flushPulseCount++;
}

