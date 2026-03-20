#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include "xe_imu.h"
#include "xe_motor.h"

MPU6050 mpu6050(Wire);

const float EMA_ALPHA = 0.15f;
float pitchEMA = 0.0f, rollEMA = 0.0f;
bool emaInitialized = false;

const float HYST_HIGH = 4.0f;
const float HYST_LOW = 2.0f;
bool pitchActive = false, rollActive = false;

float zeroPitch = 0.0, zeroRoll = 0.0;
bool cheDoCanBang = true;

extern bool latestCmd_stop;
extern int latestCmd_direction;
extern int latestCmd_speed;
extern int latestCmd_lift;

namespace {
// A=front-left, B=front-right, C=rear-left, D=rear-right
constexpr int LEG_A = 1 << 0;
constexpr int LEG_B = 1 << 1;
constexpr int LEG_C = 1 << 2;
constexpr int LEG_D = 1 << 3;

const float SINGLE_HIGH_TH = 6.0f;
const float FRONT_PAIR_TH = 7.0f;
const float ROLL_NEUTRAL_TH = 2.5f;
const unsigned long INIT_BALANCE_HOLD_MS = 1200;
const unsigned long RETURN_HOME_MS = 700;
const int LIFT_SPEED = 90;
const int DRIVE_MIN_PWM = 110;

bool initBalanced = false;
unsigned long initBalanceSince = 0;
bool returningHome = false;
unsigned long returnHomeStart = 0;
int lastLiftMask = 0;

bool isWithinBalanceTolerance(float pitch, float roll) {
  return fabs(pitch) <= HYST_LOW && fabs(roll) <= HYST_LOW;
}

int normalizeDriveCommand(int speed) {
  int bounded = constrain(speed, -255, 255);
  if (bounded == 0) {
    return 0;
  }
  if (abs(bounded) < DRIVE_MIN_PWM) {
    return bounded > 0 ? DRIVE_MIN_PWM : -DRIVE_MIN_PWM;
  }
  return bounded;
}

int computeTurnOffset() {
  if (latestCmd_direction == 1) return tocDoBanhLai;
  if (latestCmd_direction == -1) return -tocDoBanhLai;
  return 0;
}

void applyLegMask(int mask, int speed) {
  int a = (mask & LEG_A) ? speed : 0;
  int b = (mask & LEG_B) ? speed : 0;
  int c = (mask & LEG_C) ? speed : 0;
  int d = (mask & LEG_D) ? speed : 0;
  setLiftSpeed(a, b, c, d);
}

int detectLiftMask(float pitch, float roll) {
  // Case 1: one leg is high -> lift the other three legs.
  if (pitch > SINGLE_HIGH_TH && roll > SINGLE_HIGH_TH) return LEG_B | LEG_C | LEG_D; // FL high
  if (pitch > SINGLE_HIGH_TH && roll < -SINGLE_HIGH_TH) return LEG_A | LEG_C | LEG_D; // FR high
  if (pitch < -SINGLE_HIGH_TH && roll > SINGLE_HIGH_TH) return LEG_A | LEG_B | LEG_D; // RL high
  if (pitch < -SINGLE_HIGH_TH && roll < -SINGLE_HIGH_TH) return LEG_A | LEG_B | LEG_C; // RR high

  // Case 2: two front legs are high -> lift two rear legs.
  if (pitch > FRONT_PAIR_TH && fabs(roll) < ROLL_NEUTRAL_TH) return LEG_C | LEG_D;
  // Symmetric case for stability: two rear legs are high -> lift two front legs.
  if (pitch < -FRONT_PAIR_TH && fabs(roll) < ROLL_NEUTRAL_TH) return LEG_A | LEG_B;

  return 0;
}
}

void initIMU() {
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcOffsets();
  Serial.println("[IMU] Calibrate xong");
}

void updateEMA(float rawPitch, float rawRoll) {
  if (!emaInitialized) {
    pitchEMA = rawPitch;
    rollEMA = rawRoll;
    emaInitialized = true;
    return;
  }
  pitchEMA = EMA_ALPHA * rawPitch + (1.0f - EMA_ALPHA) * pitchEMA;
  rollEMA = EMA_ALPHA * rawRoll + (1.0f - EMA_ALPHA) * rollEMA;
}

bool updateHysteresis(float val, bool &active) {
  float absVal = fabs(val);
  if (absVal > HYST_HIGH) active = true;
  else if (absVal <= HYST_LOW) active = false;
  return active;
}

String angleToLabel(float pitch, float roll) {
  float m = max(fabs(pitch), fabs(roll));
  if (m <= HYST_LOW) return "BALANCED";
  if (m < HYST_HIGH) return "NEAR_BALANCE";
  if (m < 15.0f) return "TILTED";
  if (m < 30.0f) return "HEAVY_TILT";
  return "FALLEN";
}

