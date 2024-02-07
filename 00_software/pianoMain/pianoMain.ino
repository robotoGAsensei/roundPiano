/*
 Repeat timer example
 This example shows how to use hardware timer in ESP32. The timer calls onTimer
 function every second. The timer can be stopped with button attached to PIN 0
 (IO0).
 This example code is in the public domain.
 */
// Stop button is attached to PIN 0 (IO0)
#include "pianoKey.h"

#define BTN_STOP_ALARM    0
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

pianoKey key;

const uint32_t DOUT[4] = {23,22,21,19}; //13,12 を使うとシリアル通信ができなくなるので注意

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

void task50us(void *pvParameters){
  static uint32_t address;
  static uint32_t volume[OCTAVENUM][MULTIPLEXNUM];

  while(1){
    // If Timer has fired
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
      uint32_t isrCount = 0, isrTime = 0;
      // Read the interrupt count and time
      portENTER_CRITICAL(&timerMux);
      isrCount = isrCounter;
      isrTime = lastIsrAt;
      portEXIT_CRITICAL(&timerMux);

      // キーボードの状態を更新しボリュームを取得する
      for(int i=0; i<OCTAVENUM; i++) volume[i][address] = key.process(i,address);

      // マルチプレクサに出力するセレクト信号
      if(++address > MULTIPLEXNUM - 1) address = 0;
      digitalWrite(DOUT[3], 0x1 & (address >> 3));
      digitalWrite(DOUT[2], 0x1 & (address >> 2));
      digitalWrite(DOUT[1], 0x1 & (address >> 1));
      digitalWrite(DOUT[0], 0x1 & address);

      if(isrCount % 20000 == 0) {
        Serial.print(" onTimer no. ");
        Serial.print(isrCount);
        Serial.print(" at ");
        Serial.print(isrTime);
        Serial.println(" ms");
      }
    }

    // If button is pressed
    if (digitalRead(BTN_STOP_ALARM) == LOW) {
      // If timer is still running
      if (timer) {
        // Stop and free timer
        timerEnd(timer);
        timer = NULL;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Set BTN_STOP_ALARM to input mode
  pinMode(BTN_STOP_ALARM, INPUT);
  
  // タスクを作る前にpinModeの設定をする必要がある
  key.init();
  for(int j=0;j<4;j++) pinMode(DOUT[j], OUTPUT);

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

  //タスクを作った瞬間に最高優先度の無限ループに入るので、これ以降の処理は実行されない点に注意
  xTaskCreateUniversal(
    task50us,
    "task50us",
    8192,
    NULL,
    configMAX_PRIORITIES-1, // 最高優先度
    NULL,
    APP_CPU_NUM //PRO_CPU:Core0,WDT有効 / APP_CPU:Core1,WDT無効
  );
}

void loop() {
  Serial.print("You will never see this message as task50us occupy the Core1");
}