/**
 * @file serie.h
 * @brief Comunicación con la PC por la UART2.
 *
 * Todo viaja como texto, una línea por mensaje terminada en '\n', para poder
 * verlo también con cualquier terminal serie.
 */

#ifndef SERIE_H_
#define SERIE_H_

/**
 * @brief Configura la UART2 (TX en P0.10 y RX en P0.11) a UART_BAUDIOS, 8N1.
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

#endif
