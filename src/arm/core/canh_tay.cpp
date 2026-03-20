#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <ArduinoJson.h>

#include "canh_tay_context.h"
#include "../network/config_portal.h"
#include "../control/control_module.h"
#include "../vacuum/vacuum_module.h"
#include "../network/mqtt_fsm.h"
#include "../network/store_forward.h"
#include "../ui/dashboard_css.h"
#include "../ui/dashboard_html.h"
#include "../ui/dashboard_js.h"
#include "../ui/websocket_module.h"
#include "canh_tay.h"

#define I2C_SDA 21
#define I2C_SCL 22

namespace {
const char *ALT_WIFI_SSID = "QuangTrung11111111";
const char *ALT_WIFI_PASS = "11111111";
const uint8_t PCA_CANDIDATES[] = {0x40, 0x41, 0x42, 0x43};

const char *authModeName(wifi_auth_mode_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
#ifdef WIFI_AUTH_WPA2_ENTERPRISE
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
#endif
#ifdef WIFI_AUTH_WPA3_PSK
    case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
#endif
#ifdef WIFI_AUTH_WPA2_WPA3_PSK
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
#endif
    default: return "UNKNOWN";
  }
}

void printTargetSsidAuth(const String &ssid) {
  int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    Serial.println("[WIFI] Khong quet duoc danh sach mang");
    return;
  }
  bool found = false;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid) {
      found = true;
      wifi_auth_mode_t mode = WiFi.encryptionType(i);
      Serial.printf("[WIFI] SSID '%s' auth=%s RSSI=%d\n", ssid.c_str(), authModeName(mode), WiFi.RSSI(i));
#ifdef WIFI_AUTH_WPA3_PSK
      if (mode == WIFI_AUTH_WPA3_PSK) {
        Serial.println("[WIFI] Canh bao: WPA3-Personal co the khong tuong thich voi ESP32 nay");
      }
#endif
    }
  }
  if (!found) {
    Serial.printf("[WIFI] Khong tim thay SSID '%s' trong danh sach quet\n", ssid.c_str());
  }
}

bool tryConnectWifi(const String &ssid, const String &password, int maxRetry) {
  printTargetSsidAuth(ssid);
  Serial.printf("[WIFI] Connecting to '%s'...\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < maxRetry) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

int detectPcaAddress() {
  for (uint8_t addr : PCA_CANDIDATES) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      return addr;
    }
  }
  return -1;
}

bool setupPca9685() {
  int detectedAddr = detectPcaAddress();
  if (detectedAddr < 0) {
    Serial.println("[PCA9685] Khong tim thay module tren I2C (thu 0x40..0x43)");
    Serial.println("[PCA9685] Kiem tra day SDA/SCL, GND chung, VCC logic va nguon servo");
    return false;
  }

  pwm = Adafruit_PWMServoDriver((uint8_t)detectedAddr, Wire);
  pwm.begin();
  pwm.setPWMFreq(60);
  delay(200);
  Serial.printf("[PCA9685] Da ket noi tai dia chi 0x%02X\n", detectedAddr);
  return true;
}
}

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
WiFiClient espClient;
PubSubClient mqtt(espClient);
uint8_t nodeMAC[] = {0xF0, 0x24, 0xF9, 0x45, 0xBE, 0xD4};

TelemetryPacket latestTele = {0, 0, 0, 0, 0, 0, 0, false};
bool newTeleAvailable = false;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);
bool pca9685Ready = false;
int pos[4] = {330, 150, 300, 410};
bool autoMode = false;
bool resetAuto = false;
bool vacuumRunning = false;

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  memcpy(&latestTele, data, sizeof(latestTele));
  newTeleAvailable = true;

  String payload = "{\"source\":\"vehicle\","
    "\"pitch\":" + String(latestTele.pitch, 2) +
    ",\"roll\":" + String(latestTele.roll, 2) +
    ",\"curA\":" + String(latestTele.curA) +
    ",\"curB\":" + String(latestTele.curB) +
    ",\"curC\":" + String(latestTele.curC) +
    ",\"curD\":" + String(latestTele.curD) +
    ",\"curE\":" + String(latestTele.curE) +
    ",\"isBalanced\":" + String(latestTele.isBalanced ? "true" : "false") + "}";

  if (mqtt.connected()) {
    mqtt.publish("vehicle/status", payload.c_str());
  }
  Serial.println("[ESP-NOW] " + payload);
}

