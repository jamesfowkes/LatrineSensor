#ifndef _COMMS_H_
#define _COMMS_H_

/*
 *Defines and Typedefs 
 */
 
enum comms_type
{
	CT_UART
	//CT_SPI - may be supported in future versions
};
typedef enum comms_type COMMS_TYPE;

void COMMS_Init(COMMS_TYPE eType);
void COMMS_Send(char * s);
void COMMS_Check(void);

#endif
