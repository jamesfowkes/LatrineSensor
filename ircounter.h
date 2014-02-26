#ifndef _IR_COUNTER_H_
#define _IR_COUNTER_H_

/*
 * Defines and typedefs
 */
 
enum ir_sensor
{
	IR_OUTFLOW,
	IR_LEVEL
};
typedef enum ir_sensor IR_SENSOR;

/*
 * Public Function Prototypes
 */
 
void IR_Reset(IR_SENSOR eSensor);
bool IR_UpdateCount(IR_SENSOR eSensor, uint16_t newCount, bool detect);
bool IR_SensorHasTriggered(IR_SENSOR eSensor);
uint32_t IR_GetOutflowSenseDurationMs(void);

#endif
