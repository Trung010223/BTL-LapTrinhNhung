#pragma once

#include <WebSocketsServer.h>

void broadcastVacuumState();
void broadcastTelemetry();
void broadcastQueueStatus();
void broadcastAlert(const String &message, const char *level = "warn");

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
