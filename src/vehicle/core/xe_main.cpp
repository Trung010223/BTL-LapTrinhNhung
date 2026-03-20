#include <Arduino.h>
#include "xe.h"
#include "xe_motor.h"
#include "xe_imu.h"
#include "xe_espnow.h"
#include "xe_failsafe.h"

void initXe() {
  initMotor();
  
  // DEBUG: Check for test mode during first 5 seconds
  // Send 'T' via Serial to run IN3/IN4 test
  Serial.println("\n[XE] Init starting... Send 'T' in next 5s to run IN3/IN4 test");
  unsigned long testWaitStart = millis();
  bool runTest = false;
  
  while (millis() - testWaitStart < 5000) {
    if (Serial.available()) {
      char cmd = Serial.read();
      if (cmd == 'T' || cmd == 't') {
        runTest = true;
        break;
      }
    }
    delay(50);
  }
  
  if (runTest) {
    Serial.println("[XE] Test mode activated!");
    testIN3IN4Mode();
    while (true) {
      delay(1000);  // Halt after test
      Serial.println("[XE] TEST COMPLETE - waiting for reset");
    }
  }
  
  initIMU();
  initESPNow();
  initFailsafe();
  Serial.println("[XE] Init xong!");
}

void updateXe() {
  watchdogReset();
  
  updateIMU();
  checkVehicleFailsafe();
  xuLyCanBang(pitchEMA, rollEMA);
  sendTelemetry();
}
