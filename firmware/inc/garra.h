/**
 * @file garra.h
 * @brief Garra del esclavo.
 *
 * La palanca del maestro da un "cierre" de 0 (suelta) a 1000 (apretada a fondo).
 * Por ahora el cierre va directo a la posición de la garra. Más adelante acá
 * se suma la medición de fuerza y el PID.
 */

#ifndef GARRA_H_
#define GARRA_H_

#include <stdint.h>

/**
 * @brief Deja la garra abierta.
 */
void garra_init(void);

/**
 * @brief Mueve la garra según lo que pide el maestro.
 *
 * Se llama una vez por tick.
 *
 * @param cierre De 0 (palanca suelta) a 1000 (apretada a fondo).
 */
void garra_actualizar(uint16_t cierre);

#endif
