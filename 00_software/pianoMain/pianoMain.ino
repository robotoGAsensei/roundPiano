/*
 Repeat timer example
 This example shows how to use hardware timer in ESP32. The timer calls onTimer
 function every second. The timer can be stopped with button attached to PIN 0
 (IO0).
 This example code is in the public domain.
 */
// Stop button is attached to PIN 0 (IO0)
#include "dac.h"
#include "pianoKey.h"

hw_timer_t                *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE               timerMux   = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t          isrCounter = 0;
volatile uint32_t          lastIsrAt  = 0;
static uint32_t            address;

pianoKey key;
dac      dac;

const uint32_t DOUT[4] = {19, 21, 22, 23};  // 13,12 を使うとシリアル通信ができなくなるので注意

void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

// 処理に時間のかかる関数を同一タスク内で実行すると実行周期が遅くなり波形が乱れる
// dac.output()は一度実行すると50ms程度は関数から抜ける事ができない
// このため50us周期で実行する必要のあるタスクを独立したコアに割り当てる
// delayの最小単位であるdelay(1)によりウォッチドッグによるリセットを回避できるが
// 実行周期として50usを達成するためにはdelay(1)による1msの空白期間が許容されない
// このため disableCore0WDT()によりウォッチドッグを無効化する
void task50us(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
      // キーボードの状態を更新しボリュームを取得する
      for (int i = 0; i < OCTAVENUM; i++) key.process(i, address);

      // マルチプレクサに出力するセレクト信号
      if (++address > MULTIPLEXNUM - 1) address = 0;
      digitalWrite(DOUT[0], 0x1 & address);
      digitalWrite(DOUT[1], 0x1 & (address >> 1));
      digitalWrite(DOUT[2], 0x1 & (address >> 2));
      digitalWrite(DOUT[3], 0x1 & (address >> 3));
    }
  }
}

// dac.output()をコールしたタイミングでバッファ受け取り可能であれば受け取り処理が走る
// 受け取り処理として例えば50msを要するが、時間はバッファサイズやサンプリングレートに依存する
// 受け取り可能でない場合にコールすると、1ms以内に関数から抜ける
// dac.output()のコール周期が一定以上に長くなると波形が途絶えて断続的な再生になる
// 連続波形を再生するためには連続的にコールし続ける必要がある
void taskDac(void *pvParameters) {
  uint32_t isrCount, isrTime;

  while (1) {
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime  = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);

    dac.output(&key);
  }
}

void setup() {
  Serial.begin(115200);

  // タスクを作る前にpinModeの設定をする必要がある
  key.init();
  for (int j = 0; j < 4; j++) pinMode(DOUT[j], OUTPUT);

  dac.init(&key);

  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Clock count at 40MHz(=80MHz/2). By the prescaler 2 which must be in the range from 2 to 655535
  timer = timerBegin(0, 2, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every 50us(2000 = 40,000,000Hz / 20,000Hz)
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 2000, true);
  // Start an alarm
  timerAlarmEnable(timer);

  xTaskCreateUniversal(
      taskDac,
      "taskDac",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      APP_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  xTaskCreateUniversal(
      task50us,
      "task50us",
      8192,
      NULL,
      configMAX_PRIORITIES - 1,  // 最高優先度
      NULL,
      PRO_CPU_NUM  // PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );

  disableCore0WDT();  // PRO_CPUのウォッチドッグ停止
}

void loop() {
  delay(1);
}