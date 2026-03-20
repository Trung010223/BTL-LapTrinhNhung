#include <Arduino.h>
#include <esp_task_wdt.h>
#include "xe_failsafe.h"
#include "xe_motor.h"

const unsigned long VEHICLE_CMD_TIMEOUT = 3000;
const int WDT_TIMEOUT = 5;

bool vehicleFailsafeActive = false;

void initFailsafe() {
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("[WDT] Watchdog ON: timeout=%ds\n", WDT_TIMEOUT);
}

void watchdogReset() {
  esp_task_wdt_reset();
}

void checkVehicleFailsafe() {
  if (lastCmdTime == 0 || vehicleFailsafeActive) return;

  if (millis() - lastCmdTime > VEHICLE_CMD_TIMEOUT) {
    vehicleFailsafeActive = true;
    stopAllMotors();

    Serial.println("[FAILSAFE] Mat lien lac Gateway (>3s) -> DUNG XE!");
  }
}
