#include <Arduino.h>
#include "fsm_control.h"
#include "hal_motor.h"

namespace {
constexpr size_t N_STATES = static_cast<size_t>(State::COUNT);

State    g_state   = State::IDLE;
uint32_t g_enterMs = 0;

constexpr const char* STATE_NAME[N_STATES] = {"IDLE", "RUNNING", "FAULT"};

// Timeout por estado en ms (0 = sin timeout). RUNNING se detiene solo a los 15 s.
constexpr uint32_t TIMEOUT_MS[N_STATES] = {0, 15000, 0};

// Ciclo de trabajo de marcha. Ajustar a la resolucion del HAL (10 bits => 0..1023).
constexpr uint32_t RUN_DUTY = 800;
}  // namespace

// ---- Handlers: llaman al HAL del motor ----
void onStart()  { motorPWM(RUN_DUTY); }
void onFault()  { motorBrake(); }      // paro de emergencia: frenado dinamico
void onReset()  { motorStop(); }       // coast
void onNoOp()   {}

// ---- Matriz de transiciones ----
//                           NONE                      BTN_PRESS                      TIMEOUT
const Transition TransitionFSM[N_STATES][static_cast<size_t>(Event::COUNT)] = {
    /* IDLE    */ { {State::IDLE,    onNoOp},  {State::RUNNING, onStart}, {State::IDLE,    onNoOp} },
    /* RUNNING */ { {State::RUNNING, onNoOp},  {State::FAULT,   onFault}, {State::IDLE,    onReset} },
    /* FAULT   */ { {State::FAULT,   onNoOp},  {State::IDLE,    onReset}, {State::FAULT,   onNoOp} },
};

void fsmInit() {
    g_state   = State::IDLE;
    g_enterMs = millis();
    motorStop();
}

void fsmDispatch(Event e) {
    const Transition& t = TransitionFSM[static_cast<size_t>(g_state)]
                                       [static_cast<size_t>(e)];
    t.action();
    if (t.nextState != g_state) {
        Serial.printf("[FSM] %s -> %s\n",
                      STATE_NAME[static_cast<size_t>(g_state)],
                      STATE_NAME[static_cast<size_t>(t.nextState)]);
        g_state   = t.nextState;
        g_enterMs = millis();
    }
}

bool fsmTimedOut(uint32_t nowMs) {
    uint32_t limit = TIMEOUT_MS[static_cast<size_t>(g_state)];
    return limit != 0 && (uint32_t)(nowMs - g_enterMs) >= limit;
}

State fsmState() { return g_state; }
