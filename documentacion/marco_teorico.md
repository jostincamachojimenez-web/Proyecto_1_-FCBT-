# Marco teórico (respuestas)

## 1. Concurrencia en FreeRTOS

**1.1 Contexto de tarea vs. contexto de excepción (ISR).**
Una tarea tiene su propio stack, su TCB (Task Control Block) y estado (Running/Ready/Blocked/Suspended); el scheduler puede suspenderla y reanudarla. Una ISR corre en contexto de excepción: usa el stack de interrupción, no tiene TCB y no pertenece a ningún estado del scheduler. `vTaskDelay()` coloca la *tarea actual* en la lista de tareas bloqueadas y pide un cambio de contexto; en una ISR no existe una tarea que bloquear, y además el scheduler asume que se invoca desde contexto de tarea. El resultado es corrupción de las listas del kernel o un `configASSERT` fallido, que termina en pánico (Guru Meditation). Dentro de una ISR solo se pueden usar las variantes `...FromISR()`.

**1.2 Spinlock vs. Mutex.**
El Mutex (`SemaphoreHandle_t`) es un objeto del kernel basado en colas: si está ocupado, la tarea pasa a *Blocked* y el scheduler cede la CPU a otra; soporta herencia de prioridad. El Spinlock (`portMUX_TYPE`) es una primitiva de hardware (operación atómica *compare-and-set*, `S32C1I` en Xtensa) que hace espera activa (*busy-wait*) y, con `portENTER_CRITICAL`, además enmascara interrupciones en el núcleo local, protegiendo también entre núcleos (SMP). Entre `loop()` y una ISR se debe usar spinlock porque la ISR **no puede bloquearse** (no hay tarea que dormir) y no puede tomar un mutex con `xSemaphoreTake`. Además, al enmascarar interrupciones durante la sección crítica, la ISR no puede interrumpir al propietario del lock en el mismo núcleo.

**1.3 Inversión de prioridad.**
Ocurre cuando una tarea de baja prioridad (L) retiene un recurso que necesita una de alta prioridad (H); H se bloquea, y una tarea de prioridad media (M) desaloja a L. Así H queda esperando indirectamente a M, por un tiempo no acotado (caso Mars Pathfinder, 1997). Con **herencia de prioridad**, cuando H se bloquea en el mutex, el kernel eleva temporalmente la prioridad de L a la de H; M ya no puede desalojar a L, que termina su sección crítica, libera el mutex y recupera su prioridad original. Esto lo implementan los mutex de FreeRTOS (no los semáforos binarios).

**1.4 Deadlock: condiciones de Coffman y escenario con ISR.**
Las cuatro condiciones: (1) exclusión mutua, (2) retención y espera, (3) no apropiación (no expropiación del recurso) y (4) espera circular.
Secuencia: (a) `loop()` toma el Mutex M; (b) llega la interrupción, que tiene prioridad sobre cualquier tarea, y desaloja a `loop()` con M aún retenido; (c) la ISR intenta tomar M, que está ocupado; (d) la ISR no puede bloquearse ni ceder la CPU, así que queda esperando a que `loop()` libere M; (e) `loop()` nunca vuelve a ejecutarse porque la ISR no termina. Cada uno espera al otro: deadlock. El *Interrupt Watchdog* termina reiniciando el chip.

## 2. Arquitectura de memoria y silicio

**2.1 Caché de Flash e `IRAM_ATTR`.**
El código se ejecuta desde Flash SPI externa a través de la caché. Al escribir/borrar Flash (NVS, OTA), el controlador SPI queda ocupado y el sistema **deshabilita la caché** (y detiene la otra CPU) durante la operación; mientras tanto no se puede leer código de Flash. Si una interrupción ocurre en ese lapso y su ISR está en Flash, la CPU intenta buscar instrucciones sin caché → *Cache disabled but cached memory region accessed* → Guru Meditation Error. `IRAM_ATTR` ubica la función en IRAM (SRAM interna), siempre accesible. Los datos que toque la ISR deben estar en DRAM (las variables globales `volatile` lo están por defecto).

**2.2 Torn reads.**
El ESP32 (Xtensa LX6) tiene registros y bus de datos de 32 bits; un `uint64_t` se lee con dos instrucciones `l32i` (palabra baja y alta). Ejemplo: el contador vale `0x00000000_FFFFFFFF`. `loop()` lee la parte baja (`0xFFFFFFFF`); justo entonces la ISR lo incrementa a `0x00000001_00000000`; `loop()` lee la parte alta (`0x00000001`) y compone `0x00000001_FFFFFFFF`, un valor que **nunca existió** (error de ~4.3×10⁹). La solución es leer dentro de una sección crítica (spinlock), como hace `Button::consumePress`.

