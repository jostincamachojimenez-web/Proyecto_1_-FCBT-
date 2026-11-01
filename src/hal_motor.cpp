#include <Arduino.h>
#include "hal_motor.h"

namespace {
constexpr uint8_t  PIN_MOTOR_A = 12;     // L9110S entrada A (IO12)
constexpr uint8_t  PIN_MOTOR_B = 14;     // L9110S entrada B (IO14)
constexpr uint32_t PWM_FREQ_HZ = 1000;
constexpr uint8_t  PWM_RES_BITS = 10;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
// Core 3.x: el pin se asocia directamente al periferico LEDC
void pwmA(uint32_t duty) { ledcWrite(PIN_MOTOR_A, duty); }
void pwmB(uint32_t duty) { ledcWrite(PIN_MOTOR_B, duty); }
#else
// Core 2.x: se usan canales LEDC explicitos
constexpr uint8_t CH_A = 0;
constexpr uint8_t CH_B = 1;
void pwmA(uint32_t duty) { ledcWrite(CH_A, duty); }
void pwmB(uint32_t duty) { ledcWrite(CH_B, duty); }
#endif
}  // namespace

void motorInit() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_MOTOR_A, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_MOTOR_B, PWM_FREQ_HZ, PWM_RES_BITS);
#else
    ledcSetup(CH_A, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcSetup(CH_B, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(PIN_MOTOR_A, CH_A);
    ledcAttachPin(PIN_MOTOR_B, CH_B);
#endif
    motorCoast();
}

void motorForward(uint16_t duty) {
    if (duty > MOTOR_DUTY_MAX) duty = MOTOR_DUTY_MAX;
    pwmA(duty);          // si el motor gira al reves, intercambia A y B
    pwmB(0);
}

void motorBrake() {
    pwmA(MOTOR_DUTY_MAX);
    pwmB(MOTOR_DUTY_MAX);
}

void motorCoast() {
    pwmA(0);
    pwmB(0);
}