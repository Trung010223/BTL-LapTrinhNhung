#include "control_module.h"

#include <Adafruit_PWMServoDriver.h>

#include "../core/canh_tay_context.h"
#include "../network/mqtt_fsm.h"

namespace {
const unsigned long CEP_CONFIRM_WINDOW = 3000;

struct CepState {
  String pendingCmd;
  unsigned long firstCmdTime;
  bool waitingConfirm;
};

CepState cep = {"", 0, false};

int rev(int value) {
  return 750 - value;
}
}

bool cepProcess(const String &cmd) {
  if (cmd == "stop" || cmd == "home") {
    cep.waitingConfirm = false;
    cep.pendingCmd = "";
    Serial.printf("[CEP] Khan cap '%s' -> thuc thi ngay\n", cmd.c_str());
    return true;
  }

  unsigned long now = millis();
  if (!cep.waitingConfirm) {
    cep.pendingCmd = cmd;
    cep.firstCmdTime = now;
    cep.waitingConfirm = true;
    Serial.printf("[CEP] Lan 1: '%s' -> gui lai trong 3s\n", cmd.c_str());
    return false;
  }

  if (now - cep.firstCmdTime > CEP_CONFIRM_WINDOW) {
    cep.pendingCmd = cmd;
    cep.firstCmdTime = now;
    Serial.printf("[CEP] Timeout -> reset cho '%s'\n", cmd.c_str());
    return false;
  }

  if (cep.pendingCmd == cmd) {
    cep.waitingConfirm = false;
    cep.pendingCmd = "";
    Serial.printf("[CEP] Xac nhan lan 2: '%s' -> THUC THI\n", cmd.c_str());
    return true;
  }

  cep.pendingCmd = cmd;
  cep.firstCmdTime = now;
  return false;
}

int pwmToDeg(int pwmVal) {
  return map(pwmVal, 150, 600, 0, 180);
}

void printAngles() {
  Serial.println("-----------------------------");
  Serial.printf("S1 Base     : %3d deg (PWM=%d)\n", pwmToDeg(pos[0]), pos[0]);
  Serial.printf("S2 Khop duoi: %3d deg (PWM=%d)\n", pwmToDeg(pos[1]), pos[1]);
  Serial.printf("S3 Khop tren: %3d deg (PWM=%d)\n", pwmToDeg(pos[2]), pos[2]);
  Serial.printf("S4 Tay gap  : %3d deg (PWM=%d)\n", pwmToDeg(pos[3]), pos[3]);
}

void writeServo(int ch, int value) {
  if (!pca9685Ready) {
    Serial.printf("[SERVO] Bo qua lenh ch=%d val=%d (PCA9685 chua san sang)\n", ch, value);
    return;
  }

  if (ch < 0 || ch > 3) {
    Serial.printf("[SERVO] Kenh khong hop le: %d\n", ch);
    return;
  }

  value = constrain(value, 150, 600);
  if (ch == 1 || ch == 2) {
    pwm.setPWM(ch, 0, rev(value));
  } else {
    pwm.setPWM(ch, 0, value);
  }
  pos[ch] = value;
}

void runAutoStep() {
  static int step = 0;
  static unsigned long t = 0;
  static int s1 = 330, s2 = 150, s3 = 300;

  if (resetAuto) {
    step = 0;
    s1 = 330;
    s2 = 150;
    s3 = 300;
    resetAuto = false;
  }

  if (millis() - t < 12) {
    return;
  }
  t = millis();

  switch (step) {
    case 0: if (s1 > 250) { s1--; writeServo(0, s1); } else step++; break;
    case 1: if (s2 < 380) { s2++; writeServo(1, s2); } else step++; break;
    case 2: if (s3 < 380) { s3++; writeServo(2, s3); } else step++; break;
    case 3: writeServo(3, 510); step++; t += 500; break;
    case 4: writeServo(3, 410); step++; break;
    case 5: if (s3 > 300) { s3--; writeServo(2, s3); } else step++; break;
    case 6: if (s2 > 150) { s2--; writeServo(1, s2); } else step++; break;
    case 7:
      if (s1 < 450) {
        s1++;
        writeServo(0, s1);
      } else {
        s1 = 330;
        s2 = 150;
        s3 = 300;
        step = 0;
        mqttPublishStatus();
      }
      break;
  }
}
