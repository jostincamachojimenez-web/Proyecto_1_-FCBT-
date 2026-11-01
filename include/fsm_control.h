#pragma once
#include <stdint.h>
#include <stddef.h>

enum class State : uint8_t { IDLE = 0, RUNNING, FAULT, COUNT };
enum class Event : uint8_t {
    NONE = 0,
    BTN_PRESS,      // paro de emergencia (IO33) -> FAULT
    CMD_START,      // arranque digital (boton IO32 o comando serial 's')
    CMD_STOP,       // paro digital (boton IO25 o comando serial 'p')
    TIMEOUT,        // paso de la rampa de arranque suave
    COUNT
};

struct Transition {
    State nextState;
    void (*action)();
};

// Matriz [Estado][Evento] -> acceso O(1)
extern const Transition TransitionFSM[static_cast<size_t>(State::COUNT)]
                                     [static_cast<size_t>(Event::COUNT)];

// Handlers
void onStart();     // arranca el motor con rampa suave
void onStop();      // frena el motor (sin FAULT)
void onFault();     // paro de emergencia
void onRamp();      // siguiente paso de la rampa
void onLocked();    // ordenes ignoradas mientras esta en FAULT
void onNoOp();

// API de la FSM
void  fsmInit();
void  fsmDispatch(Event e);
bool  fsmTimedOut(uint32_t nowMs);   // no bloqueante
State fsmState();