/**
 * @file botones.h
 * @brief Pulsadores para grabar y reproducir secuencias, por EINT1 y EINT2.
 *
 * Van entre el pin y masa, con el pull-up interno, así que al apretar el pin
 * baja a 0 y la interrupción salta por flanco de bajada.
 */

#ifndef BOTONES_H_
#define BOTONES_H_

#include <stdint.h>

/**
 * @name Botones
 * @{
 */
#define BOTON_GRABAR     0 ///< P2.11, EINT1.
#define BOTON_REPRODUCIR 1 ///< P2.12, EINT2.
/** @} */

/**
 * @brief Configura los dos pines como EINT con pull-up y flanco de bajada.
 */
void botones_init(void);

/**
 * @brief Se fija si apretaron un botón desde la última vez.
 *
 * Hay que llamarla una vez por tick para cada botón, porque también lleva la
 * cuenta del antirrebote.
 *
 * @param boton BOTON_GRABAR o BOTON_REPRODUCIR.
 * @return 1 si hubo una pulsación nueva, 0 si no.
 */
uint8_t botones_apretado(uint8_t boton);

#endif
