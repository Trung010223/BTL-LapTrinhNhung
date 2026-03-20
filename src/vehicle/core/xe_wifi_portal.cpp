#include "xe_wifi_portal.h"

#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
Preferences prefs;
DNSServer dnsServer;
WebServer portalServer(80);
const byte DNS_PORT = 53;
const char *CONFIG_NAMESPACE = "xe-wifi";
const char *TEMP_NAMESPACE = "xe-wifi-t";

constexpr char PORTAL_HTML[] = R"HTML(
<!doctype html>
<html lang="vi">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Xe Can Bang WiFi Setup</title>
    <style>
      body {
        margin: 0;
        min-height: 100vh;
        display: grid;
        place-items: center;
        background: linear-gradient(180deg, #0a1620, #10293a);
        font-family: Arial, sans-serif;
        color: #eff7ff;
      }
      .card {
        width: min(92vw, 420px);
        padding: 24px;
        border-radius: 16px;
        background: rgba(10, 23, 34, 0.92);
        border: 1px solid rgba(120, 180, 220, 0.3);
        box-shadow: 0 18px 40px rgba(0, 0, 0, 0.28);
      }
      h1 {
        margin: 0 0 10px;
        font-size: 24px;
      }
      p {
        margin: 0 0 18px;
        color: #b8d2e8;
        line-height: 1.5;
      }
      label {
        display: block;
        margin-bottom: 14px;
        font-size: 14px;
        color: #b8d2e8;
      }
      input {
        width: 100%;
        margin-top: 6px;
        height: 42px;
        box-sizing: border-box;
        border: 1px solid rgba(140, 190, 220, 0.35);
        border-radius: 10px;
        background: #08131c;
        color: #eff7ff;
        padding: 0 12px;
      }
      button {
        width: 100%;
        height: 44px;
        border: 0;
        border-radius: 10px;
        background: #31c48d;
        color: #062312;
        font-size: 16px;
        font-weight: 700;
      }
      .hint {
        margin-top: 14px;
        font-size: 13px;
        color: #8fb3cf;
      }
    </style>
  </head>
  <body>
    <form class="card" method="POST" action="/save">
      <h1>Xe Can Bang Setup WiFi</h1>
      <p>ESP32 xe chua ket noi duoc WiFi. Nhap SSID va mat khau de luu cau hinh, sau do thiet bi se tu khoi dong lai.</p>
      <label>
        Ten WiFi (SSID)
        <input name="ssid" type="text" placeholder="Nhap ten WiFi" required />
      </label>
      <label>
        Mat khau WiFi
        <input name="pass" type="password" placeholder="Nhap mat khau" required />
      </label>
      <button type="submit">Luu va ket noi</button>
      <div class="hint">Truy cap bang dia chi http://192.168.4.1 khi ket noi vao AP XeCanBang-Setup.</div>
    </form>
  </body>
</html>
)HTML";

constexpr char PORTAL_SAVED_HTML[] = R"HTML(
<!doctype html>
<html lang="vi">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Da luu cau hinh</title>
    <style>
      body {
        margin: 0;
        min-height: 100vh;
        display: grid;
        place-items: center;
        background: #0c1823;
        color: #eff7ff;
        font-family: Arial, sans-serif;
      }
      .card {
        padding: 24px;
        border-radius: 16px;
        background: #10293a;
        text-align: center;
        width: min(92vw, 420px);
      }
    </style>
  </head>
  <body>
    <div class="card">
      <h1>Da luu cau hinh</h1>
      <p>ESP32 dang khoi dong lai de ket noi vao WiFi.</p>
    </div>
  </body>
</html>
)HTML";
}

VehicleWifiConfig vehicleWifiCfg;
bool vehiclePortalMode = false;

bool loadVehicleWifiConfig() {
  bool opened = prefs.begin(CONFIG_NAMESPACE, true);
  if (opened) {
    vehicleWifiCfg.ssid = prefs.getString("ssid", "");
    vehicleWifiCfg.password = prefs.getString("pass", "");
    prefs.end();
  } else {
    vehicleWifiCfg.ssid = "";
    vehicleWifiCfg.password = "";
  }

  bool valid = vehicleWifiCfg.ssid.length() > 0 && vehicleWifiCfg.password.length() > 0;
  Serial.printf("[XE-WIFI] Loaded config valid=%d ssid='%s'\n", valid, vehicleWifiCfg.ssid.c_str());
  return valid;
}

bool saveVehicleWifiConfig(const String &ssid, const String &password) {
  if (ssid.length() == 0 || password.length() == 0) {
    return false;
  }

  if (!prefs.begin(TEMP_NAMESPACE, false)) {
    return false;
  }

  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();

  if (!prefs.begin(TEMP_NAMESPACE, true)) {
    return false;
  }

  String savedSsid = prefs.getString("ssid", "");
  String savedPassword = prefs.getString("pass", "");
  prefs.end();

  if (savedSsid != ssid || savedPassword != password) {
    return false;
  }

  if (!prefs.begin(CONFIG_NAMESPACE, false)) {
    return false;
  }

  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();

  prefs.begin(TEMP_NAMESPACE, false);
  prefs.clear();
  prefs.end();

  vehicleWifiCfg.ssid = ssid;
  vehicleWifiCfg.password = password;
  return true;
}

bool connectVehicleWifi(int maxRetry) {
  if (vehicleWifiCfg.ssid.isEmpty()) {
    Serial.println("[XE-WIFI] Chua co cau hinh WiFi");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(vehicleWifiCfg.ssid.c_str(), vehicleWifiCfg.password.c_str());

  Serial.printf("[XE-WIFI] Dang ket noi '%s'", vehicleWifiCfg.ssid.c_str());
  for (int retry = 0; retry < maxRetry; retry++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.printf("[XE-WIFI] Ket noi OK. IP=%s channel=%d\n", WiFi.localIP().toString().c_str(), WiFi.channel());
      vehiclePortalMode = false;
      return true;
    }
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  Serial.println("[XE-WIFI] Ket noi that bai");
  WiFi.disconnect(true, true);
  return false;
}

void startVehicleCaptivePortal() {
  vehiclePortalMode = true;
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("XeCanBang-Setup", "12345678", 6);

  Serial.printf("[XE-WIFI] Portal AP ready at %s\n", WiFi.softAPIP().toString().c_str());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  portalServer.on("/", HTTP_GET, []() {
    portalServer.send_P(200, "text/html", PORTAL_HTML);
  });

  portalServer.on("/save", HTTP_POST, []() {
    String ssid = portalServer.arg("ssid");
    String password = portalServer.arg("pass");

    if (!saveVehicleWifiConfig(ssid, password)) {
      portalServer.send(400, "text/plain", "Khong luu duoc cau hinh");
      return;
    }

    portalServer.send_P(200, "text/html", PORTAL_SAVED_HTML);
    delay(1500);
    ESP.restart();
  });

  portalServer.onNotFound([]() {
    portalServer.sendHeader("Location", "/", true);
    portalServer.send(302, "text/plain", "");
  });

  portalServer.begin();
}

void handleVehiclePortal() {
  if (!vehiclePortalMode) {
    return;
  }

  dnsServer.processNextRequest();
  portalServer.handleClient();
}