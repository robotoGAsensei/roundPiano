#ifndef PIANOKEY_H
#define PIANOKEY_H

#define OCTAVENUM (8)
#define MULTIPLEXNUM (16)

typedef enum
{
    STEP00 = 0U,
    STEP01,
    STEP02,
    STEP03,
    STEP04,
    STEP05,
    STEP06,
    STEP07,
    STEP08,
    STEP09,
    STEP10,
    STEP11,
    STEP12,
    STEP13,
    STEP14,
    STEP15,
    STEP16,
    STEP17,
    STEP18,
    STEP19,
    STEP20,
    STEP21,
    STEP22,
    STEP23,
    STEP24,
    STEP25,
    STEP26,
    STEP27,
    STEP28,
    STEP29,
    STEP30,
    LOOP,
    RUN,
    INIT,
    IDLE,
    END
} seqID_t;

//クラスの定義
class pianoKey {
public:
  void init();
  uint32_t process(uint32_t octave, uint32_t addr);
 
private:
  void polling(uint32_t octave, uint32_t addr);
  void state(uint32_t octave, uint32_t addr);

  typedef struct
  {
    seqID_t seqID;
    uint32_t start;
    uint32_t end;
    uint32_t volume;
    uint32_t upper;
    uint32_t lower;
  } key_st;

  key_st key[OCTAVENUM][MULTIPLEXNUM];
};


#endif /* PIANOKEY_H */