#ifndef DAC_H
#define DAC_H

#include "pianoKey.h"

class dac {
public:
  void init(pianoKey *pkey);
  void output(pianoKey *pkey);

private:

};

#endif /* DAC_H */