**2.3 `volatile` y asignación de registros.**
Con `-O2`, el compilador mantiene las variables en registros para evitar accesos a memoria. En `while(!flag){}`, si `flag` no es `volatile`, el compilador asume que nadie más la modifica (el flujo secuencial no la cambia), así que hace *hoisting*: carga `flag` una sola vez en un registro (p. ej. `a2`) y evalúa siempre ese valor obsoleto, ignorando que la ISR modificó la RAM. `volatile` obliga a hacer una lectura/escritura real en memoria en cada acceso. Ojo: `volatile` **no** da atomicidad ni exclusión mutua; por eso se combina con el spinlock.

## 3. Rebote mecánico y depuración

**3.1 Física del contacto.**
Las superficies metálicas son microscópicamente rugosas y elásticas. Al cerrarse, el contacto móvil impacta, rebota y oscila de forma amortiguada durante milisegundos; cada vez se forman y rompen micro-puntos de contacto (con posible microarco y óxidos). La tensión cruza el umbral lógico múltiples veces. Como un ESP32 a 160 MHz (6,25 ns por ciclo, latencia de interrupción de ~1 µs) es mucho más rápido que esas oscilaciones, detecta cada transición: ráfagas de 20 a 50 cambios por una sola pulsación.

**3.2 Por qué no `millis()`/`delay()` en la ISR y por qué CCOUNT.**
`delay()` bloquea la CPU (y termina en `vTaskDelay`, prohibido en ISR), aumenta la latencia de otras interrupciones y dispara watchdogs. `millis()` es una llamada a función (potencialmente en Flash, no segura con caché deshabilitada), con resolución de 1 ms, que depende de un temporizador del sistema. `XTHAL_GET_CCOUNT()` lee el registro especial `CCOUNT` del núcleo con una sola instrucción (`rsr.ccount`), sin llamadas, sin bus de memoria y sin bloqueo; se incrementa cada ciclo de reloj. (Es por núcleo; la ISR y la medición ocurren en el mismo núcleo.)

**3.3 Aritmética modular.**
Sea T = 2³². Si el tiempo real (en ciclos) en dos instantes es t₀ y t₁ = t₀ + Δ con 0 ≤ Δ < T, los registros contienen c₀ = t₀ mod T y c₁ = t₁ mod T. La resta de `uint32_t` calcula (c₁ − c₀) mod T = (t₁ − t₀) mod T = Δ mod T = Δ. Se cumple aunque haya rollover. Ejemplo numérico: c₀ = `0xFFFFFF00`, c₁ = `0x00000100`. c₁ − c₀ = 256 − 4294967040 = −4294966784 ≡ 2³² − 4294966784 = **512** ciclos, que es el tiempo real. El rollover ocurre a 2³²/160×10⁶ = 26,84 s, y el método es válido siempre que Δ < 26,84 s.

## 4. Periféricos y determinismo

**4.1 Hardware PWM (LEDC).**
Los canales LEDC no generan la señal por sí mismos: cada canal compara su duty con el contador de un *timer*, que define la frecuencia y la resolución (ancho del contador), compartidas por los canales asociados. Sin configurar el timer primero no hay contador de referencia. Con 10 bits se tienen 1024 niveles de duty (0–1023), y la resolución máxima está limitada por f_clk/f_pwm: con APB de 80 MHz y 1 kHz → log₂(80000) ≈ 16,3 bits, así que 10 bits es viable. Usar el mismo timer para IO12 e IO14 mantiene ambas entradas del L9110S sincronizadas.

**4.2 Pull-up en IO34.**
Los pines GPIO34–39 del ESP32 son solo entrada y **no tienen resistencias pull-up/pull-down internas**. `INPUT_PULLUP` no tiene efecto allí; con el interruptor abierto la entrada flota y capta ruido, generando lecturas aleatorias. Se requiere una resistencia externa (≈10 kΩ a 3,3 V).

**4.3 Matriz de transición O(1).**
Acceder a `FSM[estado][evento]` es calcular `base + (s·N_EVENTOS + e)·4` y hacer una carga y una llamada indirecta: el costo no depende del número de estados ni eventos, es O(1) con tiempo fijo y determinista. Una cadena de `if-else`/`switch` anidados hace un número variable de comparaciones y saltos condicionales según el caso (peor caso lineal), y cada salto es una oportunidad de *misprediction*; la matriz usa un único salto indirecto, con un patrón mucho más predecible. Nota honesta: el LX6 es un procesador en orden de pipeline corto, con penalización pequeña por saltos; en este chip la ventaja principal es el determinismo temporal, la mantenibilidad y la ausencia de lógica espagueti. En CPUs superescalares el beneficio de predicción de bifurcaciones es mayor.
