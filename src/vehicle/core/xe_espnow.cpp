#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "xe_espnow.h"
#include "../../shared/robot_protocol.h"

uint8_t gatewayMAC[] = {0x30, 0xC6, 0xF7, 0x30, 0x79, 0xCC};
const uint8_t ESPNOW_CHANNEL = 6;

int latestCmd_speed = 0;
int latestCmd_direction = 0;
int latestCmd_lift = 0;
bool latestCmd_stop = true;
unsigned long lastCmdTime = 0;
unsigned long lastTelemetry = 0;

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  (void)len;
  CmdPacket cmd;
  memcpy(&cmd, data, sizeof(cmd));
  
  latestCmd_speed = cmd.speed;
  latestCmd_direction = cmd.direction;
  latestCmd_lift = cmd.lift;
  latestCmd_stop = cmd.stop;
  lastCmdTime = millis();
  
  Serial.println("[ESP-NOW] Nhan lenh tu Gateway");
}

void initESPNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, gatewayMAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.printf("[ESP-NOW] Node san sang (channel=%u)\n", ESPNOW_CHANNEL);
}

void sendTelemetry() {
  if (millis() - lastTelemetry > 500) {
    TelemetryPacket tele;
    tele.pitch = pitchEMA;
    tele.roll = rollEMA;
    tele.curA = curA; tele.curB = curB;
    tele.curC = curC; tele.curD = curD;
    tele.curE = curE;
    tele.isBalanced = (!pitchActive && !rollActive);

    esp_now_send(gatewayMAC, (uint8_t *)&tele, sizeof(tele));
    lastTelemetry = millis();
  }
}
