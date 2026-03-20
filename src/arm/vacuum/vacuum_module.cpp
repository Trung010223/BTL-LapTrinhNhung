#include "vacuum_module.h"

#include <ArduinoJson.h>

#include "../core/canh_tay_context.h"
#include "../network/mqtt_fsm.h"
#include "../ui/websocket_module.h"

namespace {
#define VACUUM_IN1 25
#define VACUUM_IN2 26
#define VACUUM_ENA 27

const unsigned long MAX_VACUUM_RUNTIME = 5UL * 60 * 1000;
unsigned long vacuumStartTime = 0;
}

void initVacuum() {
  pinMode(VACUUM_IN1, OUTPUT);
  pinMode(VACUUM_IN2, OUTPUT);
  pinMode(VACUUM_ENA, OUTPUT);
  digitalWrite(VACUUM_IN1, LOW);
  digitalWrite(VACUUM_IN2, LOW);
  digitalWrite(VACUUM_ENA, LOW);
}

void startVacuum() {
  digitalWrite(VACUUM_ENA, HIGH);
  digitalWrite(VACUUM_IN1, HIGH);
  digitalWrite(VACUUM_IN2, LOW);
  vacuumRunning = true;
  Serial.println("[VACUUM] BAT");
}

void stopVacuum() {
  digitalWrite(VACUUM_IN1, LOW);
  digitalWrite(VACUUM_IN2, LOW);
  digitalWrite(VACUUM_ENA, LOW);
  vacuumRunning = false;
  Serial.println("[VACUUM] TAT");
}

void startVacuumSafe() {
  startVacuum();
  vacuumStartTime = millis();
  Serial.printf("[FAILSAFE] Quat hut bat - tu tat sau %lus\n", MAX_VACUUM_RUNTIME / 1000);
}

void checkVacuumFailsafe() {
  if (!vacuumRunning) {
    return;
  }

  if (millis() - vacuumStartTime >= MAX_VACUUM_RUNTIME) {
    stopVacuum();
    broadcastVacuumState();
    mqttPublishStatus();
    Serial.println("[FAILSAFE] Quat hut qua 5 phut -> TAT!");

    JsonDocument doc;
    doc["type"] = "alert";
    doc["msg"] = "Quat hut tu dong tat sau 5 phut (Failsafe)";
    doc["level"] = "warn";

    String out;
    serializeJson(doc, out);
    webSocket.broadcastTXT(out);
  }
}