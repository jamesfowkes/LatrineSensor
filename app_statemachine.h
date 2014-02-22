#ifndef _APP_STATEMACHINE_H_
#define _APP_STATEMACHINE_H_

enum states
{
	IDLE,
	COUNTING,
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


int8_t APPSM_SetupStateMachine(void);

#endif
