#ifndef XE_MOTOR_H
#define XE_MOTOR_H

void initMotor();
int rampTo(int cur, int target, int step);
void setDriveSpeed(int leftSpeed, int rightSpeed);
void setLiftSpeed(int motorA, int motorB, int motorC, int motorD);
void stopAllMotors();
void testIN3IN4Mode();  // Debug mode: cycle IN3/IN4 pins for 60 seconds

extern int curA, curB, curC, curD, curE;
extern int tocDoCanBang, tocDoBanhLai, buocRamp;

#endif
