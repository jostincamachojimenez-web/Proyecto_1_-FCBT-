#pragma once
#include <stdint.h>

// Paro de emergencia: microinterruptor entre IO33 y GND (INPUT_PULLUP, flanco FALLING).
void     buttonInit();
bool     isButtonPressed();    // true UNA vez por pulsacion valida (consume el evento)
uint64_t buttonPressCount();   // total acumulado; lectura atomica de 64 bits
