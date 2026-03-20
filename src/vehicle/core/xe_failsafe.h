#ifndef XE_FAILSAFE_H
#define XE_FAILSAFE_H

#include <esp_task_wdt.h>

void initFailsafe();
void checkVehicleFailsafe(float pitch, float roll);
void checkBalanceFeedbackLoop(float pitch, float roll);
void watchdogReset();

extern unsigned long lastCmdTime;
extern bool vehicleFailsafeActive;
extern bool feedbackLoopFaultActive;

#endif
