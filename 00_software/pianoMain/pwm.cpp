#include <esp32-hal.h>  // C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\cores\esp32
#include <stdio.h>

#include "HardwareSerial.h"
#include "pianoKey.h"
#include "pwm.h"
#include "sin_table.h"

const uint32_t PWMOUTPIN     = 26;
const uint32_t PWMCH         = 0;
const uint32_t PWMMAX        = 1023;
const float    SAMPLING_RFEQ = 20000.0f;  // 20[kHz]

void pwm::output(uint32_t time_count, pianoKey *pkey) {
  uint32_t waveNum = 0;
  float    result  = 0.0f;

  uint32_t index        = 0;
  uint32_t result_digit = 0;
  float    time         = 0.0f;

  // To avoid calculation error with float, limit the time_count range
  // time_count counts up to 20000 every 1s.
  // Limit the range of time_count within 16bit because multiplication with big number may iclude error.
  time_count = 0xffff & time_count;
  time       = (float)time_count / SAMPLING_RFEQ;

  for (uint32_t oct = 0; oct < OCTAVENUM; oct++) {
    for (uint32_t tone = 0; tone < TONENUM; tone++) {
      if (pkey->key[oct][tone].volume > 0) {
        // To keep the same loudness, devide the result by the number of synthesized wave
        waveNum++;
        // theta = 2PI * f * t
        // 0 < index < TABLE_NUM corresponds to 0 < theta < 2PI
        // sin_table(index) -> sin_table(TABLE_NUM * theta / 2PI)
        // sin(theta) = sin_table(index) = sin_table(TABLE_NUM * f * t)
        index = (uint32_t)((float)SIN_TABLE_NUM * pkey->key[oct][tone].freq * time);
        // calculate remainder by multiplexing index with TABLE_NUM
        index = (SIN_TABLE_NUM - 1) & index;
        result += sin_table[index];
        result /= (float)waveNum;
      }
    }
  }
  result_digit = (uint32_t)((float)PWMMAX * (1.0f + result) / 2.0f);
  ledcWrite(PWMCH, result_digit);

  // Serial.printf("%d %d %d %f\n", result_digit, time_count, waveNum, result);
}

void pwm::init(void) {
  pinMode(PWMOUTPIN, OUTPUT);
  ledcSetup(PWMCH, 78125, 10);  // 78.125kHz, 10Bit(1024 resolution)
  ledcAttachPin(PWMOUTPIN, PWMCH);
  ledcWrite(PWMCH, PWMMAX / 2);  //  50%(1.7V)
}