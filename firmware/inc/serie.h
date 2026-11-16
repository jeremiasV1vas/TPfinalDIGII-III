/**
 * @file serie.h
 * @brief Comunicación con la PC por la UART2.
 *
 * Todo viaja como texto, una línea por mensaje terminada en '\n', para poder
 * verlo también con cualquier terminal serie.
 *
 * De la PC a la placa van comandos de una letra (ver main.c y el README): la
 * recepción es por interrupción y cada comando se guarda hasta que el main lo
 * atiende.
 */

#ifndef SERIE_H_
#define SERIE_H_

#include <stdint.h>

/** Largo máximo de un comando, sin contar el fin de línea. Lo que sobra se descarta. */
#define COMANDO_LARGO_MAX 16

/**
 * @brief Configura la UART2 (TX en P0.10 y RX en P0.11) a UART_BAUDIOS, 8N1,
 * con la interrupción de recepción habilitada.
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
 * @brief Se fija si llegó un comando completo de la PC.
 *
 * Un comando es una línea de texto terminada en '\n' o en '\r'. Si llegó uno,
 * lo copia sin el fin de línea y deja lugar para el siguiente. Mientras no se
 * lo atiende, lo que siga llegando se descarta.
 *
 * @param comando Donde se copia. Tiene que tener lugar para
 * COMANDO_LARGO_MAX + 1 caracteres.
 * @return 1 si había un comando, 0 si no.
 */
uint8_t serie_recibir(char* comando);

#endif
