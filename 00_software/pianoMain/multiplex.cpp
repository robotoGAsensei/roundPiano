#include "multiplex.h"

#include <esp32-hal.h>  // C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\cores\esp32
#include <stdio.h>

const uint32_t DOUT[4] = {19, 21, 22, 23};  // 13,12 を使うとシリアル通信ができなくなるので注意

void multiplex::output(uint32_t address) {
  digitalWrite(DOUT[0], 0x1 & address);
  digitalWrite(DOUT[1], 0x1 & (address >> 1));
  digitalWrite(DOUT[2], 0x1 & (address >> 2));
  digitalWrite(DOUT[3], 0x1 & (address >> 3));
}

void multiplex::init(void) {
  for (int j = 0; j < 4; j++) pinMode(DOUT[j], OUTPUT);
}