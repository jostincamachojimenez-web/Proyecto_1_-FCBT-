# Proyecto 1: Firmware Controlador de Banda Transportadora
Ingeniería Electrónica – Programación II · IdeaBoard (ESP32) · PlatformIO

Firmware con FSM orientada a eventos, PWM por hardware (LEDC), anti-rebote por ciclos (CCOUNT) y ISR segura.

## Estructura
```
platformio.ini            configuracion (esp32dev, arduino, 160 MHz)
include/ + src/
  hal_motor.*             Fase 2: HAL del motor con LEDC (10 bits, 1000 Hz)
  button_isr.*            Fase 3: ISR IRAM_ATTR + debounce CCOUNT + spinlock
  fsm_control.*           Fase 4: FSM O(1) con matriz de transiciones
  main.cpp                loop() sin delay(): eventos -> FSM
documentacion/
  arquitectura.md         diagrama y explicacion por fase
  hardware_pines.json     asignacion de pines
  marco_teorico.md        respuestas del cuestionario
```

## Hardware
Motor DG01D por L9110S (IO12 / IO14), jumper en SELECT-Vin con fuente 5 V / 3 A. Microinterruptor entre IO33 y GND.

## Compilar y cargar
`pio run` · `pio run -t upload` · `pio device monitor`
