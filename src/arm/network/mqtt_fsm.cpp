#include "mqtt_fsm.h"

#include <ArduinoJson.h>

#include "../core/canh_tay_context.h"
#include "config_portal.h"
#include "../control/control_module.h"
#include "../vacuum/vacuum_module.h"
#include "store_forward.h"
#include "../ui/websocket_module.h"

namespace {
const unsigned long BACKOFF_BASE_MS = 2000;
const unsigned long BACKOFF_MAX_MS = 30000;
const unsigned long ACK_TIMEOUT_MS = 1500;
const int ACK_MAX_RETRY = 3;
const int MAX_PENDING_STATUS = 8;
const char *STATUS_ACK_TOPIC = "gateway/status_ack";

MqttFsmState mqttFsmState = ROBOT_MQTT_DISCONNECTED;
int backoffRetryCount = 0;
unsigned long backoffUntil = 0;
bool servoCommandSubscribed = false;
bool statusAckSubscribed = false;
unsigned long lastSubAttemptMs = 0;

struct PendingStatusPublish {
  bool used;
  uint32_t packetId;
  char topic[64];
  char payload[320];
  unsigned long lastSentAt;
  int retryCount;
};

PendingStatusPublish pendingPublishes[MAX_PENDING_STATUS] = {};

unsigned long calcBackoff() {
  unsigned long delay = BACKOFF_BASE_MS;
  for (int i = 0; i < backoffRetryCount; i++) {
    delay *= 2;
    if (delay >= BACKOFF_MAX_MS) {
      delay = BACKOFF_MAX_MS;
      break;
    }
  }
  return delay;
}

String attachPacketId(const String &payload, uint32_t packetId) {
  if (payload.length() > 1 && payload.endsWith("}")) {
    String enriched = payload;
    enriched.remove(enriched.length() - 1);
    enriched += ",\"packetId\":" + String(packetId) + "}";
    return enriched;
  }

  JsonDocument wrapper;
  wrapper["payload"] = payload;
  wrapper["packetId"] = packetId;
  String enriched;
  serializeJson(wrapper, enriched);
  return enriched;
}

int findPendingIndexByPacketId(uint32_t packetId) {
  for (int index = 0; index < MAX_PENDING_STATUS; index++) {
    if (pendingPublishes[index].used && pendingPublishes[index].packetId == packetId) {
      return index;
    }
  }
  return -1;
}

int allocatePendingIndex() {
  for (int index = 0; index < MAX_PENDING_STATUS; index++) {
    if (!pendingPublishes[index].used) {
      return index;
    }
  }

  return -1;
}

int findOldestPendingIndex() {
  unsigned long oldestTs = millis();
  int oldestIndex = 0;
  for (int index = 0; index < MAX_PENDING_STATUS; index++) {
    if (pendingPublishes[index].lastSentAt <= oldestTs) {
      oldestTs = pendingPublishes[index].lastSentAt;
      oldestIndex = index;
    }
  }
  return oldestIndex;
}

void clearPendingIndex(int index) {
  if (index < 0 || index >= MAX_PENDING_STATUS) {
    return;
  }
  pendingPublishes[index].used = false;
  pendingPublishes[index].packetId = 0;
  pendingPublishes[index].topic[0] = '\0';
  pendingPublishes[index].payload[0] = '\0';
  pendingPublishes[index].lastSentAt = 0;
  pendingPublishes[index].retryCount = 0;
}

int rememberPendingPublish(uint32_t packetId, const char *topic, const String &payload) {
  int index = allocatePendingIndex();
  if (index < 0) {
    index = findOldestPendingIndex();
    Serial.printf("[MQTT] Pending ACK full -> move PID=%lu topic=%s to Store&Forward\n",
                  pendingPublishes[index].packetId,
                  pendingPublishes[index].topic);
    sfEnqueue(pendingPublishes[index].topic, String(pendingPublishes[index].payload));
  }

  pendingPublishes[index].used = true;
  pendingPublishes[index].packetId = packetId;
  strncpy(pendingPublishes[index].topic, topic, sizeof(pendingPublishes[index].topic) - 1);
  pendingPublishes[index].topic[sizeof(pendingPublishes[index].topic) - 1] = '\0';
  strncpy(pendingPublishes[index].payload, payload.c_str(), sizeof(pendingPublishes[index].payload) - 1);
  pendingPublishes[index].payload[sizeof(pendingPublishes[index].payload) - 1] = '\0';
  pendingPublishes[index].lastSentAt = millis();
  pendingPublishes[index].retryCount = 0;
  return index;
}

bool publishPendingIndex(int index, bool countRetry) {
  if (index < 0 || index >= MAX_PENDING_STATUS || !pendingPublishes[index].used) {
    return false;
  }

  if (countRetry) {
    pendingPublishes[index].retryCount++;
  }

  bool ok = mqtt.publish(pendingPublishes[index].topic, pendingPublishes[index].payload);
  pendingPublishes[index].lastSentAt = millis();
  return ok;
}

void processStatusAck(const JsonDocument &doc) {
  uint32_t packetId = doc["packetId"] | 0;
  if (packetId == 0) {
    return;
  }

  int index = findPendingIndexByPacketId(packetId);
  if (index >= 0) {
    Serial.printf("[MQTT] ACK packetId=%lu topic=%s\n", packetId, pendingPublishes[index].topic);
    clearPendingIndex(index);
  }
}
}

