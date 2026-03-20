#include "store_forward.h"

namespace {
const int SF_QUEUE_SIZE = 20;
const int SF_PAYLOAD_LEN = 256;

struct QueueItem {
  char topic[64];
  char payload[SF_PAYLOAD_LEN];
  bool used;
};

QueueItem sfQueue[SF_QUEUE_SIZE];
int sfQueueCount = 0;
uint32_t packetId = 0;
}

void sfInit() {
  for (int i = 0; i < SF_QUEUE_SIZE; i++) {
    sfQueue[i].used = false;
  }
  sfQueueCount = 0;
  Serial.printf("[SF] Queue khoi tao: %d slots\n", SF_QUEUE_SIZE);
}

bool sfEnqueue(const char *topic, const String &payload) {
  for (int i = 0; i < SF_QUEUE_SIZE; i++) {
    if (!sfQueue[i].used) {
      strncpy(sfQueue[i].topic, topic, sizeof(sfQueue[i].topic) - 1);
      strncpy(sfQueue[i].payload, payload.c_str(), sizeof(sfQueue[i].payload) - 1);
      sfQueue[i].topic[sizeof(sfQueue[i].topic) - 1] = '\0';
      sfQueue[i].payload[sizeof(sfQueue[i].payload) - 1] = '\0';
      sfQueue[i].used = true;
      sfQueueCount++;
      Serial.printf("[SF] Enqueue [%d/%d]: %s\n", sfQueueCount, SF_QUEUE_SIZE, topic);
      return true;
    }
  }

  Serial.println("[SF] Queue day -> drop ban ghi cu nhat");
  for (int i = 0; i < SF_QUEUE_SIZE - 1; i++) {
    sfQueue[i] = sfQueue[i + 1];
  }

  strncpy(sfQueue[SF_QUEUE_SIZE - 1].topic, topic, sizeof(sfQueue[0].topic) - 1);
  strncpy(sfQueue[SF_QUEUE_SIZE - 1].payload, payload.c_str(), sizeof(sfQueue[0].payload) - 1);
  sfQueue[SF_QUEUE_SIZE - 1].topic[sizeof(sfQueue[0].topic) - 1] = '\0';
  sfQueue[SF_QUEUE_SIZE - 1].payload[sizeof(sfQueue[0].payload) - 1] = '\0';
  sfQueue[SF_QUEUE_SIZE - 1].used = true;
  return false;
}

int sfFlush(PubSubClient &mqtt) {
  if (sfQueueCount == 0) {
    return 0;
  }

  Serial.printf("[SF] Flush %d ban ghi len MQTT...\n", sfQueueCount);
  int sent = 0;

  for (int i = 0; i < SF_QUEUE_SIZE; i++) {
    if (!sfQueue[i].used) {
      continue;
    }
    if (!mqtt.connected()) {
      Serial.println("[SF] Mat ket noi MQTT -> dung flush");
      break;
    }

    if (mqtt.publish(sfQueue[i].topic, sfQueue[i].payload)) {
      sfQueue[i].used = false;
      sfQueueCount--;
      sent++;
      Serial.printf("[SF] Sent [%d]: %s\n", sent, sfQueue[i].topic);
      delay(10);
    } else {
      Serial.printf("[SF] Publish that bai, giu lai ban ghi %d\n", i);
    }
  }

  Serial.printf("[SF] Flush xong: %d/%d gui thanh cong, con lai %d\n",
                sent, sent + sfQueueCount, sfQueueCount);
  return sent;
}

int sfPending() {
  return sfQueueCount;
}

uint32_t nextPacketId() {
  return ++packetId;
}