void updateIMU() {
  mpu6050.update();
  float rawPitch = mpu6050.getAngleX() - zeroPitch;
  float rawRoll = mpu6050.getAngleY() - zeroRoll;
  updateEMA(rawPitch, rawRoll);
}

void xuLyCanBang(float pitch, float roll) {
  if (!cheDoCanBang || latestCmd_stop) {
    stopAllMotors();
    pitchActive = rollActive = false;
    initBalanced = false;
    initBalanceSince = 0;
    returningHome = false;
    lastLiftMask = 0;
    return;
  }

  const unsigned long now = millis();
  const bool stablePlane = isWithinBalanceTolerance(pitch, roll);
  int driveBase = normalizeDriveCommand(latestCmd_speed);
  int turnOffset = computeTurnOffset();
  int leftDrive = constrain(driveBase + turnOffset, -255, 255);
  int rightDrive = constrain(driveBase - turnOffset, -255, 255);
  setDriveSpeed(leftDrive, rightDrive);

  const bool isDriving = (driveBase != 0 || latestCmd_direction != 0);

  // Manual lift command from web/api has highest priority over terrain auto-lift.
  if (latestCmd_lift != 0) {
    int manual = constrain(latestCmd_lift * LIFT_SPEED, -255, 255);
    setLiftSpeed(manual, manual, manual, manual);
    return;
  }

  // While actively driving, keep lift idle to avoid fighting traction/power.
  if (isDriving) {
    setLiftSpeed(0, 0, 0, 0);
    return;
  }

  // Startup phase: wait for a stable balanced posture before terrain mode.
  if (!initBalanced) {
    bool doPitch = updateHysteresis(pitch, pitchActive);
    bool doRoll = updateHysteresis(roll, rollActive);

    int pVal = 0, rVal = 0;
    if (doPitch) pVal = (pitch > 0) ? tocDoCanBang : -tocDoCanBang;
    if (doRoll) rVal = (roll > 0) ? tocDoCanBang : -tocDoCanBang;

    int tA = constrain(pVal - rVal, -tocDoCanBang, tocDoCanBang);
    int tB = constrain(pVal + rVal, -tocDoCanBang, tocDoCanBang);
    int tC = constrain(-pVal - rVal, -tocDoCanBang, tocDoCanBang);
    int tD = constrain(-pVal + rVal, -tocDoCanBang, tocDoCanBang);
    setLiftSpeed(tA, tB, tC, tD);

    if (stablePlane) {
      if (initBalanceSince == 0) initBalanceSince = now;
      if (now - initBalanceSince >= INIT_BALANCE_HOLD_MS) {
        initBalanced = true;
        initBalanceSince = 0;
        Serial.println("[LIFT] Khoi dong can bang xong, vao che do map mo");
      }
    } else {
      initBalanceSince = 0;
    }
    return;
  }

  // Return phase: after obstacle, lower previously lifted legs to home.
  if (returningHome) {
    if (now - returnHomeStart < RETURN_HOME_MS) {
      applyLegMask(lastLiftMask, -LIFT_SPEED);
      return;
    }
    returningHome = false;
    lastLiftMask = 0;
    setLiftSpeed(0, 0, 0, 0);
    Serial.println("[LIFT] Da ve lai trang thai ban dau");
    return;
  }

  int liftMask = detectLiftMask(pitch, roll);
  if (liftMask != 0) {
    lastLiftMask = liftMask;
    applyLegMask(liftMask, LIFT_SPEED);
    return;
  }

  // If we were lifting and now terrain is stable, return lifted legs.
  if (stablePlane && lastLiftMask != 0) {
    returningHome = true;
    returnHomeStart = now;
    return;
  }

  bool doPitch = updateHysteresis(pitch, pitchActive);
  bool doRoll = updateHysteresis(roll, rollActive);

  int pVal = 0, rVal = 0;
  if (doPitch) pVal = (pitch > 0) ? tocDoCanBang : -tocDoCanBang;
  if (doRoll) rVal = (roll > 0) ? tocDoCanBang : -tocDoCanBang;

  int tA = constrain(pVal - rVal, -tocDoCanBang, tocDoCanBang);
  int tB = constrain(pVal + rVal, -tocDoCanBang, tocDoCanBang);
  int tC = constrain(-pVal - rVal, -tocDoCanBang, tocDoCanBang);
  int tD = constrain(-pVal + rVal, -tocDoCanBang, tocDoCanBang);

  setLiftSpeed(tA, tB, tC, tD);
}
