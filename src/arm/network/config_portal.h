#pragma once

#include <Arduino.h>

struct WifiConfig {
  String ssid;
  String password;
  String mqttServer;
  int mqttPort;
};

extern WifiConfig wifiCfg;
extern bool portalMode;

bool loadConfig();
bool saveConfigAtomic(const String &ssid, const String &pass, const String &mqttIp, int mqttPort);
void clearSavedConfig();
void startCaptivePortal();
void handlePortal();
