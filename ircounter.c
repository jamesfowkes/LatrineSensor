/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Local Application Includes
 */

#include "ircounter.h"

/*
 * Defines and typedefs
 */
 
#define DETECTION_THRESHOLD_MS (1000U)
#define DETECTION_DELAY_BEFORE_STOPPED_MS (10000U)
/* 
 * Private Variables
 */
static uint32_t totalIRTimeMs[2] = {0U, 0U};
static int16_t	countFinishedTimeoutMs[2] = {0U, 0U};

/*
 * Public Function Defintions
 */

void IR_Reset(IR_SENSOR eSensor)
{
	totalIRTimeMs[eSensor] = 0U;
	countFinishedTimeoutMs[eSensor] = 0U;
}

bool IR_UpdateCount(IR_SENSOR eSensor, uint16_t timeMs, bool detect)
{
	totalIRTimeMs[eSensor] += timeMs;

	if (detect)
	{
		countFinishedTimeoutMs[eSensor] = DETECTION_DELAY_BEFORE_STOPPED_MS;
	}
	else
	{
		countFinishedTimeoutMs[eSensor] -= timeMs;
	}

	return (countFinishedTimeoutMs[eSensor] <= 0);
}

bool IR_SensorHasTriggered(IR_SENSOR eSensor)
{
	return (totalIRTimeMs[eSensor] > DETECTION_THRESHOLD_MS);
}

uint32_t IR_GetOutflowSenseDurationMs(void)
{
	return totalIRTimeMs[IR_OUTFLOW] - DETECTION_DELAY_BEFORE_STOPPED_MS;
}

