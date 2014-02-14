/*
 * Standard Library Includes
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * AVR Library Includes
 */

#include "lib_tmr8_tick.h"

/*
 * Local Application Includes
 */

#include "app_test_harness.h"

void DO_TEST_HARNESS_SETUP(void)
{
	setbuf(stdout, NULL);
}

void DO_TEST_HARNESS_RUNNING(void)
{
	TMR8_Tick_Kick(50);
}