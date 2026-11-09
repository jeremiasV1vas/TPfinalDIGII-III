/**
 * @file serie.h
 * @brief Comunicación con la PC por la UART2.
 *
 * Todo viaja como texto, una línea por mensaje terminada en '\n', para poder
 * probarlo también con cualquier terminal serie.
 */

#ifndef SERIE_H_
#define SERIE_H_

#include <stdint.h>

/** Largo máximo de una línea que llega de la PC, contando el '\0'. */
#define SERIE_LARGO_LINEA 40

/**
 * @brief Configura la UART2 (P0.10 y P0.11) y prende la interrupción de recepción.
 */
void serie_init(void);

/**
 * @brief Manda un texto a la PC.
 *
 * Espera a que salga todo, así que conviene mandar líneas cortas.
 *
 * @param texto Texto terminado en '\0'.
 */
void serie_enviar(const char* texto);

/**
 * @brief Se fija si llegó una línea completa de la PC.
 *
 * Devuelve de a una línea por vez. Hay que llamarla seguido desde el while
 * del main.
 *
 * @param destino Arreglo de al menos SERIE_LARGO_LINEA caracteres.
 * @return 1 si había una línea nueva, 0 si no.
 */
uint8_t serie_leer_linea(char* destino);

#endif
