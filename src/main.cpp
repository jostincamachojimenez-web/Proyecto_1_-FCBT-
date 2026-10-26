#include <Arduino.h>
#include "hal_motor.h"

// Prueba del HAL: adelante 2 s -> freno 1 s -> coast 1 s, sin delay().
void setup() {
    Serial.begin(115200);
    motorInit();
    motorPWM(800);
}

void loop() {
    static uint32_t t0 = millis();
    static uint8_t paso = 0;
    uint32_t ahora = millis();
    const uint32_t duracion[3] = {2000, 1000, 1000};

    if ((uint32_t)(ahora - t0) >= duracion[paso]) {
        t0 = ahora;
        paso = (paso + 1) % 3;
        if (paso == 0)      { motorPWM(800); Serial.println("adelante"); }
        else if (paso == 1) { motorBrake();  Serial.println("freno"); }
        else                { motorStop();   Serial.println("coast"); }
    }
}