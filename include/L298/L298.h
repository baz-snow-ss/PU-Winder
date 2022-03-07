/*
  L298.h - Library to control the L298 motor driver
*/
#ifndef L298_h
#define L298_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class L298
{
  public:
    L298(uint8_t IN1,uint8_t IN2,uint8_t EN1);
    void Disable();
    void Forward(uint8_t pwm);
    void Reverse(uint8_t pwm);
    void Stop();

  private:
    uint8_t _IN1;
    uint8_t _IN2;
    uint8_t _EN1;
};
#endif