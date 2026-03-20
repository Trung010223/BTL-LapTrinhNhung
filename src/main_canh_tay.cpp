#include <Arduino.h>
#include "arm/core/canh_tay.h"

void setup() {
  initCanhTay();
}

void loop() {
  updateCanhTay();
  delay(20);
}
