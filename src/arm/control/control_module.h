#pragma once

#include <Arduino.h>

bool cepProcess(const String &cmd);

int pwmToDeg(int pwmVal);
void printAngles();
void writeServo(int ch, int value);

void runAutoStep();
