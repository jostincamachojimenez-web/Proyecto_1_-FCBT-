#include <Arduino.h>
#include "hal_motor.h"
#include "button_isr.h"

// Prueba de la ISR: cada pulsacion valida imprime el contador. Un rebote no debe contar doble.
void setup() {
    Serial.begin(115200);
    motorInit();
    buttonInit();
}

void loop() {
    if (isButtonPressed()) {
        Serial.printf("Pulsacion valida. Total: %llu\n",
                      (unsigned long long)buttonPressCount());
    }
}