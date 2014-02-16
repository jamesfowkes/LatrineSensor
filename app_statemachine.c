/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

/*
 * Generic Library Includes
 */

#include "statemachine.h"
#include "statemachinemanager.h" 

/*
 * Local Application Includes
 */
#include "latrinesensor.h"
#include "app_statemachine.h"

#ifdef TEST_HARNESS
static void onStateChange(SM_STATEID old, SM_STATEID new, SM_EVENT e);
#else
#define onStateChange NULL
#endif

static const SM_STATE stateIdle = {IDLE, NULL, onStateChange};
static const SM_STATE stateCounting = {COUNTING, NULL, onStateChange};
static const SM_STATE stateSending = {SENDING, NULL, onStateChange};
static const SM_STATE stateLevelTest = {LEVEL_TEST, NULL, onStateChange};

static const SM_ENTRY sm[] = {
	{&stateIdle,		TIMER,			startCounting,	&stateCounting	},
	{&stateIdle,		TEST_LEVEL,		startLevelTest,	&stateLevelTest	},
		
	{&stateCounting,	TIMER,			testCount,		&stateCounting	},
	{&stateCounting,	NO_DETECT,		NULL,			&stateIdle		},
	{&stateCounting,	DETECT,			sendData,		&stateSending	},
	
	{&stateLevelTest,	TIMER,			testPitFull,	&stateLevelTest	},
	{&stateLevelTest,	PIT_FULL,		sendPitFull,	&stateSending	},
	{&stateLevelTest,	PIT_NOT_FULL,	NULL,			&stateIdle		},
	
	{&stateSending,		SEND_COMPLETE,	NULL,			&stateIdle		},

	{NULL,				(STATES)0,		NULL,			NULL}
};

int8_t APPSM_SetupStateMachine(void)
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

