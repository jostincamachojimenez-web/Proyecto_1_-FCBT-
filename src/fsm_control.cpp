#include <Arduino.h>
#include "fsm_control.h"
#include "hal_motor.h"

namespace {
State    estado        = State::IDLE;
uint16_t dutyActual    = 0;
uint32_t tUltimoPaso   = 0;

constexpr uint16_t DUTY_INICIAL      = 400;   // par minimo para vencer la inercia
constexpr uint16_t DUTY_PASO         = 40;
constexpr uint32_t PERIODO_RAMPA_MS  = 20;    // 20 ms por paso => ~300 ms hasta el maximo
}  // namespace

// ---------------- Handlers ----------------
void onStart() {
    dutyActual  = DUTY_INICIAL;
    motorForward(dutyActual);
    tUltimoPaso = millis();
    Serial.println("[FSM] Motor ARRANCADO (rampa suave) -> RUNNING");
}

void onStop() {
    motorBrake();
    Serial.println("[FSM] Motor DETENIDO (sin falla) -> IDLE");
}

void onFault() {
    motorBrake();
    Serial.println("[FSM] !!! PARO DE EMERGENCIA -> FAULT. Presione RESET de la tarjeta para rearmar.");
}

void onRamp() {
    uint16_t siguiente = dutyActual + DUTY_PASO;
    dutyActual = (siguiente > MOTOR_DUTY_MAX) ? MOTOR_DUTY_MAX : siguiente;
    motorForward(dutyActual);
}

void onLocked() {
    Serial.println("[FSM] Sistema en FAULT: orden ignorada. Solo el RESET de la tarjeta lo rearma.");
}

void onNoOp() {}

// ---------------- Matriz de transicion [Estado][Evento] ----------------
//                          NONE                      BTN_PRESS (emerg.)         CMD_START                   CMD_STOP                   TIMEOUT
const Transition TransitionFSM[static_cast<size_t>(State::COUNT)]
                              [static_cast<size_t>(Event::COUNT)] = {
    /* IDLE    */ { {State::IDLE,    onNoOp}, {State::FAULT, onFault}, {State::RUNNING, onStart},  {State::IDLE, onNoOp},   {State::IDLE,    onNoOp} },
    /* RUNNING */ { {State::RUNNING, onNoOp}, {State::FAULT, onFault}, {State::RUNNING, onNoOp},   {State::IDLE, onStop},   {State::RUNNING, onRamp} },
    /* FAULT   */ { {State::FAULT,   onNoOp}, {State::FAULT, onNoOp},  {State::FAULT,   onLocked}, {State::FAULT, onLocked}, {State::FAULT,   onNoOp} },
};

// ---------------- API ----------------
void fsmInit() {
    estado     = State::IDLE;
    dutyActual = 0;
    motorCoast();
}

void fsmDispatch(Event e) {
    const Transition& t = TransitionFSM[static_cast<size_t>(estado)]
                                       [static_cast<size_t>(e)];
    t.action();
    estado = t.nextState;
}

bool fsmTimedOut(uint32_t nowMs) {
    // Solo hay temporizacion mientras la rampa de arranque no termina
    if (estado != State::RUNNING || dutyActual >= MOTOR_DUTY_MAX) return false;
    if ((uint32_t)(nowMs - tUltimoPaso) >= PERIODO_RAMPA_MS) {
        tUltimoPaso = nowMs;
        return true;
    }
    return false;
}

State fsmState() { return estado; }