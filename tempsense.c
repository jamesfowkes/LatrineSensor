/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Utility Library Includes
 */

#include "util_fixedpoint.h"

/*
 * Local Application Includes
 */

#include "tempsense.h"

/*
 * AVR Library Includes
 */

#include "lib_adc.h"

/*
 * Device Library Includes
 */
 
#include "lib_thermistor.h"

/*
 * Defines and typedefs
 */
 
#define AMBIENT_ADC_TICK_SECS		(600)
#define OUTFLOW_ADC_TICK_MS			(1000)

#define RTHERM						(10000UL)
#define RPULLDOWN					(10000UL)

#define	THERMISTOR_BETA				(3000U)

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

// Ambient countdown is in seconds, the other in milliseconds
static int16_t countdowns[] = {OUTFLOW_ADC_TICK_MS, AMBIENT_ADC_TICK_SECS};

static THERMISTOR thermistor;

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
	
	THERMISTOR_Init();
	(void)THERMISTOR_InitDevice(&thermistor, THERMISTOR_BETA, RTHERM);
}

void TS_AmbientTimerTick(uint8_t seconds)
{
	countdowns[SENSOR_AMBIENT] -= seconds;
}

void TS_OutflowTimerTick(uint8_t milliseconds)
{
	countdowns[SENSOR_OUTFLOW] -= milliseconds;
}

bool TS_IsTimeForAmbientRead(void)
{
	return (countdowns[SENSOR_AMBIENT] <= 0);
}

bool TS_IsTimeForOutflowRead(void)
{
	return (countdowns[SENSOR_OUTFLOW] <= 0);
}

void TS_Check(void)
{
	if (ADC_TestAndClear(&adc))
	{
		countdowns[SENSOR_OUTFLOW] = OUTFLOW_ADC_TICK_MS;
		countdowns[SENSOR_AMBIENT] = AMBIENT_ADC_TICK_SECS;
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

TENTHSDEGC TS_GetTemperature(TEMPERATURE_SENSOR eSensor)
{
	return readings[eSensor];
}

bool TS_ConversionStarted(void)
{
	return adc.busy;
}

/*
 * Private Function Defintions
 */
 
static TENTHSDEGC convertToTenthsOfDegrees(uint16_t reading)
{
	// Convert the reading into thermistor resistance before conversion

	return (TENTHSDEGC)(10U * (uint16_t)fp_to_int( THERMISTOR_GetReading(&thermistor, reading) ) );
}
