#pragma once
#include <stdint.h>

void     buttonInit();          // configura los 3 botones con interrupcion

// Paro de emergencia (IO33): lleva la FSM a FAULT
bool     isButtonPressed();     // true una vez por pulsacion
uint64_t buttonPressCount();    // total acumulado (64 bits, leido bajo spinlock)
bool     emergencyActive();     // true si el boton de emergencia esta presionado ahora

// Botones digitales de operacion normal (NO provocan FAULT)
bool     isStartPressed();      // arranque (IO32)
bool     isStopPressed();       // paro (IO25)