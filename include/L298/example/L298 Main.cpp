/*
    L298(uint8_t IN1,uint8_t IN2,uint8_t EN1);
    disable();
    Forward(uint8_t pwm);
    Reverse(uint8_t pwm);
    Stop();
*/
#include "L298.h"

const uint8_t IN1 = 8;
const uint8_t IN2 = 9;
const uint8_t EN1 = 10;

L298 motor(IN1, IN2, EN1);

void setup(){

}

void loop (){
    for(int speed = 0 ; speed < 255; speed+=10)
    {
      motor.Forward(speed);
    } 
    motor.Stop();
    for(int speed = 255 ; speed > 0; speed-=10)
    {
      motor.Reverse(speed);
    } 
    motor.Stop();
    motor.Disable();
    delay(5000);
}