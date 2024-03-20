#ifndef PWM_H
#define PWM_H

#include <stdio.h>

#include "pianoKey.h"

class pwm {
 public:
  void init(void);
  void output(uint32_t time_count, pianoKey *pkey);

 private:
};

#endif /* PWM_H */