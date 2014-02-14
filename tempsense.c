/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Local Application Includes
 */

#include "tempsense.h"

/*
 * AVR Library Includes
 */

#include "lib_adc.h"

/*
 * Private Function Prototypes
 */

static TENTHSDEGC convertToTenthsOfDegrees(uint16_t reading);

/* 
 * Private Variables
 */

static ADC_CONTROL_ENUM adc;
static TEMPERATURE_SENSOR currentSensor;

static LIB_ADC_CHANNEL_ENUM channels[2] = {
	LIB_ADC_CH_0, //SENSOR_OUTFLOW
	LIB_ADC_CH_1, //SENSOR_AMBIENT
};

static TENTHSDEGC readings[2] = {0, 0};

/*
 * Public Function Defintions
 */

void TS_Setup(void)
{
	ADC_SelectPrescaler(LIB_ADC_PRESCALER_DIV64);
	ADC_SelectReference(LIB_ADC_REF_VCC);
	ADC_Enable(true);
	ADC_EnableInterrupts(true);

	currentSensor = SENSOR_OUTFLOW;
	
	adc.busy = false;
	adc.channel = currentSensor;
	adc.conversionComplete = false;
}

void TS_Check(void)
{
	if (ADC_TestAndClear(&adc))
	{
		readings[currentSensor] = convertToTenthsOfDegrees(adc.reading);
	}
}

void TS_StartConversion(TEMPERATURE_SENSOR eSensor)
{
	if (!adc.busy)
	{
		adc.channel = channels[eSensor];
		ADC_GetReading(&adc);
	}
}

uint16_t TS_GetTemperature(TEMPERATURE_SENSOR eSensor)
{
	return readings[eSensor];
}

/*
 * Private Function Defintions
 */
 
static TENTHSDEGC convertToTenthsOfDegrees(uint16_t reading)
{
	//TODO: Convert reading to temperature
	return (TENTHSDEGC)reading;
}
