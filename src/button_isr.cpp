#include <Arduino.h>
#include <xtensa/core-macros.h>     // XTHAL_GET_CCOUNT()
#include "button_isr.h"

namespace {
constexpr uint8_t  PIN_EMERGENCIA = 33;            // microinterruptor de emergencia
constexpr uint8_t  PIN_ARRANQUE   = 32;            // boton digital de arranque
constexpr uint8_t  PIN_PARO       = 25;            // boton digital de paro
constexpr uint32_t DEBOUNCE_CICLOS = 4000000UL;    // 25 ms @ 160 MHz
constexpr uint32_t CONFIRMACION_CICLOS = 800000UL; // 5 ms @ 160 MHz: filtro de ruido en emergencia

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;   // spinlock SMP de hardware

// Estado de cada boton. Compartido entre ISR y loop(): volatile + spinlock.
struct Boton {
    uint8_t           pin;
    volatile uint32_t pendientes;    // eventos sin atender
    volatile uint64_t total;         // 64 bits => riesgo de torn read sin spinlock
    volatile uint32_t ultimoCiclo;   // solo la ISR la modifica
};

Boton emergencia = {PIN_EMERGENCIA, 0, 0, 0};
Boton arranque   = {PIN_ARRANQUE,   0, 0, 0};
Boton paro       = {PIN_PARO,       0, 0, 0};

// Una sola ISR para los tres botones; el argumento indica cual disparo.
void IRAM_ATTR botonISR(void* arg) {
    Boton* b = static_cast<Boton*>(arg);
    uint32_t ahora = XTHAL_GET_CCOUNT();
    // Resta modular sin signo: correcta aunque CCOUNT haga rollover.
    if ((uint32_t)(ahora - b->ultimoCiclo) >= DEBOUNCE_CICLOS) {
        b->ultimoCiclo = ahora;
        portENTER_CRITICAL_ISR(&mux);
        b->pendientes = b->pendientes + 1;
        b->total      = b->total + 1;
        portEXIT_CRITICAL_ISR(&mux);
    }
}

void configurar(Boton& b) {
    pinMode(b.pin, INPUT_PULLUP);
    // Deja el "ultimo disparo" 25 ms atras para aceptar la primera pulsacion.
    b.ultimoCiclo = XTHAL_GET_CCOUNT() - DEBOUNCE_CICLOS;
    attachInterruptArg(digitalPinToInterrupt(b.pin), botonISR, &b, FALLING);
}

bool consumir(Boton& b) {
    bool hubo = false;
    portENTER_CRITICAL(&mux);
    if (b.pendientes > 0) {
        b.pendientes = b.pendientes - 1;
        hubo = true;
    }
    portEXIT_CRITICAL(&mux);
    return hubo;
}
}  // namespace

void buttonInit() {
    configurar(emergencia);
    configurar(arranque);
    configurar(paro);
}

// Emergencia con filtro anti-ruido: el flanco solo cuenta si 5 ms despues
// el pin sigue en LOW. Un pico de ruido (microsegundos) se descarta.
bool isButtonPressed() {
    uint32_t pend, tFlanco;
    portENTER_CRITICAL(&mux);
    pend    = emergencia.pendientes;
    tFlanco = emergencia.ultimoCiclo;
    portEXIT_CRITICAL(&mux);

    if (pend == 0) return false;
    if ((uint32_t)(XTHAL_GET_CCOUNT() - tFlanco) < CONFIRMACION_CICLOS) {
        return false;                                  // aun no pasan los 5 ms: espera sin bloquear
    }
    bool confirmada = (digitalRead(PIN_EMERGENCIA) == LOW);
    consumir(emergencia);                              // atiende o descarta el flanco pendiente
    return confirmada;
}
bool isStartPressed()  { return consumir(arranque); }
bool isStopPressed()   { return consumir(paro); }

uint64_t buttonPressCount() {
    portENTER_CRITICAL(&mux);
    uint64_t total = emergencia.total;
    portEXIT_CRITICAL(&mux);
    return total;
}

bool emergencyActive() {
    return digitalRead(PIN_EMERGENCIA) == LOW;   // con INPUT_PULLUP, LOW = presionado
}