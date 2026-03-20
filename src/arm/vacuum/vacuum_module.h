#pragma once

#include <Arduino.h>

void initVacuum();
void startVacuum();
void stopVacuum();
void startVacuumSafe();
void checkVacuumFailsafe();