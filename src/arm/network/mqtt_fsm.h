#pragma once

#include <Arduino.h>

enum MqttFsmState {
  ROBOT_MQTT_DISCONNECTED,
  ROBOT_MQTT_CONNECTING,
  ROBOT_MQTT_CONNECTED,
  ROBOT_MQTT_FAILED
};

void mqttPublishSafe(const char *topic, const String &payload);
void mqttPublishStatus();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void mqttFsmTick();

MqttFsmState mqttGetState();
int mqttGetRetryCount();