void sendCmdToVehicle(int speed, int direction, int lift, bool stop) {
  CmdPacket cmd = {speed, direction, lift, stop};
  esp_err_t sendRc = esp_now_send(nodeMAC, (uint8_t *)&cmd, sizeof(cmd));

  JsonDocument ack;
  ack["source"] = "gateway";
  ack["speed"] = speed;
  ack["direction"] = direction;
  ack["lift"] = lift;
  ack["stop"] = stop;
  ack["espnow_rc"] = (int)sendRc;

  String ackPayload;
  serializeJson(ack, ackPayload);
  Serial.println("[ESP-NOW][ACK] " + ackPayload);

  if (mqtt.connected()) {
    mqtt.publish("vehicle/cmd_ack", ackPayload.c_str());
  }
}

void initCanhTay() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  initVacuum();

  delay(500);
  pca9685Ready = setupPca9685();
  if (pca9685Ready) {
    writeServo(0, 330);
    writeServo(1, 150);
    writeServo(2, 300);
    writeServo(3, 410);
  }

  sfInit();

  bool hasConfig = loadConfig();
  if (!hasConfig) {
    startCaptivePortal();
    while (portalMode) {
      handlePortal();
      delay(10);
    }
    return;
  }

  bool connected = tryConnectWifi(wifiCfg.ssid, wifiCfg.password, 20);

  if (!connected && wifiCfg.ssid != ALT_WIFI_SSID) {
    Serial.printf("[WIFI] Thu SSID du phong '%s'...\n", ALT_WIFI_SSID);
    connected = tryConnectWifi(String(ALT_WIFI_SSID), String(ALT_WIFI_PASS), 20);
    if (connected) {
      wifiCfg.ssid = ALT_WIFI_SSID;
      wifiCfg.password = ALT_WIFI_PASS;
      saveConfigAtomic(wifiCfg.ssid, wifiCfg.password, wifiCfg.mqttServer, wifiCfg.mqttPort);
      Serial.println("[WIFI] Da luu config tu SSID du phong");
    }
  }

  if (!connected) {
    Serial.println("[WIFI] That bai -> Captive Portal");
    startCaptivePortal();
    while (true) {
      handlePortal();
      delay(10);
    }
  }

  Serial.println("\n[WIFI] IP: " + WiFi.localIP().toString());

  mqtt.setCallback(mqttCallback);

  server.on("/", []() { server.send_P(200, "text/html", DASHBOARD_HTML); });
  server.on("/app.css", []() { server.send_P(200, "text/css", DASHBOARD_CSS); });
  server.on("/app.js", []() { server.send_P(200, "application/javascript", DASHBOARD_JS); });
  server.on("/reset-config", HTTP_GET, []() {
    clearSavedConfig();
    server.send(200, "text/plain", "Da xoa config! Khoi dong lai...");
    delay(3000);
    ESP.restart();
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, nodeMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("[SETUP] Hoan tat!");
  printAngles();
}

void updateCanhTay() {
  if (portalMode) {
    handlePortal();
    return;
  }

  server.handleClient();
  webSocket.loop();

  mqttFsmTick();
  if (mqttGetState() == ROBOT_MQTT_CONNECTED) {
    mqtt.loop();
  }

  if (newTeleAvailable) {
    newTeleAvailable = false;
    broadcastTelemetry();
  }

  if (autoMode) {
    runAutoStep();
  }

  checkVacuumFailsafe();

  static unsigned long lastQueueBroadcast = 0;
  if (millis() - lastQueueBroadcast > 5000) {
    broadcastQueueStatus();
    lastQueueBroadcast = millis();
  }
}
