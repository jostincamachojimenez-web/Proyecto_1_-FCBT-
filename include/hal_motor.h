#pragma once
#include <stdint.h>

// PWM de 10 bits => duty de 0 a 1023
constexpr uint16_t MOTOR_DUTY_MAX = 1023;

void motorInit();                    // configura LEDC en IO12 e IO14 (10 bits, 1000 Hz)
void motorForward(uint16_t duty);    // adelante con PWM
void motorBrake();                   // frenado dinamico (ambas entradas del L9110S en alto)
void motorCoast();                   // giro por inercia (ambas entradas en bajo)