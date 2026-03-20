#pragma once

#include <stdint.h>

// Telemetry data structure for vehicle status
typedef struct {
  float pitch;           // Pitch angle in degrees
  float roll;            // Roll angle in degrees
  int16_t curA;          // Current measurement A
  int16_t curB;          // Current measurement B
  int16_t curC;          // Current measurement C
  int16_t curD;          // Current measurement D
  int16_t curE;          // Current measurement E
  bool isBalanced;       // Balance status flag
  bool feedbackFault;    // Feedback loop fault flag
  char postureLabel[16]; // Semantic posture label
} TelemetryPacket;

// Command data structure for vehicle commands
typedef struct {
  int16_t speed;         // Speed command
  int16_t direction;     // Direction command
  int16_t lift;          // Lift command
  bool stop;             // Stop flag
} CmdPacket;
