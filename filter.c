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
 
static void handleNewValueInFlush(uint16_t newValue);
static void handleNewValueInIdle(uint16_t newValue);

/* 
 * Private Variables
 */
 
static uint16_t s_idleAverage = 0UL;

static AVERAGER* s_averager;

static bool s_flushing;

void Filter_Init(void)
{
	s_averager = AVERAGER_GetAverager(U16, N);
}

bool Filter_NewValue(uint16_t newValue)
{
	if (s_flushing)
	{
		handleNewValueInFlush(newValue);
	}
	else
	{
		handleNewValueInIdle(newValue);
	}
	
	return s_flushing;
}

/*
 * Private Function Defintions
 */
 
static void handleNewValueInFlush(uint16_t newValue)
{
	// Flush has finished when reading has returned to half of previous level
	s_flushing = newValue > (s_idleAverage - (THRESHOLD/2));
}

static void handleNewValueInIdle(uint16_t newValue)
{
	s_flushing = newValue < (s_idleAverage - THRESHOLD);
	
	if (!s_flushing)
	{
		// Not flushing, so make this reading part of the idle average and update
		AVERAGER_NewData(s_averager, &newValue);
		AVERAGER_GetAverage(s_averager, &s_idleAverage);
	}
}
