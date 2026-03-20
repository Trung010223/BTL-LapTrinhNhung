#ifndef XE_FAILSAFE_H
#define XE_FAILSAFE_H

#include <esp_task_wdt.h>

void initFailsafe();
void checkVehicleFailsafe();
void watchdogReset();

extern unsigned long lastCmdTime;
extern bool vehicleFailsafeActive;

#endif
