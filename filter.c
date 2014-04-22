/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Generic Library Includes
 */
 
#include "averager.h"

/*
 * Local Application Includes
 */

#include "filter.h"

/*
 * Defines and typedefs
 */
 
#define N 10 // Number of samples to average
#define THRESHOLD 500 // Readings must fall by this amount to be counted as a flush

/*
 * Private Function Prototypes
 */

/* 
 * Private Variables
 */
 
static uint16_t s_idleAverage = 0UL;
static uint16_t s_lastThreeAverage = 0UL;

static AVERAGER* s_idleAverager;
static AVERAGER* s_lastThreeAverager;

static bool s_bFlushing;

void Filter_Init(void)
{
	s_idleAverager = AVERAGER_GetAverager(U16, N);
	s_lastThreeAverager = AVERAGER_GetAverager(U16, 3);
	s_bFlushing = false;
}

bool Filter_NewValue(uint16_t newValue)
{

	AVERAGER_NewData(s_lastThreeAverager, &newValue);
	AVERAGER_GetAverage(s_lastThreeAverager, &s_lastThreeAverage);

	// Flush has started when average reading has dropped below threshold
	bool bFlushing = s_lastThreeAverage < (s_idleAverage - THRESHOLD);

	if (s_bFlushing && !bFlushing)
	{
		// Stopped flushing, reset the idle averager to the last three readings
		AVERAGER_Reset(s_idleAverager, &s_lastThreeAverage);
	}
	
	s_bFlushing = bFlushing;
	
	if (!bFlushing)
	{
		// Not flushing, so make this reading part of the idle average and update
		AVERAGER_NewData(s_idleAverager, &newValue);
		AVERAGER_GetAverage(s_idleAverager, &s_idleAverage);

	}
	
	return s_bFlushing;
}

uint16_t Filter_GetIdleAverage(void)
{
	return s_idleAverage;
}

uint16_t Filter_GetLastThreeAverage(void)
{
	return s_lastThreeAverage;
}