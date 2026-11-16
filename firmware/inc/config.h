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
 * @name Recorrido de los servos
 * Pulsos para 0° y 180°, los mismos que usa la librería Servo del Arduino.
 * Los MG90S del esclavo usan todo ese recorrido.
 * @{
 */
#define SERVO_0_GRADOS_US   544
#define SERVO_180_GRADOS_US 2400
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
 * Los valores salen de las mediciones del 16/11 con los brazos armados. Se
 * midieron con un Arduino (0 a 1023), así que acá están multiplicados por 4.
 * El pote gira 270° en todo su recorrido, o sea unas 15 cuentas por grado.
 * @{
 */

/// Base: todo el recorrido del pote va a todo el recorrido del servo. Gira al revés que el maestro, por eso A va a 180° y B a 0°.
#define CAL_BASE_POTE_A    0
#define CAL_BASE_POTE_B    4095
#define CAL_BASE_PULSO_A   SERVO_180_GRADOS_US
#define CAL_BASE_PULSO_B   SERVO_0_GRADOS_US

/// Hombro: recto marca 2182 (90°), y de ahí 90° para cada lado.
#define CAL_HOMBRO_POTE_A  817
#define CAL_HOMBRO_POTE_B  3547
#define CAL_HOMBRO_PULSO_A SERVO_0_GRADOS_US
#define CAL_HOMBRO_PULSO_B SERVO_180_GRADOS_US

/// Codo: recto marca 2022 (90°). Gira al revés que el maestro, por eso A va a 180° y B a 0°.
#define CAL_CODO_POTE_A    657
#define CAL_CODO_POTE_B    3387
#define CAL_CODO_PULSO_A   SERVO_180_GRADOS_US
#define CAL_CODO_PULSO_B   SERVO_0_GRADOS_US

#define CAL_GARRA_POTE_SUELTA   1001 ///< Pote de la palanca del maestro suelta (garra abierta). Más arriba también es abierta.
#define CAL_GARRA_POTE_APRETADA 0    ///< Pote de la palanca apretada a fondo (garra cerrada).
/** @} */

/**
 * Si el pulso nuevo difiere del anterior en menos que esto, el servo no se
 * mueve. Así no tiembla por el ruido que queda en los potes.
 */
#define ZONA_MUERTA_US 4

/**
 * @name Garra del esclavo
 * Pulso con la garra abierta del todo y cerrada del todo (mandíbulas juntas).
 * La garra cierra en 0° y abre en 180°.
 * @{
 */
#define GARRA_ABIERTA_US SERVO_180_GRADOS_US
#define GARRA_CERRADA_US SERVO_0_GRADOS_US
/** @} */

/**
 * @name Control de fuerza de la garra
 * Con la referencia tomada, la palanca del maestro ya no pide una posición
 * sino una carga, en cuentas del ADC (unos 6 mA por cuenta): apenas apretada
 * pide CARGA_CONTACTO y a fondo pide CARGA_MAX. La garra primero se cierra de
 * a poco hasta que la carga pasa CARGA_CONTACTO (tocó el objeto), y después
 * el PID ajusta cuánto cierra para llegar a la carga pedida. Ver garra.c.
 *
 * CARGA_CONTACTO tiene que quedar por encima de lo que marca la carga con la
 * garra cerrándose en el aire (moverse también consume), y por debajo de lo
 * que marca apretando algo. Se ajusta mirando "Carga garra" en la interfaz.
 * Las ganancias salen de una simulación y hay que ajustarlas en la placa.
 * @{
 */
#define CARGA_MAX          80   ///< Carga pedida con la palanca a fondo (unos 480 mA).
#define CARGA_CONTACTO     30   ///< Carga a partir de la cual se toma que tocó el objeto (unos 180 mA).
#define VELOCIDAD_BUSQUEDA 25   ///< Cuánto se cierra por tick mientras busca el objeto (de 0 a 1000 en 0,8 s).
#define PID_KP             0.2f ///< Cierre (de 0 a 1000) por cada cuenta de error.
#define PID_KI             5.0f ///< Cierre por cada cuenta de error acumulada durante un segundo.
#define PID_KD             0.0f ///< La derivada queda en 0: con el ruido de la medición no ayudaba.
/** @} */

/**
 * @name Pines de los botones
 * Pulsadores a masa con el pull-up interno. No usamos el EINT0 (P2.10) porque
 * si ese pin está en 0 cuando arranca la placa, entra al modo de grabación
 * (ISP) y no corre el programa.
 * @{
 */
#define BOTON_GRABAR_PIN     PIN_11 ///< P2.11 (J2-52), EINT1.
#define BOTON_REPRODUCIR_PIN PIN_12 ///< P2.12 (J2-53), EINT2.
#define BOTON_REFERENCIA_PIN PIN_13 ///< P2.13 (J2-27), EINT3.
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
