#include <Arduino.h>
#include "hal_motor.h"
#include "button_isr.h"
#include "fsm_control.h"

// Comandos digitales por el monitor serie: 's' = arranque, 'p' = paro.
// No bloquea: solo lee si ya hay un byte disponible.
static Event leerComandoSerial() {
    if (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == 's' || c == 'S') return Event::CMD_START;
        if (c == 'p' || c == 'P') return Event::CMD_STOP;
    }
    return Event::NONE;
}

void setup() {
    Serial.begin(115200);
    motorInit();
    buttonInit();
    fsmInit();

    Serial.println("Banda transportadora: arranque automatico.");
    Serial.println("Arranque: boton IO32 o 's' | Paro: boton IO25 o 'p' | Emergencia: IO33 (FAULT, solo RESET)");

    // Arranque automatico al iniciar. Si el boton de emergencia ya esta
    // presionado durante el boot, el sistema entra directo a FAULT.
    fsmDispatch(emergencyActive() ? Event::BTN_PRESS : Event::CMD_START);
}

void loop() {
    Event ev = Event::NONE;

    if (isButtonPressed()) {                     // emergencia: maxima prioridad
        ev = Event::BTN_PRESS;
        Serial.printf("[BTN] emergencias acumuladas: %llu\n",
                      (unsigned long long)buttonPressCount());
    } else if (isStopPressed()) {                // paro digital (no es falla)
        ev = Event::CMD_STOP;
    } else if (isStartPressed()) {               // arranque digital
        ev = Event::CMD_START;
    } else {
        ev = leerComandoSerial();
        if (ev == Event::NONE && fsmTimedOut(millis())) {   // sin delay()
            ev = Event::TIMEOUT;
        }
    }

    fsmDispatch(ev);                             // despacho O(1) por la matriz
}