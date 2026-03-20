#ifndef XE_WIFI_PORTAL_H
#define XE_WIFI_PORTAL_H

#include <Arduino.h>

struct VehicleWifiConfig {
  String ssid;
  String password;
};

extern VehicleWifiConfig vehicleWifiCfg;
extern bool vehiclePortalMode;

bool loadVehicleWifiConfig();
bool saveVehicleWifiConfig(const String &ssid, const String &password);
bool connectVehicleWifi(int maxRetry = 20);
void startVehicleCaptivePortal();
void handleVehiclePortal();

#endif