/**
 * @file garra.h
 * @brief Garra del esclavo: medición de fuerza y control por posición o por fuerza.
 *
 * La palanca del maestro da un "cierre" de 0 (suelta) a 1000 (apretada a fondo).
 * - En modo posición, el cierre va directo a la posición de la garra.
 * - En modo fuerza, el cierre es la fuerza que se pide, y un PID mueve la garra
 *   hasta que la fuerza medida (por la corriente del servo) llega a ese valor.
 */

#ifndef GARRA_H_
#define GARRA_H_

#include <stdint.h>

/**
 * @brief Deja la garra en modo posición, abierta, con las ganancias de config.h.
 */
void garra_init(void);

/**
 * @brief Lee la corriente del servo y calcula la fuerza.
 *
 * Se llama una vez por tick, en todos los modos, así la PC siempre ve la
 * corriente (también al caracterizar).
 */
void garra_medir(void);

/**
 * @brief Mueve la garra según lo que pide el maestro.
 *
 * Se llama una vez por tick, después de garra_medir().
 *
 * @param cierre De 0 (palanca suelta) a 1000 (apretada a fondo).
 */
void garra_actualizar(uint16_t cierre);

/**
 * @brief Abre la garra del todo y reinicia el PID.
 */
void garra_abrir(void);

/**
 * @brief Elige entre modo posición y modo fuerza.
 *
 * @param activo 1 para modo fuerza, 0 para modo posición.
 */
void garra_set_modo_fuerza(uint8_t activo);

/**
 * @brief Dice si está en modo fuerza.
 *
 * @return 1 en modo fuerza, 0 en modo posición.
 */
uint8_t garra_modo_fuerza(void);

/**
 * @brief Cambia las ganancias del PID. Reinicia la parte integral.
 *
 * @param kp Proporcional, en milésimas de cierre por gramo.
 * @param ki Integral, en milésimas de cierre por gramo y por segundo.
 * @param kd Derivativa, en milésimas de cierre por gramo/segundo.
 */
void garra_set_ganancias(float kp, float ki, float kd);

/**
 * @brief Corriente del servo de la garra en el último tick.
 * @return Corriente en mA.
 */
float garra_corriente(void);

/**
 * @brief Fuerza de la garra calculada con la corriente.
 * @return Fuerza en gramos.
 */
float garra_fuerza(void);

/**
 * @brief Fuerza que se le pide a la garra en modo fuerza.
 * @return Fuerza en gramos. En modo posición es 0.
 */
float garra_referencia(void);

#endif
