/**
 * @file config.h
 * @brief Pines, tiempos y valores de calibración de todo el proyecto.
 *
 * Si cambia algo del cableado, o hay que ajustar algún número después de
 * probar con los brazos armados, se cambia acá y listo.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "lpc17xx_common.h"

/**
 * @name LED de la placa
 * LED RGB de la LPCXpresso. Usamos el rojo y el verde, y los dos se prenden con un 0.
 * @{
 */
#define LED_ROJO_PORT  PORT_0
#define LED_ROJO_PIN   PIN_22
#define LED_VERDE_PORT PORT_3
#define LED_VERDE_PIN  PIN_25
/** @} */

/**
 * @name Tiempos
 * @{
 */
#define TICK_MS          20    ///< Cada cuánto interrumpe el SysTick. Es un período de los servos.
#define SERVO_PERIODO_US 20000 ///< Un servo espera un pulso cada 20 ms.
/** @} */

/**
 * @name Canales del ADC
 * Los potes del maestro. El AD0.3 no lo usamos porque comparte pin con la
 * salida del DAC. La corriente de la garra entra por el AD0.5 (P1.31) y ese
 * canal lo lee el DMA, así que está fijo en sensores.c.
 * @{
 */
#define POTE_BASE   0 ///< AD0.0 (P0.23)
#define POTE_HOMBRO 1 ///< AD0.1 (P0.24)
#define POTE_CODO   2 ///< AD0.2 (P0.25)
#define POTE_GARRA  4 ///< AD0.4 (P1.30)
/** @} */

/**
 * @name Calibración del maestro
 * Cada articulación se calibra con dos posiciones, A y B (por ejemplo, la base
 * mirando a la izquierda y a la derecha). Para cada una se anota la lectura del
 * pote del maestro y el pulso que lleva al esclavo a esa misma posición.
 * Entre A y B se hace una regla de tres, así el maestro no tiene que medir
 * exactamente lo mismo que el esclavo.
 *
 * Mientras no estén calibrados, todo el recorrido del pote va de 1000 a 2000 µs.
 * @{
 */
#define CAL_BASE_POTE_A    0
#define CAL_BASE_POTE_B    4095
#define CAL_BASE_PULSO_A   1000
#define CAL_BASE_PULSO_B   2000

#define CAL_HOMBRO_POTE_A  0
#define CAL_HOMBRO_POTE_B  4095
#define CAL_HOMBRO_PULSO_A 1000
#define CAL_HOMBRO_PULSO_B 2000

#define CAL_CODO_POTE_A    0
#define CAL_CODO_POTE_B    4095
#define CAL_CODO_PULSO_A   1000
#define CAL_CODO_PULSO_B   2000

#define CAL_GARRA_POTE_SUELTA   0    ///< Pote de la palanca del maestro sin apretar.
#define CAL_GARRA_POTE_APRETADA 4095 ///< Pote de la palanca apretada a fondo contra el tope.
/** @} */

/**
 * Si el pulso nuevo difiere del anterior en menos que esto, el servo no se
 * mueve. Así no tiembla por el ruido que queda en los potes.
 */
#define ZONA_MUERTA_US 4

/**
 * @name Garra del esclavo
 * Pulso con la garra abierta del todo y cerrada del todo (mandíbulas juntas).
 * Se ajustan con el brazo armado.
 * @{
 */
#define GARRA_ABIERTA_US 1000
#define GARRA_CERRADA_US 2000
/** @} */

/**
 * @name Pines de los botones
 * Pulsadores a masa con el pull-up interno. No usamos el EINT0 (P2.10) porque
 * si ese pin está en 0 cuando arranca la placa, entra al modo de grabación
 * (ISP) y no corre el programa.
 * @{
 */
#define BOTON_GRABAR_PIN     PIN_11 ///< P2.11, EINT1.
#define BOTON_REPRODUCIR_PIN PIN_12 ///< P2.12, EINT2.
/** @} */

/**
 * Máximo de pasos de una secuencia grabada. Se guarda un paso por tick, así
 * que 1500 pasos son 30 segundos.
 */
#define SECUENCIA_MAX 1500

/**
 * @name Comunicación con la PC
 * UART2: TX en P0.10 (J2-40) y RX en P0.11 (J2-41), con un adaptador de 3,3 V.
 * @{
 */
#define UART_BAUDIOS    115200
#define TELEMETRIA_CADA 2 ///< Cada cuántos ticks se manda el estado a la PC (2 = 25 veces por segundo).
/** @} */

#endif
