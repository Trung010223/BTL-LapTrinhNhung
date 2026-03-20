#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "xe_espnow.h"
#include "xe_imu.h"
#include "xe_failsafe.h"
#include "../../shared/robot_protocol.h"

uint8_t gatewayMAC[] = {0x30, 0xC6, 0xF7, 0x30, 0x79, 0xCC};
const uint8_t ESPNOW_CHANNEL = 6;

namespace {
bool espNowReady = false;
}

int latestCmd_speed = 0;
int latestCmd_direction = 0;
int latestCmd_lift = 0;
bool latestCmd_stop = true;
unsigned long lastCmdTime = 0;
unsigned long lastTelemetry = 0;

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  if (len != sizeof(CmdPacket)) {
    Serial.printf("[ESP-NOW] Bo qua packet lenh sai kich thuoc: %d != %u\n",
                  len,
                  static_cast<unsigned>(sizeof(CmdPacket)));
    return;
  }

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

  esp_err_t initRc = esp_now_init();
  if (initRc != ESP_OK) {
    Serial.printf("[ESP-NOW] Init that bai rc=%d\n", static_cast<int>(initRc));
    espNowReady = false;
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, gatewayMAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  esp_err_t peerRc = esp_now_add_peer(&peer);
  if (peerRc != ESP_OK) {
    Serial.printf("[ESP-NOW] Add peer that bai rc=%d\n", static_cast<int>(peerRc));
    espNowReady = false;
    return;
  }

  espNowReady = true;

  Serial.printf("[ESP-NOW] Node san sang (channel=%u)\n", ESPNOW_CHANNEL);
}

void sendTelemetry() {
  if (!espNowReady) {
    return;
  }

  if (millis() - lastTelemetry > 500) {
    TelemetryPacket tele;
    tele.pitch = pitchEMA;
    tele.roll = rollEMA;
    tele.curA = curA; tele.curB = curB;
    tele.curC = curC; tele.curD = curD;
    tele.curE = curE;
    tele.isBalanced = (!pitchActive && !rollActive);
    tele.feedbackFault = feedbackLoopFaultActive;
    strncpy(tele.postureLabel, getPostureLabel(), sizeof(tele.postureLabel) - 1);
    tele.postureLabel[sizeof(tele.postureLabel) - 1] = '\0';

    esp_err_t sendRc = esp_now_send(gatewayMAC, (uint8_t *)&tele, sizeof(tele));
    if (sendRc != ESP_OK) {
      Serial.printf("[ESP-NOW] Gui telemetry that bai rc=%d\n", static_cast<int>(sendRc));
    }
    lastTelemetry = millis();
  }
}
