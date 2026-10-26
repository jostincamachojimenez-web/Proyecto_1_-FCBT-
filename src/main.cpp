#include <Arduino.h>
#include "hal_motor.h"
#include "button_isr.h"
#include "fsm_control.h"

void setup() {
    Serial.begin(115200);
    motorInit();
    buttonInit();
    fsmInit();
    Serial.println("Banda transportadora lista (IDLE). Presione el boton para iniciar.");
}

void loop() {
    Event ev = Event::NONE;

    if (isButtonPressed()) {                 // true una vez por pulsacion (leida bajo spinlock)
        ev = Event::BTN_PRESS;
        Serial.printf("[BTN] pulsaciones acumuladas: %llu\n",
                      (unsigned long long)buttonPressCount());
    } else if (fsmTimedOut(millis())) {      // sin delay(): solo compara tiempos
        ev = Event::TIMEOUT;
    }

    fsmDispatch(ev);                         // despacho O(1) por la matriz
}