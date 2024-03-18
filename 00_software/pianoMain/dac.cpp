#include "dac.h"

#include <driver/i2s.h>  //C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\sdk\esp32\include\driver\include\driver
#include <math.h>        //sin

#include "HardwareSerial.h"

// ピアノ鍵盤の最も低い周波数は27.5Hz(ラ)、最も高い周波数は4186Hz(ド)
// spsの上限は640000(lang-shipさんより)、また下限は20000である事を実験的に確認した
// DMA転送可能なバッファサイズは最大で1024である事が分かっている
// buf[i]:ダミー, buf[i+1]:25pin, buf[i+2]:ダミー, buf[i+3]:26pinの構成となる
// バッファとして1024byte確保した場合の1波形(buf[i+1])のサイズは256byteとなる

#define PI           (3.1415926535f)
#define MAXAMPLITUDE (127.0)
// サンプリングレート 10000:93Hz(下限値)
// サンプリングレート 20000:93Hz(下限値)
// サンプリングレート 30000:125Hz
// サンプリングレート 40000:165Hz
#define SAMPLING_RATE (20000)  // サンプリングレート
#define BUFFER_LEN    (1024)   // バッファサイズ
#define RESOLUTION    (BUFFER_LEN / 4)
#define NORMFREQ      (float)(SAMPLING_RATE / RESOLUTION)
uint8_t soundBuffer[BUFFER_LEN];  // DMA転送バッファ

#ifdef IDF_VER
#define I2SOFFSET          1
#define MY_I2S_COMM_FORMAT I2S_COMM_FORMAT_STAND_MSB
#else
#define I2SOFFSET          3
#define MY_I2S_COMM_FORMAT I2S_COMM_FORMAT_I2S_MSB
#endif

void dac::output(pianoKey *pkey) {
  size_t   transBytes;
  uint32_t waveNum;
  float    freqRatio, phase, sig, soundBufferCal[RESOLUTION];

  static uint32_t a;
  static uint32_t flag;

  // clear variables before calculation
  waveNum = 0;
  for (uint32_t m = 0; m < RESOLUTION; m++)
    soundBufferCal[m] = 0;

  // synthesize waves accroding to the pushed keyboard
  for (uint32_t oct = 0; oct < OCTAVENUM; oct++) {
    for (uint32_t tone = 0; tone < TONENUM; tone++) {
      if (pkey->key[oct][tone].volume > 0) {
        // To keep the same loudness, devide the synthesized wave by the number of synthesized wave
        waveNum++;
        for (uint32_t i = 0; i < RESOLUTION; i++) {
          // freqRatio become 1 when at 78.125Hz
          freqRatio = pkey->key[oct][tone].freq / NORMFREQ;
          // pahse from 0 to 2PI
          phase = 2.0 * PI * (float)i / (float)RESOLUTION;
          sig   = sin(freqRatio * phase);
          soundBufferCal[i] += sig;
        }
      }
    }
  }

  if (waveNum > 0) {
    // One or more keyboards were hit.
    for (uint32_t j = 0; j < BUFFER_LEN; j += 4) {
      soundBuffer[j] = 0;
      soundBuffer[j + 1] =
          (uint8_t)(127.0 +
                    MAXAMPLITUDE * soundBufferCal[j / 4] / (float)waveNum);
      soundBuffer[j + 2] = 0;
      soundBuffer[j + 3] = soundBuffer[j + 1];
    }
  } else {
    // None of the keyboards were hit, keep it silent by padding neutral(127)
    for (uint32_t k = 0; k < BUFFER_LEN; k += 4) {
      soundBuffer[k]     = 0;
      soundBuffer[k + 1] = 127;
      soundBuffer[k + 2] = 0;
      soundBuffer[k + 3] = 127;
    }
  }

  i2s_write(I2S_NUM_0, (char *)soundBuffer, BUFFER_LEN, &transBytes,
            portMAX_DELAY);
}

void dac::init(pianoKey *pkey) {
  i2s_config_t i2s_config = {
      .mode =
          (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
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

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL);
  i2s_zero_dma_buffer(I2S_NUM_0);
}