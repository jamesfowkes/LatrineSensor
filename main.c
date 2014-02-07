/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

#include "tempsense.h"
#include "ircounter.h"
#include "comms.h"

/*
 * Defines and typedefs
 */

#define FAST_DETECT_TICK_MS			(100)
#define SLOW_IDLE_TICK_MS 			(2500)
#define LEVEL_TEST_TICK_MS			(1000)

#define AMBIENT_ADC_TICK_MS	(30000)

enum level_test_mode_enum
{
	LVL_MODE_IR, // Test for full level is with a IR based system
	LVL_MODE_SW // Test for full level is with an switch based system
};
typedef enum level_test_mode_enum LEVEL_TEST_MODE_ENUM;

/*
 * Private Function Prototypes
 */

static void setupStateMachine(void);
static void setupTimers(void);

// State machine handler functions
static void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e);
static void testCount(SM_STATEID old, SM_STATEID new, SM_EVENT e);

static void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e);
static void testPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e);
static void sendPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e);
static void sendData(SM_STATEID old, SM_STATEID new, SM_EVENT e);

/*
 * Main state machine typedefs and structures
 */
enum states
{
	IDLE,
	COUNTING,
	SENDING,
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

static const SM_STATE stateIdle = {IDLE, NULL, NULL};
static const SM_STATE stateCounting = {COUNTING, NULL, NULL,};
static const SM_STATE stateSending = {SENDING, NULL, NULL };
static const SM_STATE stateLevelTest = {LEVEL_TEST, NULL, NULL};

static const SM_ENTRY sm[] = {
	{&stateIdle,		TIMER,			startCounting,	&stateCounting	},
	{&stateIdle,		TEST_LEVEL,		startLevelTest,	&stateLevelTest	},
		
	{&stateCounting,	TIMER,			testCount,		&stateCounting	},
	{&stateCounting,	NO_DETECT,		NULL,			&stateIdle		},
	{&stateCounting,	DETECT,			sendData,		&stateSending	},
	
	{&stateLevelTest,	TIMER,			testPitFull,	&stateLevelTest	},
	{&stateLevelTest,	PIT_FULL,		sendPitFull,	&stateSending	},
	{&stateLevelTest,	PIT_NOT_FULL,	NULL,			&stateIdle		},
	
	{&stateSending,		SEND_COMPLETE,	NULL,			&stateIdle		}
};
 
/* 
 * Private Variables
 */

static uint8_t smIndex;

static LEVEL_TEST_MODE_ENUM elevelTestMode = LVL_MODE_SW;

static TMR8_TICK_CONFIG applicationTick;
static TMR8_TICK_CONFIG ambientADCTick;

static uint16_t currentIRCount;

/*
 * Private Function Defintions
 */

int main(void)
{

	/* Disable watchdog: not required for this application */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	
	setupStateMachine();
	setupTimers();
	
	TS_Setup();
	IR_Reset(IR_OUTFLOW);
	IR_Reset(IR_LEVEL);
	
	COMMS_Init(CT_UART);
	
	sei();
	
	while (true)
	{
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

static void setupStateMachine(void)
{
	SMM_Config(1, 20);
	smIndex = SM_Init(&stateIdle, MAX_EVENTS, MAX_STATES, sm);
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

static void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	//TODO: enable PCINT interrupt
	IR_Reset(IR_OUTFLOW);
	TMR8_Tick_SetNewReloadValue(&applicationTick, FAST_DETECT_TICK_MS);
	TS_StartConversion(SENSOR_OUTFLOW);
}

static void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	
	switch(elevelTestMode)
	{
	case LVL_MODE_IR:
		//TODO: enable PCINT interrupt and IR output
		TMR8_Tick_SetNewReloadValue(&applicationTick, LEVEL_TEST_TICK_MS);
		IR_Reset(IR_LEVEL);
		break;
	case LVL_MODE_SW:
		// TODO: read level sense switch and send event
		break;
	}
}

static void testPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	SM_Event(smIndex, IR_SensorHasTriggered(IR_LEVEL) ?  PIT_FULL : PIT_NOT_FULL);
}

static void stopCounting(void)
{
	//TODO: Disable PCINT interrupt and IR output
	TMR8_Tick_SetNewReloadValue(&applicationTick, SLOW_IDLE_TICK_MS);
	SM_Event(smIndex, IR_SensorHasTriggered(IR_OUTFLOW) ? DETECT: NO_DETECT);
}

static void testCount(SM_STATEID old, SM_STATEID new, SM_EVENT e)
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

static void sendPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e)
{
	(void)old; (void)new; (void)e;
	COMMS_Send("PITFULL");
}

static void sendData(SM_STATEID old, SM_STATEID new, SM_EVENT e)
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
