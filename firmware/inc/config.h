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
 * LED rojo de la LPCXpresso. Se prende con un 0.
 * @{
 */
#define LED_PORT PORT_0
#define LED_PIN  PIN_22
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
 * pote del maestro y el pulso que lleva al esclavo a esa misma posición. Las
 * dos cosas se ven en la interfaz de la PC (el pulso se busca en modo manual).
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
 * Se ajustan en modo manual con el brazo armado.
 * @{
 */
#define GARRA_ABIERTA_US 1000
#define GARRA_CERRADA_US 2000
/** @} */

/**
 * @name Medición de corriente
 * La corriente del servo de la garra pasa por el shunt y el MCP6002 la amplifica.
 * @{
 */
#define R_SHUNT_OHM    0.22f ///< Resistencia de shunt.
#define GANANCIA_AMPLI 19.0f ///< 1 + 18k / 1k. Si ponen la de 15k, queda en 16.
/** @} */

/**
 * @name Corriente y fuerza
 * La fuerza de la garra se calcula con una recta:
 * fuerza (g) = (corriente (mA) - CORRIENTE_REPOSO_MA) * GRAMOS_POR_MA.
 * Los dos números salen de la caracterización (pc/caracterizar.py). Los de
 * abajo son de ejemplo hasta que la hagamos.
 * @{
 */
#define CORRIENTE_REPOSO_MA 20.0f  ///< Corriente con la garra quieta y sin carga.
#define GRAMOS_POR_MA       1.0f   ///< Pendiente de la recta.
#define FUERZA_MAX_G        300.0f ///< Fuerza que se pide con la palanca del maestro apretada a fondo.
/** @} */

/**
 * @name PID de fuerza
 * En modo fuerza la garra primero se cierra despacio hasta tocar el objeto, y
 * después el PID aprieta hasta llegar a la fuerza pedida (ver garra.c).
 * Las ganancias se pueden cambiar desde la PC sin volver a grabar la placa, y
 * cuando estén bien se copian acá.
 * @{
 */
#define VELOCIDAD_BUSQUEDA 10    ///< Cuánto se cierra por tick mientras busca el objeto (de 0 a 1000 en 2 s).
#define FUERZA_CONTACTO_G  20.0f ///< Fuerza a partir de la cual se toma que tocó el objeto.
#define PID_KP             0.05f
#define PID_KI             2.0f
#define PID_KD             0.0f
/** @} */

/**
 * Corriente de la garra a partir de la cual salta la protección, en mA.
 * Con el servo trabado anda por los 700 mA. Se puede cambiar desde la PC.
 */
#define UMBRAL_INICIAL_MA 600

/**
 * @name Pines de las interrupciones externas
 * Pulsadores a masa con el pull-up interno, y la salida del comparador de
 * sobrecorriente. No usamos el EINT0 (P2.10) porque si ese pin está en 0
 * cuando arranca la placa, entra al modo de grabación (ISP) y no corre el programa.
 * @{
 */
#define BOTON_GRABAR_PIN     PIN_11 ///< P2.11, EINT1.
#define BOTON_REPRODUCIR_PIN PIN_12 ///< P2.12, EINT2.
#define SOBRECORRIENTE_PIN   PIN_13 ///< P2.13, EINT3. Salida del comparador.
/** @} */

/**
 * @name Comunicación con la PC
 * UART2: TX en P0.10 y RX en P0.11.
 * @{
 */
#define UART_BAUDIOS    115200
#define TELEMETRIA_CADA 2 ///< Cada cuántos ticks se manda el estado a la PC (2 = 25 veces por segundo).
/** @} */

/**
 * Máximo de pasos de una secuencia grabada. Se guarda un paso por tick, así
 * que 1500 pasos son 30 segundos.
 */
#define SECUENCIA_MAX 1500

#endif