void mqttPublishSafe(const char *topic, const String &payload) {
  uint32_t pid = nextPacketId();
  String enrichedPayload = attachPacketId(payload, pid);
  if (mqtt.connected()) {
    int pendingIndex = rememberPendingPublish(pid, topic, enrichedPayload);
    bool ok = publishPendingIndex(pendingIndex, false);
    Serial.printf("[MQTT] Publish PID=%lu %s: %s\n", pid, ok ? "OK" : "FAIL", topic);
    if (!ok) {
      clearPendingIndex(pendingIndex);
      sfEnqueue(topic, enrichedPayload);
    }
  } else {
    sfEnqueue(topic, enrichedPayload);
    Serial.printf("[SF] Mat MQTT -> Luu Queue PID=%lu: %s\n", pid, topic);
  }
}

void mqttPublishStatus() {
  JsonDocument doc;
  doc["vacuum"] = vacuumRunning ? "on" : "off";
  doc["autoMode"] = autoMode;

  JsonArray arr = doc["servo"].to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    JsonObject s = arr.add<JsonObject>();
    s["ch"] = i;
    s["pwm"] = pos[i];
    s["deg"] = pwmToDeg(pos[i]);
  }

  String payload;
  serializeJson(doc, payload);
  mqttPublishSafe("servo/status", payload);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.printf("[MQTT] RX topic=%s payload=%s\n", topic, msg.c_str());

  JsonDocument doc;
  if (deserializeJson(doc, msg)) {
    int ch = -1;
    int val = -1;
    char cmdBuf[16] = {0};
    if (sscanf(msg.c_str(), "{cmd:%15[^,],ch:%d,val:%d}", cmdBuf, &ch, &val) == 3) {
      doc["cmd"] = String(cmdBuf);
      doc["ch"] = ch;
      doc["val"] = val;
      Serial.printf("[MQTT] RX fallback parse OK cmd=%s ch=%d val=%d\n", cmdBuf, ch, val);
    } else {
      Serial.println("[MQTT] RX parse JSON FAIL");
      return;
    }
  }

  if (String(topic) == STATUS_ACK_TOPIC) {
    processStatusAck(doc);
    return;
  }

  String cmd = doc["cmd"].as<String>();

  if (cmd == "cut") {
    // PCA9685 setup: cut action is a quick close/open on gripper servo (CH3).
    autoMode = false;
    writeServo(3, 510);
    delay(250);
    writeServo(3, 410);
    mqttPublishStatus();
  } else if (cmd == "auto") {
    if (cepProcess("auto")) {
      resetAuto = true;
      autoMode = true;
      mqttPublishStatus();
    } else {
      JsonDocument nd;
      nd["type"] = "alert";
      nd["msg"] = "Gui lenh AUTO lan 2 de xac nhan (trong 3s)";
      nd["level"] = "info";
      String no;
      serializeJson(nd, no);
      webSocket.broadcastTXT(no);
    }
  } else if (cmd == "stop") {
    autoMode = false;
    stopVacuum();
    sendCmdToVehicle(0, 0, 0, true);
    broadcastVacuumState();
    mqttPublishStatus();
  } else if (cmd == "home") {
    autoMode = false;
    writeServo(0, 330);
    writeServo(1, 150);
    writeServo(2, 300);
    writeServo(3, 410);
    mqttPublishStatus();
  } else if (cmd == "servo") {
    int ch = doc["ch"] | -1;
    int val = doc["val"] | -1;
    Serial.printf("[MQTT] CMD servo ch=%d val=%d\n", ch, val);
    writeServo(ch, val);
    mqttPublishStatus();
  } else if (cmd == "vehicle") {
    sendCmdToVehicle(
      doc["speed"] | 0,
      doc["direction"] | 0,
      doc["lift"] | 0,
      doc["stop"] | false
    );
  }
}

