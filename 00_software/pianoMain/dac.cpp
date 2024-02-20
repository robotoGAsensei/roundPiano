#include <math.h> //sin
#include <driver/i2s.h> //C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\sdk\esp32\include\driver\include\driver

#include "dac.h"

// ピアノ鍵盤の最も低い周波数は27.5Hz(ラ)、最も高い周波数は4186Hz(ド)
// 再生周波数freq = サンプリングレートsps / バッファサイズbufSizeの関係となる
// lang-shipさんの実験結果からspsの上限は640,000である事が分かっている
// 例えば、最高音4186HzをbufSize 64で再生する場合のspsは267,904(=4186*64)となる
// 最低音27.5Hzを同一spsで再生するためのbufSizeは9742byte(=267904/27.5)となる
// I2Sに引き渡すフレームは4byteで構成され、そのうちの1byteが再生データに該当する
// このため実際の容量として約40KB(=9742*4)を確保する必要がある
// 必要量40KBに対してRAM容量327KBは十分だが、波形データをオンタイムで算出する必要がある

#define PI (3.1415)
#define SAMPLING_RATE (267904)       // サンプリングレート
#define BUFFER_LEN    (1024)           // バッファサイズ
uint8_t soundBuffer[BUFFER_LEN];     // DMA転送バッファ

void dac::output() {
  size_t transBytes;
  i2s_write(I2S_NUM_0, (char*)soundBuffer, BUFFER_LEN, &transBytes, portMAX_DELAY);
}

void dac::init() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate          = SAMPLING_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ALL_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 2,
    .dma_buf_len          = BUFFER_LEN,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0,
  };

  int step = 16;
  int val = 0;
  for (int i = 0; i < BUFFER_LEN; i += 4) {
    soundBuffer[i] = 0;
    soundBuffer[i + 1] = 127 + 127 * sin(2 * PI / 256 * val);
    soundBuffer[i + 2] = 0;
    soundBuffer[i + 2] = 0;
    val += (256 / step);
    if (256 <= val) {
      val = 0;
    }
  }

  // 再生設定実施
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL);
  i2s_zero_dma_buffer(I2S_NUM_0);

}