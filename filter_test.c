/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/*
 * Utility Library Includes
 */

#include "util_sequence_generator.h"

/*
 * Local Application Includes
 */

#include "filter.h"

static SEQUENCE * seq;

int main(int argc, char * argv[])
{
	(void)argc; (void)argv;

	srand (time(NULL));
	
	Filter_Init();
	
	seq = SEQGEN_GetNewSequence(1000);
	SEQGEN_AddConstants(seq, 15000, 50);
	SEQGEN_AddConstants(seq, 14000, 50);
	SEQGEN_AddConstants(seq, 14300, 50);
	SEQGEN_AddRamp_StartStepLength(seq, 14300, 1, 200);
	SEQGEN_AddConstants(seq, 13750, 50);
	SEQGEN_AddConstants(seq, 14500, 50);
	SEQGEN_AddNoise(seq, 450);
	
	do 
	{
		uint16_t new = SEQGEN_Read(seq);
		bool flushing = Filter_NewValue( new );
		printf("%d, %d, %d, %d\n", new, Filter_GetIdleAverage(), Filter_GetLastThreeAverage(), flushing ? 0 : 10000);
	} while (!SEQGEN_EOS(seq));

	return 0;
}