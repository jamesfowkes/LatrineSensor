#ifndef _FLUSH_COUNTER_H_
#define _FLUSH_COUNTER_H_

/*
 * Defines and typedefs
 */
 
/*
 * Public Function Prototypes
 */
 
void Flush_Reset(void);
bool Flush_UpdateCount(uint16_t newCount, bool detect);
bool Flush_SensorHasTriggered(void);
uint32_t Flush_GetOutflowSenseDurationMs(void);

#endif
