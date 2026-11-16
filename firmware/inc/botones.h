/**
 * @file botones.h
 * @brief Pulsadores para grabar, reproducir y tomar la referencia de la
 * garra, por EINT1, EINT2 y EINT3.
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
#define BOTON_REFERENCIA 2 ///< P2.13, EINT3.
/** @} */

/**
 * @brief Configura los tres pines como EINT con pull-up y flanco de bajada.
 */
void botones_init(void);

/**
 * @brief Se fija si apretaron un botón desde la última vez.
 *
 * Hay que llamarla una vez por tick para cada botón, porque también lleva la
 * cuenta del antirrebote.
 *
 * @param boton BOTON_GRABAR, BOTON_REPRODUCIR o BOTON_REFERENCIA.
 * @return 1 si hubo una pulsación nueva, 0 si no.
 */
uint8_t botones_apretado(uint8_t boton);

#endif
