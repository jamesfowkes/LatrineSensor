#ifndef _LATRINE_SENSOR_H_
#define _LATRINE_SENSOR_H_

void startCounting(SM_STATEID old, SM_STATEID new, SM_EVENT e);

void startLevelTest(SM_STATEID old, SM_STATEID new, SM_EVENT e);
void testPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e);
void sendPitFull(SM_STATEID old, SM_STATEID new, SM_EVENT e);
void wakeMaster(SM_STATEID old, SM_STATEID new, SM_EVENT e);
void sendData(SM_STATEID old, SM_STATEID new, SM_EVENT e);
void startWakeTimer(SM_STATEID old, SM_STATEID new, SM_EVENT e);

void onIdleState(SM_STATEID old, SM_STATEID new, SM_EVENT e);

#endif
