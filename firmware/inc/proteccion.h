/**
 * @file proteccion.h
 * @brief Protección por sobrecorriente de la garra.
 *
 * El DAC pone la tensión de umbral en la entrada negativa del comparador
 * (mitad B del MCP6002) y la positiva recibe la tensión de la corriente
 * amplificada. Cuando la corriente pasa el umbral, la salida del comparador
 * sube y entra por el EINT3 (P2.13).
 */

#ifndef PROTECCION_H_
#define PROTECCION_H_

#include <stdint.h>

/**
 * @brief Arranca el DAC con el umbral inicial y configura el EINT3 por flanco
 * de subida.
 */
void proteccion_init(void);

/**
 * @brief Cambia la corriente a la que salta la protección.
 *
 * @param umbral_ma Corriente en mA. Si es más de lo que se puede medir, queda
 *                  en el máximo.
 */
void proteccion_set_umbral(uint32_t umbral_ma);

/**
 * @brief Devuelve el umbral que está puesto ahora.
 *
 * @return Corriente en mA.
 */
uint32_t proteccion_get_umbral(void);

/**
 * @brief Se fija si saltó la protección desde la última vez que se preguntó.
 *
 * @return 1 si saltó, 0 si no.
 */
uint8_t proteccion_disparo(void);

#endif
