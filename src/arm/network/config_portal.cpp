#include "config_portal.h"

#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "../ui/web_content.h"

namespace {
Preferences prefs;
DNSServer dnsServer;
WebServer portalServer(80);
const byte DNS_PORT = 53;

const char *DEFAULT_WIFI_SSID = "Nhà 15 Trinh Lương";
const char *DEFAULT_WIFI_PASS = "88888888";
const char *DEFAULT_MQTT_SERVER = "192.168.100.248";
const int DEFAULT_MQTT_PORT = 1884;
const bool FORCE_DEFAULT_CONFIG = true;
}

WifiConfig wifiCfg;
bool portalMode = false;

bool loadConfig() {
  bool opened = prefs.begin("robot-cfg", true);
  if (opened) {
    wifiCfg.ssid = prefs.getString("ssid", "");
    wifiCfg.password = prefs.getString("pass", "");
    wifiCfg.mqttServer = prefs.getString("mqtt_ip", DEFAULT_MQTT_SERVER);
    wifiCfg.mqttPort = prefs.getInt("mqtt_port", DEFAULT_MQTT_PORT);
    prefs.end();
  } else {
    wifiCfg.ssid = "";
    wifiCfg.password = "";
    wifiCfg.mqttServer = DEFAULT_MQTT_SERVER;
    wifiCfg.mqttPort = DEFAULT_MQTT_PORT;
  }

  // Force defaults when required, otherwise fallback only when values are missing.
  if (FORCE_DEFAULT_CONFIG || wifiCfg.ssid.length() == 0) {
    wifiCfg.ssid = DEFAULT_WIFI_SSID;
  }
  if (FORCE_DEFAULT_CONFIG || wifiCfg.password.length() == 0) {
    wifiCfg.password = DEFAULT_WIFI_PASS;
  }
  if (FORCE_DEFAULT_CONFIG || wifiCfg.mqttServer.length() == 0) {
    wifiCfg.mqttServer = DEFAULT_MQTT_SERVER;
  }
  if (FORCE_DEFAULT_CONFIG || wifiCfg.mqttPort <= 0) {
    wifiCfg.mqttPort = DEFAULT_MQTT_PORT;
  }

  bool valid = (wifiCfg.ssid.length() > 0 && wifiCfg.password.length() > 0 && wifiCfg.mqttServer.length() > 0);
  Serial.printf("[CFG] SSID='%s' MQTT='%s:%d' valid=%d\n",
                wifiCfg.ssid.c_str(), wifiCfg.mqttServer.c_str(), wifiCfg.mqttPort, valid);
  return valid;
}

bool saveConfigAtomic(const String &ssid, const String &pass, const String &mqttIp, int mqttPort) {
  prefs.begin("robot-tmp", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("mqtt_ip", mqttIp);
  prefs.putInt("mqtt_port", mqttPort);
  prefs.end();

  prefs.begin("robot-tmp", true);
  String check = prefs.getString("ssid", "");
  prefs.end();

  if (check != ssid) {
    Serial.println("[CFG] Atomic FAILED");
    return false;
  }

  prefs.begin("robot-cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("mqtt_ip", mqttIp);
  prefs.putInt("mqtt_port", mqttPort);
  prefs.end();

  prefs.begin("robot-tmp", false);
  prefs.clear();
  prefs.end();

  Serial.println("[CFG] Atomic OK");
  return true;
}

void clearSavedConfig() {
  prefs.begin("robot-cfg", false);
  prefs.clear();
  prefs.end();
}

void startCaptivePortal() {
  portalMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("RobotARM-Setup", "12345678");

  Serial.print("[PORTAL] AP IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  portalServer.on("/", HTTP_GET, []() {
    portalServer.send_P(200, "text/html", PORTAL_HTML);
  });

  portalServer.onNotFound([]() {
    portalServer.sendHeader("Location", "/", true);
    portalServer.send(302, "text/plain", "");
  });

  portalServer.on("/save", HTTP_POST, []() {
    String ssid = portalServer.arg("ssid");
    String pass = portalServer.arg("pass");
    String mqttIp = portalServer.arg("mqtt_ip");
    int mqttPort = portalServer.arg("mqtt_port").toInt();

    if (ssid.length() == 0 || pass.length() == 0) {
      portalServer.send(400, "text/plain", "Thieu thong tin!");
      return;
    }

    if (mqttPort <= 0) {
      mqttPort = DEFAULT_MQTT_PORT;
    }

    if (saveConfigAtomic(ssid, pass, mqttIp, mqttPort)) {
      portalServer.send_P(200, "text/html", PORTAL_SAVED_HTML);
      delay(2000);
      ESP.restart();
    } else {
      portalServer.send(500, "text/plain", "Loi luu config!");
    }
  });

  portalServer.begin();
  Serial.println("[PORTAL] San sang tai http://192.168.4.1");
}

void handlePortal() {
  dnsServer.processNextRequest();
  portalServer.handleClient();
}
