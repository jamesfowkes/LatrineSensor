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
 
#define DETECTION_THRESHOLD_COUNTS (100)

/* 
 * Private Variables
 */
static uint32_t totalIRCount[2] = {0U, 0U};

/*
 * Public Function Defintions
 */

void IR_Reset(IR_SENSOR eSensor)
{
	totalIRCount[eSensor] = 0U;
}

bool IR_UpdateCount(IR_SENSOR eSensor, uint16_t newCount)
{
	bool countHasStopped = (newCount == 0);
	
	if (!countHasStopped)
	{
		// Something was detected
		totalIRCount[eSensor] += newCount;
	}
	return countHasStopped;
}

bool IR_SensorHasTriggered(IR_SENSOR eSensor)
{
	return (totalIRCount[eSensor] > DETECTION_THRESHOLD_COUNTS);
}

uint16_t IR_GetOutflowSenseDurationMs(void)
{
	/* Because the IR frequency is 1kHz, the detection duration
	in milliseconds is just equal to the number of counts */
	return totalIRCount[IR_OUTFLOW];
}

