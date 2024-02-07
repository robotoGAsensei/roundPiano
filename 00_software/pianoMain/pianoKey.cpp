#include <stdio.h>
#include <esp32-hal.h>
#include "pianoKey.h"

const uint32_t upperDIN[OCTAVENUM] = {36, 34, 32, 27, 12, 15, 16,  5};
const uint32_t lowerDIN[OCTAVENUM] = {39, 35, 33, 14, 13,  4, 17, 18};

void pianoKey::init(){
  for(int i=0; i<OCTAVENUM; i++){
    pinMode(upperDIN[i], INPUT);
    pinMode(lowerDIN[i], INPUT);
  }
}

uint32_t pianoKey::process(uint32_t octave, uint32_t addr) {
  key_st *pkey = &key[octave][addr];

  polling(octave, addr);
  state(octave, addr);

  return(pkey->volume);
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
        pkey->volume = 0;
        pkey->seqID = STEP00;
      }
      else if((pkey->upper == false) && (pkey->lower == true) ) {/* 上側が反応せずに下側だけ反応する異常状態もしくはノイズ */}
      else if((pkey->upper == true)  && (pkey->lower == false)) {
        pkey->volume = 0;
        pkey->seqID = STEP01;/* 途中まで押した状態に戻る */
      }
      else if((pkey->upper == true)  && (pkey->lower == true) ) {/* 最後まで押した状態を維持*/
        pkey->volume = pkey->end - pkey->start;
      }
      break;
    case STEP03:
      break;    
  }
}