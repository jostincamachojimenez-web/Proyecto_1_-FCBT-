#include <Arduino.h>
#include "driver/ledc.h"
#include "hal_motor.h"

namespace {
constexpr gpio_num_t     MOTOR_A_PIN = GPIO_NUM_12;
constexpr gpio_num_t     MOTOR_B_PIN = GPIO_NUM_14;
constexpr ledc_mode_t    LEDC_MODO   = LEDC_HIGH_SPEED_MODE;
constexpr ledc_timer_t   LEDC_TIMER  = LEDC_TIMER_0;
constexpr ledc_channel_t CANAL_A     = LEDC_CHANNEL_0;
constexpr ledc_channel_t CANAL_B     = LEDC_CHANNEL_1;
constexpr uint32_t       PWM_FREQ_HZ = 1000;
constexpr uint32_t       DUTY_MAX    = (1u << 10) - 1;   // 1023 (10 bits)

void setDuty(ledc_channel_t canal, uint32_t duty) {
    ledc_set_duty(LEDC_MODO, canal, duty);
    ledc_update_duty(LEDC_MODO, canal);
}

void configCanal(ledc_channel_t canal, gpio_num_t pin) {
    ledc_channel_config_t c = {};
    c.gpio_num   = pin;
    c.speed_mode = LEDC_MODO;
    c.channel    = canal;
    c.intr_type  = LEDC_INTR_DISABLE;
    c.timer_sel  = LEDC_TIMER;
    c.duty       = 0;
    c.hpoint     = 0;
    ledc_channel_config(&c);
}
}  // namespace

void motorInit() {
    ledc_timer_config_t t = {};
    t.speed_mode      = LEDC_MODO;
    t.duty_resolution = LEDC_TIMER_10_BIT;
    t.timer_num       = LEDC_TIMER;
    t.freq_hz         = PWM_FREQ_HZ;
    t.clk_cfg         = LEDC_AUTO_CLK;
    ledc_timer_config(&t);              // 1) primero el timer
    configCanal(CANAL_A, MOTOR_A_PIN);  // 2) despues los canales
    configCanal(CANAL_B, MOTOR_B_PIN);
    motorStop();
}

void motorPWM(uint32_t cicloTrabajo) {
    if (cicloTrabajo > DUTY_MAX) cicloTrabajo = DUTY_MAX;   // proteccion de desborde
    setDuty(CANAL_A, cicloTrabajo);
    setDuty(CANAL_B, 0);
}

void motorBrake() {
    setDuty(CANAL_A, DUTY_MAX);
    setDuty(CANAL_B, DUTY_MAX);
}

void motorStop() {
    setDuty(CANAL_A, 0);
    setDuty(CANAL_B, 0);
}
