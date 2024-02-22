#include <math.h> //sin
#include <driver/i2s.h> //C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\sdk\esp32\include\driver\include\driver

#include "dac.h"

// ピアノ鍵盤の最も低い周波数は27.5Hz(ラ)、最も高い周波数は4186Hz(ド)
// lang-shipさんの実験結果からspsの上限は640000である事が分かっている
// またDMA転送可能なバッファサイズは最大で1024である事が分かっている
// buf[i]:ダミー, buf[i+1]:25pin, buf[i+2]:ダミー, buf[i+3]:26pinの構成となる
// バッファとして1024byte確保した場合のbuf[i+1]のサイズは256byteとなる
// この256byteに1波形を収録すると最低周波数となり、これを27.5Hzにするためには、
// サンプリングレートを7040(=27.5*256)にする必要がある。 

#define PI (3.1415)
#define SAMPLING_RATE (20000)       // サンプリングレート
#define BUFFER_LEN    (1024)           // バッファサイズ
uint8_t soundBuffer[BUFFER_LEN];     // DMA転送バッファ

#ifdef IDF_VER
#define I2SOFFSET 1
#define MY_I2S_COMM_FORMAT I2S_COMM_FORMAT_STAND_MSB
#else
#define I2SOFFSET 3
#define MY_I2S_COMM_FORMAT I2S_COMM_FORMAT_I2S_MSB
#endif

void dac::output() {
  size_t transBytes;
  i2s_write(I2S_NUM_0, (char*)soundBuffer, BUFFER_LEN, &transBytes, portMAX_DELAY);
}

void dac::init() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate          = SAMPLING_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = MY_I2S_COMM_FORMAT,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 2,
    .dma_buf_len          = BUFFER_LEN,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0,
  };

  int step = 16;
  int val = 0;
  int val2 = 0;
  for (int i = 0; i < BUFFER_LEN; i += 4) {
    soundBuffer[i] = 0;
    soundBuffer[i + 1] = 127 + 127 * cos(2 * PI / 256 * val);
    soundBuffer[i + 2] = 0;
    soundBuffer[i + 3] = 127 + 127 * sin(2 * PI / 256 * val2);
    val += (256 / step);
    val2 += 1;
    if (256 <= val) {
      val = 0;
    }
  }

  // 再生設定実施
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL);
  i2s_zero_dma_buffer(I2S_NUM_0);

}