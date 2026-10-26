#include <Arduino.h>
#include <xtensa/core-macros.h>     // XTHAL_GET_CCOUNT()
#include "button_isr.h"

namespace {
constexpr uint8_t  BOTON_PIN       = 33;
constexpr uint32_t DEBOUNCE_CICLOS = 4000000UL;   // 25 ms @ 160 MHz

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // spinlock SMP de hardware

// Compartidas entre la ISR y loop(): volatile + acceso bajo spinlock.
volatile uint32_t pulsacionesPendientes = 0;   // eventos sin atender (no se pierden rafagas)
volatile uint64_t pulsacionesTotal      = 0;   // 64 bits => riesgo de torn read sin spinlock
volatile uint32_t isrUltimoCiclo        = 0;   // solo la ISR la modifica

// IRAM_ATTR: la ISR vive en IRAM; segura aunque la cache de Flash este deshabilitada.
void IRAM_ATTR botonISR() {
    uint32_t ahora = XTHAL_GET_CCOUNT();
    // Resta modular sin signo: correcta aunque CCOUNT haga rollover (cada 26,84 s).
    if ((uint32_t)(ahora - isrUltimoCiclo) >= DEBOUNCE_CICLOS) {
        isrUltimoCiclo = ahora;
        portENTER_CRITICAL_ISR(&mux);
        pulsacionesPendientes = pulsacionesPendientes + 1;
        pulsacionesTotal      = pulsacionesTotal + 1;
        portEXIT_CRITICAL_ISR(&mux);
    }
}
}  // namespace

void buttonInit() {
    pinMode(BOTON_PIN, INPUT_PULLUP);
    // Deja el "ultimo disparo" 25 ms atras para que la primera pulsacion sea aceptada.
    isrUltimoCiclo = XTHAL_GET_CCOUNT() - DEBOUNCE_CICLOS;
    attachInterrupt(digitalPinToInterrupt(BOTON_PIN), botonISR, FALLING);
}

bool isButtonPressed() {
    bool hubo = false;
    portENTER_CRITICAL(&mux);
    if (pulsacionesPendientes > 0) {
        pulsacionesPendientes = pulsacionesPendientes - 1;
        hubo = true;
    }
    portEXIT_CRITICAL(&mux);
    return hubo;
}

uint64_t buttonPressCount() {
    portENTER_CRITICAL(&mux);
    uint64_t total = pulsacionesTotal;
    portEXIT_CRITICAL(&mux);
    return total;
}
