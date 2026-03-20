#ifndef XE_IMU_H
#define XE_IMU_H

#include <MPU6050_light.h>

void initIMU();
void updateIMU();
void xuLyCanBang(float pitch, float roll);

extern float pitchEMA, rollEMA;
extern bool pitchActive, rollActive;
extern int curA, curB, curC, curD, curE;

#endif
