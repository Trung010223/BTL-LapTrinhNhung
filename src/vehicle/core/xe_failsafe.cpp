#include <Arduino.h>
#include <esp_task_wdt.h>
#include "xe_failsafe.h"
#include "xe_espnow.h"
#include "xe_motor.h"

const unsigned long VEHICLE_CMD_TIMEOUT = 3000;
const int WDT_TIMEOUT = 5;
const unsigned long FEEDBACK_WINDOW_MS = 1500;
const float FEEDBACK_ARMED_TILT = 5.0f;
const float FEEDBACK_MIN_IMPROVEMENT = 0.6f;
const unsigned long FEEDBACK_RECOVER_HOLD_MS = 1200;
const float FEEDBACK_RECOVER_TILT = 2.5f;

bool vehicleFailsafeActive = false;
bool feedbackLoopFaultActive = false;

namespace {
bool commandTimeoutFaultActive = false;
bool feedbackMonitorActive = false;
unsigned long feedbackWindowStart = 0;
float feedbackInitialTilt = 0.0f;
float feedbackBestTilt = 0.0f;
unsigned long feedbackRecoverSince = 0;

float tiltMagnitude(float pitch, float roll) {
  return max(fabs(pitch), fabs(roll));
}

bool hasActiveActuatorOutput() {
  return abs(curA) > 0 || abs(curB) > 0 || abs(curC) > 0 || abs(curD) > 0 || abs(curE) > 0;
}

void resetFeedbackMonitor() {
  feedbackMonitorActive = false;
  feedbackWindowStart = 0;
  feedbackInitialTilt = 0.0f;
  feedbackBestTilt = 0.0f;
}

void updateFailsafeState() {
  vehicleFailsafeActive = commandTimeoutFaultActive || feedbackLoopFaultActive;
}

void clearFeedbackRecoveryWindow() {
  feedbackRecoverSince = 0;
}

void tryRecoverFeedbackFault(float pitch, float roll) {
  if (!feedbackLoopFaultActive) {
    clearFeedbackRecoveryWindow();
    return;
  }

  if (commandTimeoutFaultActive) {
    clearFeedbackRecoveryWindow();
    return;
  }

  float currentTilt = tiltMagnitude(pitch, roll);
  if (!latestCmd_stop || currentTilt > FEEDBACK_RECOVER_TILT || hasActiveActuatorOutput()) {
    clearFeedbackRecoveryWindow();
    return;
  }

  if (feedbackRecoverSince == 0) {
    feedbackRecoverSince = millis();
    return;
  }

  if (millis() - feedbackRecoverSince < FEEDBACK_RECOVER_HOLD_MS) {
    return;
  }

  feedbackLoopFaultActive = false;
  resetFeedbackMonitor();
  clearFeedbackRecoveryWindow();
  updateFailsafeState();
  Serial.println("[FEEDBACK] Dieu kien an toan dat yeu cau -> clear feedback fault");
}
}

void initFailsafe() {
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("[WDT] Watchdog ON: timeout=%ds\n", WDT_TIMEOUT);
}

void watchdogReset() {
  esp_task_wdt_reset();
}

void checkBalanceFeedbackLoop(float pitch, float roll) {
  float currentTilt = tiltMagnitude(pitch, roll);

  if (feedbackLoopFaultActive) {
    resetFeedbackMonitor();
    return;
  }

  if (commandTimeoutFaultActive || currentTilt < FEEDBACK_ARMED_TILT || !hasActiveActuatorOutput()) {
    resetFeedbackMonitor();
    return;
  }

  if (!feedbackMonitorActive) {
    feedbackMonitorActive = true;
    feedbackWindowStart = millis();
    feedbackInitialTilt = currentTilt;
    feedbackBestTilt = currentTilt;
    return;
  }

  if (currentTilt < feedbackBestTilt) {
    feedbackBestTilt = currentTilt;
  }

  if (millis() - feedbackWindowStart < FEEDBACK_WINDOW_MS) {
    return;
  }

  if ((feedbackInitialTilt - feedbackBestTilt) < FEEDBACK_MIN_IMPROVEMENT) {
    feedbackLoopFaultActive = true;
    updateFailsafeState();
    stopAllMotors();
    Serial.printf("[FEEDBACK] Tilt khong cai thien (start=%.2f best=%.2f) -> DUNG XE!\n",
                  feedbackInitialTilt, feedbackBestTilt);
    resetFeedbackMonitor();
    return;
  }

  feedbackInitialTilt = feedbackBestTilt;
  feedbackWindowStart = millis();
}

void checkVehicleFailsafe(float pitch, float roll) {
  if (lastCmdTime == 0) {
    updateFailsafeState();
    return;
  }

  if (millis() - lastCmdTime > VEHICLE_CMD_TIMEOUT) {
    if (!commandTimeoutFaultActive) {
      commandTimeoutFaultActive = true;
      updateFailsafeState();
      stopAllMotors();
      Serial.println("[FAILSAFE] Mat lien lac Gateway (>3s) -> DUNG XE!");
    }
    clearFeedbackRecoveryWindow();
    return;
  }

  if (commandTimeoutFaultActive) {
    commandTimeoutFaultActive = false;
    updateFailsafeState();
    Serial.println("[FAILSAFE] Gateway phuc hoi lien lac");
  }

  tryRecoverFeedbackFault(pitch, roll);
}
