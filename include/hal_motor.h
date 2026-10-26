#pragma once
#include <stdint.h>

// HAL del motor DC (L9110S de la IdeaBoard: IO12 = A, IO14 = B) sobre el periferico LEDC.
void motorInit();                       // timer 10 bits @ 1000 Hz + canales 0 y 1
void motorPWM(uint32_t cicloTrabajo);   // adelante, 0..1023 (se satura en 1023)
void motorBrake();                      // frenado dinamico (ambos canales al 100 %)
void motorStop();                       // coast: giro por inercia (ambos canales a 0)
