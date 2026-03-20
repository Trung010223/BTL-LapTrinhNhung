#include "websocket_module.h"

#include <ArduinoJson.h>

#include "../core/canh_tay_context.h"
#include "../control/control_module.h"
#include "../vacuum/vacuum_module.h"
#include "../network/mqtt_fsm.h"
#include "../network/store_forward.h"

void broadcastVacuumState() {
  JsonDocument doc;
  doc["type"] = "vacuum";
  doc["state"] = vacuumRunning ? "on" : "off";

  String out;
  serializeJson(doc, out);
  webSocket.broadcastTXT(out);
}

void broadcastTelemetry() {
  JsonDocument doc;
  doc["type"] = "tele";
  doc["pitch"] = latestTele.pitch;
  doc["roll"] = latestTele.roll;
  doc["curA"] = latestTele.curA;
  doc["curB"] = latestTele.curB;
  doc["curC"] = latestTele.curC;
  doc["curD"] = latestTele.curD;
  doc["curE"] = latestTele.curE;
  doc["isBalanced"] = latestTele.isBalanced;

  String out;
  serializeJson(doc, out);
  webSocket.broadcastTXT(out);
}

void broadcastQueueStatus() {
  JsonDocument doc;
  doc["type"] = "queue";
  doc["pending"] = sfPending();
  doc["mqttState"] = (mqttGetState() == ROBOT_MQTT_CONNECTED) ? "connected" : "disconnected";
  doc["retries"] = mqttGetRetryCount();

  String out;
  serializeJson(doc, out);
  webSocket.broadcastTXT(out);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    JsonDocument doc;
    doc["type"] = "state";

    JsonArray arr = doc["pos"].to<JsonArray>();
    for (int i = 0; i < 4; i++) {
      arr.add(pos[i]);
    }

    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(num, out);

    broadcastVacuumState();
    broadcastTelemetry();
    broadcastQueueStatus();
    return;
  }

  if (type != WStype_TEXT) {
    return;
  }

  JsonDocument doc;
  deserializeJson(doc, payload, length);
  String t = doc["type"].as<String>();

  if (t == "servo") {
    writeServo(doc["ch"], doc["val"]);
    mqttPublishStatus();
    return;
  }

  if (t == "cmd") {
    String c = doc["cmd"].as<String>();

    if (c == "home") {
      autoMode = false;
      writeServo(0, 330);
      writeServo(1, 150);
      writeServo(2, 300);
      writeServo(3, 410);
    } else if (c == "auto") {
      if (cepProcess("auto")) {
        resetAuto = true;
        autoMode = true;
      } else {
        JsonDocument nd;
        nd["type"] = "alert";
        nd["msg"] = "Nhan AUTO lan 2 de xac nhan (trong 3s)";
        nd["level"] = "info";
        String no;
        serializeJson(nd, no);
        webSocket.sendTXT(num, no);
      }
    } else if (c == "stop") {
      autoMode = false;
      stopVacuum();
      sendCmdToVehicle(0, 0, 0, true);
      broadcastVacuumState();
    } else if (c == "cut") {
      autoMode = false;
      writeServo(3, 510);
      delay(250);
      writeServo(3, 410);
    }

    mqttPublishStatus();
    return;
  }

  if (t == "vehicle") {
    sendCmdToVehicle(
      doc["speed"] | 0,
      doc["direction"] | 0,
      doc["lift"] | 0,
      doc["stop"] | false
    );
  }
}
