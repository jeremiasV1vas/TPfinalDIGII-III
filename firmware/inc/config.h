// Pines y constantes del proyecto.
// Si cambia algo del cableado, se cambia acá y listo.

#ifndef CONFIG_H_
#define CONFIG_H_

#include "lpc17xx_common.h"

// LED rojo de la placa. Se prende con un 0.
#define LED_PORT PORT_0
#define LED_PIN  PIN_22

// Los servos del esclavo van en P2.0 a P2.3 (PWM1.1 a PWM1.4).
// Un servo espera un pulso cada 20 ms.
#define SERVO_PERIODO_US 20000

// Entradas del ADC:
//   AD0.0 (P0.23) base del maestro
//   AD0.1 (P0.24) hombro del maestro
//   AD0.2 (P0.25) codo del maestro
//   AD0.4 (P1.30) garra del maestro
//   AD0.5 (P1.31) corriente de la garra del esclavo
// El AD0.3 no lo usamos porque comparte pin con la salida del DAC.

// Cada cuánto interrumpe el SysTick. Coincide con el período de los servos.
#define TICK_MS 20

#endif
