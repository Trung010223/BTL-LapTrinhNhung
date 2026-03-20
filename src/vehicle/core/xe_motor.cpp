#include <Arduino.h>
#include "xe_motor.h"

// Drive L298N (4 wheels)
const int ENA = 32; const int IN1 = 33; const int IN2 = 25; // Left side
const int ENB = 26; const int IN3 = 27; const int IN4 = 14; // Right side

// Lift L298N #1 (no EN wire, jumper HIGH)
const int IN5 = 13; const int IN6 = 15; // Lift motor A
const int IN7 = 4;  const int IN8 = 5;  // Lift motor B

// Lift L298N #2 (no EN wire, jumper HIGH)
const int IN9 = 23;  const int IN10 = 2;  // Lift motor C
const int IN11 = 18; const int IN12 = 19; // Lift motor D

const int freq = 5000, res = 8;

namespace {
// Screw-lift mechanics usually mirror one side, so right-side motors
// must spin opposite to keep all corners moving in the same physical direction.
constexpr int DRIVE_DIR_LEFT = 1;
constexpr int DRIVE_DIR_RIGHT = 1;
// Hardware fallback: keep EN pins as plain ON/OFF to avoid PWM-ENA wiring issues.
constexpr bool DRIVE_ENABLE_ALWAYS_HIGH = true;
constexpr int LIFT_DIR_A = 1;
constexpr int LIFT_DIR_B = -1;
constexpr int LIFT_DIR_C = 1;
constexpr int LIFT_DIR_D = -1;

int toPhysicalDriveSpeed(int logicalSpeed, int motorDir) {
  return constrain(logicalSpeed * motorDir, -255, 255);
}

int toPhysicalLiftSpeed(int logicalSpeed, int motorDir) {
  return constrain(logicalSpeed * motorDir, -255, 255);
}
}

int curA = 0, curB = 0, curC = 0, curD = 0, curE = 0;
int tocDoCanBang = 80, tocDoBanhLai = 70, buocRamp = 8;

static void runDriveChannel(int in1, int in2, int enablePin, int channel, int speed) {
  int pwm = constrain(abs(speed), 0, 255);
  if (DRIVE_ENABLE_ALWAYS_HIGH) {
    digitalWrite(enablePin, pwm > 0 ? HIGH : LOW);
  }

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    if (!DRIVE_ENABLE_ALWAYS_HIGH) {
      ledcWrite(channel, pwm);
    }
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    if (!DRIVE_ENABLE_ALWAYS_HIGH) {
      ledcWrite(channel, pwm);
    }
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    if (!DRIVE_ENABLE_ALWAYS_HIGH) {
      ledcWrite(channel, 0);
    }
  }
}

static void runLiftMotor(int in1, int in2, int speed) {
  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

void initMotor() {
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  if (!DRIVE_ENABLE_ALWAYS_HIGH) {
    // PWM only for drive L298N if ENA/ENB are wired to ESP32.
    ledcSetup(0, freq, res); ledcAttachPin(ENA, 0);
    ledcSetup(1, freq, res); ledcAttachPin(ENB, 1);
  } else {
    digitalWrite(ENA, LOW);
    digitalWrite(ENB, LOW);
  }

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(IN5, OUTPUT); pinMode(IN6, OUTPUT);
  pinMode(IN7, OUTPUT); pinMode(IN8, OUTPUT);
  pinMode(IN9, OUTPUT); pinMode(IN10, OUTPUT);
  pinMode(IN11, OUTPUT); pinMode(IN12, OUTPUT);

  digitalWrite(IN5, LOW); digitalWrite(IN6, LOW);
  digitalWrite(IN7, LOW); digitalWrite(IN8, LOW);
  digitalWrite(IN9, LOW); digitalWrite(IN10, LOW);
  digitalWrite(IN11, LOW); digitalWrite(IN12, LOW);

  Serial.println("[MOTOR] Init xong");
}

int rampTo(int cur, int target, int step) {
  if (cur < target) return min(cur + step, target);
  if (cur > target) return max(cur - step, target);
  return cur;
}

void setDriveSpeed(int leftSpeed, int rightSpeed) {
  curE = (leftSpeed + rightSpeed) / 2;
  runDriveChannel(IN1, IN2, ENA, 0, toPhysicalDriveSpeed(leftSpeed, DRIVE_DIR_LEFT));
  runDriveChannel(IN3, IN4, ENB, 1, toPhysicalDriveSpeed(rightSpeed, DRIVE_DIR_RIGHT));
}

void setLiftSpeed(int motorA, int motorB, int motorC, int motorD) {
  curA = rampTo(curA, motorA, buocRamp);
  curB = rampTo(curB, motorB, buocRamp);
  curC = rampTo(curC, motorC, buocRamp);
  curD = rampTo(curD, motorD, buocRamp);

  runLiftMotor(IN5, IN6, toPhysicalLiftSpeed(curA, LIFT_DIR_A));
  runLiftMotor(IN7, IN8, toPhysicalLiftSpeed(curB, LIFT_DIR_B));
  runLiftMotor(IN9, IN10, toPhysicalLiftSpeed(curC, LIFT_DIR_C));
  runLiftMotor(IN11, IN12, toPhysicalLiftSpeed(curD, LIFT_DIR_D));
}

void stopAllMotors() {
  setDriveSpeed(0, 0);
  setLiftSpeed(0, 0, 0, 0);
  curA = curB = curC = curD = curE = 0;
}

// DEBUG TEST MODE: Cycle IN3/IN4 pins for 60 seconds to verify hardware
// Use multimeter to monitor voltage at IN3 (GPIO27) and IN4 (GPIO14)
// ENB (GPIO26) should also remain HIGH during test
void testIN3IN4Mode() {
  Serial.println("\n[TEST] === START IN3/IN4 HARDWARE TEST (60s) ===");
  Serial.println("[TEST] EN3=27, IN4=14 will cycle 1s ON / 1s OFF");
  Serial.println("[TEST] Use multimeter to check voltage at these pins");
  Serial.println("[TEST] ENB=26 should be HIGH during entire test\n");
  
  // Setup EN pin to stay HIGH
  digitalWrite(ENB, HIGH);
  
  unsigned long startTime = millis();
  unsigned long lastToggle = millis();
  bool state = true;  // true = IN3 HIGH / IN4 LOW (forward), false = opposite
  int cycleCount = 0;
  
  while (millis() - startTime < 60000) {  // Run for 60 seconds
    unsigned long now = millis();
    
    // Toggle every 1 second
    if (now - lastToggle >= 1000) {
      state = !state;
      cycleCount++;
      
      if (state) {
        // Forward: IN3 HIGH, IN4 LOW
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        Serial.printf("[TEST] Cycle %d: IN3=HIGH, IN4=LOW (Forward)\n", cycleCount);
      } else {
        // Reverse: IN3 LOW, IN4 HIGH
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        Serial.printf("[TEST] Cycle %d: IN3=LOW, IN4=HIGH (Reverse)\n", cycleCount);
      }
      
      lastToggle = now;
    }
    
    delay(100);  // Small delay to prevent blocking
  }
  
  // Stop test and safeguard
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(ENB, LOW);
  Serial.println("[TEST] === TEST COMPLETE: IN3/IN4 paused (LOW) ===\n");
}