void mqttFsmTick() {
  unsigned long now = millis();

  switch (mqttFsmState) {
    case ROBOT_MQTT_DISCONNECTED:
      if (now >= backoffUntil) {
        mqttFsmState = ROBOT_MQTT_CONNECTING;
        Serial.printf("[FSM] DISCONNECTED -> CONNECTING (retry #%d)\n", backoffRetryCount + 1);
      }
      break;

    case ROBOT_MQTT_CONNECTING: {
      mqtt.setServer(wifiCfg.mqttServer.c_str(), wifiCfg.mqttPort);
      String clientId = "ESP32-RobotArm-" + String(random(0xffff), HEX);

      if (mqtt.connect(clientId.c_str())) {
        mqttFsmState = ROBOT_MQTT_CONNECTED;
        backoffRetryCount = 0;
        backoffUntil = 0;
        servoCommandSubscribed = false;
        statusAckSubscribed = false;

        servoCommandSubscribed = mqtt.subscribe("servo/command");
        statusAckSubscribed = mqtt.subscribe(STATUS_ACK_TOPIC);
        lastSubAttemptMs = now;
        Serial.println("[FSM] CONNECTING -> CONNECTED");
        Serial.printf("[FSM] Client: %s\n", clientId.c_str());
        Serial.printf("[FSM] Subscribe servo/command: %s\n", servoCommandSubscribed ? "OK" : "FAIL");
        Serial.printf("[FSM] Subscribe %s: %s\n", STATUS_ACK_TOPIC, statusAckSubscribed ? "OK" : "FAIL");

        mqttPublishStatus();

        int sentBacklog = sfFlush(mqtt);

        JsonDocument nd;
        nd["type"] = "alert";
        nd["msg"] = "MQTT ket noi lai - da gui " + String(sentBacklog) + " ban ghi ton dong";
        nd["level"] = "info";
        String no;
        serializeJson(nd, no);
        webSocket.broadcastTXT(no);
      } else {
        backoffRetryCount++;
        unsigned long backoffMs = calcBackoff();
        backoffUntil = now + backoffMs;
        mqttFsmState = ROBOT_MQTT_FAILED;

        Serial.printf("[FSM] CONNECTING -> FAILED (rc=%d) retry#%d backoff=%lums\n",
                      mqtt.state(), backoffRetryCount, backoffMs);
      }
      break;
    }

    case ROBOT_MQTT_CONNECTED:
      if (!mqtt.connected()) {
        mqttFsmState = ROBOT_MQTT_DISCONNECTED;
        servoCommandSubscribed = false;
        statusAckSubscribed = false;
        backoffUntil = millis() + calcBackoff();
        Serial.println("[FSM] CONNECTED -> DISCONNECTED");

        JsonDocument nd;
        nd["type"] = "alert";
        nd["msg"] = "MQTT mat ket noi - dang Store & Forward";
        nd["level"] = "warn";
        String no;
        serializeJson(nd, no);
        webSocket.broadcastTXT(no);
      } else {
        if (!servoCommandSubscribed && now - lastSubAttemptMs >= 5000) {
          servoCommandSubscribed = mqtt.subscribe("servo/command");
          lastSubAttemptMs = now;
          Serial.printf("[FSM] Retry subscribe servo/command: %s\n", servoCommandSubscribed ? "OK" : "FAIL");
        }
        if (!statusAckSubscribed && now - lastSubAttemptMs >= 5000) {
          statusAckSubscribed = mqtt.subscribe(STATUS_ACK_TOPIC);
          lastSubAttemptMs = now;
          Serial.printf("[FSM] Retry subscribe %s: %s\n", STATUS_ACK_TOPIC, statusAckSubscribed ? "OK" : "FAIL");
        }

        for (int index = 0; index < MAX_PENDING_STATUS; index++) {
          if (!pendingPublishes[index].used) {
            continue;
          }
          if (now - pendingPublishes[index].lastSentAt < ACK_TIMEOUT_MS) {
            continue;
          }
          if (pendingPublishes[index].retryCount >= ACK_MAX_RETRY) {
            Serial.printf("[MQTT] ACK timeout PID=%lu -> Store&Forward %s\n",
                          pendingPublishes[index].packetId, pendingPublishes[index].topic);
            sfEnqueue(pendingPublishes[index].topic, String(pendingPublishes[index].payload));
            clearPendingIndex(index);
            continue;
          }

          bool ok = publishPendingIndex(index, true);
          Serial.printf("[MQTT] Retry PID=%lu #%d %s\n",
                        pendingPublishes[index].packetId,
                        pendingPublishes[index].retryCount,
                        ok ? "OK" : "FAIL");
        }
      }
      break;

    case ROBOT_MQTT_FAILED:
      if (now >= backoffUntil) {
        mqttFsmState = ROBOT_MQTT_DISCONNECTED;
        Serial.println("[FSM] FAILED -> DISCONNECTED (backoff het, thu lai)");
      }
      break;
  }
}

MqttFsmState mqttGetState() {
  return mqttFsmState;
}

int mqttGetRetryCount() {
  return backoffRetryCount;
}

int mqttGetAwaitingAckCount() {
  int count = 0;
  for (int index = 0; index < MAX_PENDING_STATUS; index++) {
    if (pendingPublishes[index].used) {
      count++;
    }
  }
  return count;
}
