#ifndef _APP_TEST_HARNESS_H_
#define _APP_TEST_HARNESS_H_

#ifdef TEST_HARNESS
void DO_TEST_HARNESS_SETUP(void);
void DO_TEST_HARNESS_RUNNING(void);
#else
#define DO_TEST_HARNESS_SETUP() {};
#define DO_TEST_HARNESS_RUNNING() {};
#endif

#endif
