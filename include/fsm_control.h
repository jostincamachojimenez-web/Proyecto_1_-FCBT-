#pragma once
#include <stdint.h>
#include <stddef.h>

enum class State : uint8_t { IDLE = 0, RUNNING, FAULT, COUNT };
enum class Event : uint8_t { NONE = 0, BTN_PRESS, TIMEOUT, COUNT };

struct Transition {
    State nextState;
    void (*action)();
};

// Matriz [Estado][Evento] -> acceso O(1)
extern const Transition TransitionFSM[static_cast<size_t>(State::COUNT)]
                                     [static_cast<size_t>(Event::COUNT)];

// Handlers
void onStart();
void onFault();
void onReset();
void onNoOp();

// API de la FSM
void  fsmInit();
void  fsmDispatch(Event e);
bool  fsmTimedOut(uint32_t nowMs);   // no bloqueante
State fsmState();
