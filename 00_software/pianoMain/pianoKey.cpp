#include <stdio.h>
#include <esp32-hal.h> // C:\Users\nb1e4\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\cores\esp32
#include "pianoKey.h"

const uint32_t upperDIN[OCTAVENUM] = {36, 34, 32, 27, 12, 15, 16};
const uint32_t lowerDIN[OCTAVENUM] = {39, 35, 33, 14, 13,  4, 17};
const float freqList[OCTAVENUM * TONENUM] =
{36.708,38.891,41.203,43.654,46.249,48.999,51.913,55,58.27,61.735,65.406,69.296,
73.416,77.782,82.407,87.307,92.499,97.999,103.826,110,116.541,123.471,130.813,
138.591,146.832,155.563,164.814,174.614,184.997,195.998,207.652,220,233.082,
246.942,261.626,277.183,293.665,311.127,329.628,349.228,369.994,391.995,
415.305,440,466.164,493.883,523.251,554.365,587.33,622.254,659.255,698.456,
739.989,783.991,830.609,880,932.328,987.767,1046.502,1108.731,1174.659,1244.508,
1318.51,1396.913,1479.978,1567.982,1661.219,1760,1864.655,1975.533,2093.005,
2217.461,2349.318,2489.016,2637.02,2793.826,2959.955,3135.963,3322.438,3520,
3729.31,3951.066,4186.009,4434.922047};

void pianoKey::init(){
  for(uint32_t oct=0; oct<OCTAVENUM; oct++){
    pinMode(upperDIN[oct], INPUT);
    pinMode(lowerDIN[oct], INPUT);
    for(uint32_t tone=0; tone<TONENUM; tone++){
      key[oct][tone].freq = freqList[TONENUM * oct + tone];
    }
  }
}

void pianoKey::process(uint32_t octave, uint32_t addr) {
  key_st *pkey = &key[octave][addr];

  polling(octave, addr);
  stateLower(octave, addr);//Uppwer接点んが機能せずLower接点のみが機能
  // state(octave, addr);//Upper接点、Lower接点の両方ともに機能
}

void pianoKey::polling(uint32_t octave, uint32_t addr) {
  key_st *pkey = &key[octave][addr];
  /*
    例：第0オクターブ
            upper   lower   mux_addr  key
    RE      IO36    IO39    0         key[0][0]
    RE#     IO36    IO39    1         key[0][1]
    MI      IO36    IO39    2         key[0][2]
    FA      IO36    IO39    3         key[0][3]
    FA#     IO36    IO39    4         key[0][4]
    SO      IO36    IO39    5         key[0][5]
    SO#     IO36    IO39    6         key[0][6]
    RA      IO36    IO39    7         key[0][7]
    RA#     IO36    IO39    8         key[0][8]
    SHI     IO36    IO39    9         key[0][9]
    DO      IO36    IO39    10        key[0][10]
    DO#     IO36    IO39    11        key[0][11]
    Free0   IO36    IO39    12        key[0][12]
    Free1   IO36    IO39    13        key[0][13]
    Free2   IO36    IO39    14        key[0][14]
    Free3   IO36    IO39    15        key[0][15]
  */
  pkey->upper = digitalRead(upperDIN[octave]);
  pkey->lower = digitalRead(lowerDIN[octave]);
}


void pianoKey::stateLower(uint32_t octave, uint32_t addr) {
  key_st *pkey = &key[octave][addr];

  if(pkey->lower == true) pkey->volume = 1.0;
  else pkey->volume = 0.0;
}

void pianoKey::state(uint32_t octave, uint32_t addr) {
  key_st *pkey = &key[octave][addr];

  switch(pkey->seqID) {
    case STEP00: /* キーが押されていない状態 */
      if(pkey->upper == true) {/* キーが押された */
        pkey->start = millis();
        pkey->seqID = STEP01;
      }
      else if((pkey->upper == false) && (pkey->lower == false)) {/* 押してない状態を継続 */}
      else if((pkey->upper == false) && (pkey->lower == true) ) {/* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */}
      break;
    case STEP01:/* キーを途中まで押した */
      if     ((pkey->upper == false) && (pkey->lower == false)) {
        pkey->seqID = STEP00; /* 最後まで押されずに途中でリリース */
      }
      else if((pkey->upper == false) && (pkey->lower == true) ) {/* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */}
      else if((pkey->upper == true)  && (pkey->lower == false)) {/* 途中まで押した状態を継続 */}
      else if((pkey->upper == true)  && (pkey->lower == true) ) {/* 最後までキーを押した */
        pkey->end = millis();
        pkey->seqID = STEP02;
      }
      break;
    case STEP02:/* 最後までキーを押した */
      if     ((pkey->upper == false) && (pkey->lower == false)) {/* 最後まで押した後にリリース */
        pkey->volume = 0.0;
        pkey->seqID = STEP00;
      }
      else if((pkey->upper == false) && (pkey->lower == true) ) {/* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */}
      else if((pkey->upper == true)  && (pkey->lower == false)) {
        pkey->volume = 0.0;
        pkey->seqID = STEP01;/* 途中まで押した状態に戻る */
      }
      else if((pkey->upper == true)  && (pkey->lower == true) ) {/* 最後まで押した状態を維持*/
        pkey->volume = 1.0/(1.0 + (float)pkey->end - (float)pkey->start);
      }
      break;
    case STEP03:
      break;    
  }
}