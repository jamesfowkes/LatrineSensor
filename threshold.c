/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * AVR Includes (Defines and Primitives)
 */
 
#include <avr/eeprom.h>

/*
 * Local Application Includes
 */

#include "threshold.h"

/*
 * Defines and typedefs
 */

#define DEFAULT_THRESHOLD 500U;

/* 
 * Private Variables
 */
 
static uint16_t s_threshold = 0UL;
uint16_t EEMEM s_thresholdEEPROM = DEFAULT_THRESHOLD;

void Threshold_Init(void)
{
	s_threshold = eeprom_read_word(&s_thresholdEEPROM);
}

uint16_t Threshold_Get(void)
{
	return s_threshold;
}

void Threshold_Set(uint16_t newThreshold)
{
	s_threshold = newThreshold;
	eeprom_update_word(&s_thresholdEEPROM, s_threshold) ;
}
