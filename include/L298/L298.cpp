/*
  L298.cpp - Library to control the L298 motor driver (c).
*/
#include "L298.h"

L298::L298(uint8_t IN1,uint8_t IN2,uint8_t EN1){
  _IN1 = IN1;
  _IN2 = IN2;
  _EN1 = EN1;
  pinMode(_IN1, OUTPUT);
  pinMode(_IN2, OUTPUT);
  pinMode(_EN1, OUTPUT);

}

void L298::Forward(uint8_t pwm){
  digitalWrite(_IN1,1);
  digitalWrite(_IN2,0);
  analogWrite(_EN1,pwm);
}

void L298::Reverse(uint8_t pwm){
  digitalWrite(_IN1,0);
  digitalWrite(_IN2,1);
  analogWrite(_EN1,pwm);

}

void L298::Disable(){
  digitalWrite(_IN1,1);
  digitalWrite(_IN2,1);
}

void L298::Stop(){
  analogWrite(_IN1,0);
  analogWrite(_IN2,0);
}
