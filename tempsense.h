#ifndef _TEMPERATURE_SENSORS_H_
#define _TEMPERATURE_SENSORS_H_

/*
 * Defines and typedefs
 */

typedef uint16_t TENTHSDEGC;

enum temperature_sensor
{
	SENSOR_OUTFLOW,
	SENSOR_AMBIENT
};
typedef enum temperature_sensor TEMPERATURE_SENSOR;

/*
 * Public Function Prototypes
 */
 
void TS_Setup(void);
void TS_Check(void);

void TS_AmbientTimerTick(uint8_t seconds);
void TS_OutflowTimerTick(uint8_t milliseconds);

bool TS_IsTimeForAmbientRead(void);
bool TS_IsTimeForOutflowRead(void);

bool TS_ConversionStarted(void);

void TS_StartConversion(TEMPERATURE_SENSOR eSensor);
uint16_t TS_GetTemperature(TEMPERATURE_SENSOR eSensor);

#endif
