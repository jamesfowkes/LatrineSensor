/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Local Application Includes
 */

#include "flush_counter.h"

/*
 * Defines and typedefs
 */
 
#define DETECTION_THRESHOLD_MS (1000U)
#define DETECTION_DELAY_BEFORE_STOPPED_MS (10000U)

/* 
 * Private Variables
 */
static uint32_t totalFlushTimeMs = 0U;
static int16_t	countFinishedTimeoutMs = 0U;

/*
 * Public Function Defintions
 */

void Flush_Reset(void)
{
	totalFlushTimeMs = 0U;
	countFinishedTimeoutMs = 0U;
}

bool Flush_UpdateCount(uint16_t timeMs, bool detect)
{
	
	if (detect)
	{
		countFinishedTimeoutMs = DETECTION_DELAY_BEFORE_STOPPED_MS;
		totalFlushTimeMs += timeMs;
	}
	else
	{
		countFinishedTimeoutMs -= timeMs;
	}

	return (countFinishedTimeoutMs <= 0);
}

bool Flush_SensorHasTriggered(void)
{
	return (totalFlushTimeMs > DETECTION_THRESHOLD_MS);
}

uint32_t Flush_GetOutflowSenseDurationMs(void)
{
	return totalFlushTimeMs;
}

