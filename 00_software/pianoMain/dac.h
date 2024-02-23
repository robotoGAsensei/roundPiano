#ifndef DAC_H
#define DAC_H

#include "pianoKey.h"

class dac {
public:
  void init();
  void output(pianoKey *pkey);

private:

};

#endif /* DAC_H */