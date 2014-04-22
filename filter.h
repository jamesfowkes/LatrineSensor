#ifndef _FILTER_H_
#define _FILTER_H_

/*
 * Defines and typedefs
 */
 
/*
 * Public Function Prototypes
 */
 
void Filter_Init(void);
bool Filter_NewValue(uint16_t newValue);

uint16_t Filter_GetIdleAverage(void);
uint16_t Filter_GetLastThreeAverage(void);

#endif
