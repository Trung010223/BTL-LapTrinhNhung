#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

void sfInit();
bool sfEnqueue(const char *topic, const String &payload);
int sfFlush(PubSubClient &mqtt);
int sfPending();

uint32_t nextPacketId();
