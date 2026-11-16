/**
 * @file sensores.h
 * @brief Lecturas del ADC: los potes del brazo maestro y la corriente de la
 * garra del esclavo.
 */

#ifndef SENSORES_H_
#define SENSORES_H_

#include <stdint.h>

/**
 * @brief Arranca el ADC en modo burst con los cinco canales y el DMA que va
 * guardando la corriente de la garra.
 */
void sensores_init(void);

/**
 * @brief Suma las lecturas nuevas de los potes.
 *
 * Hay que llamarla todo el tiempo desde el while del main.
 */
void sensores_acumular(void);

/**
 * @brief Saca el promedio de lo que se juntó desde la última vez.
 *
 * Se llama una vez por tick, antes de usar los potes.
 */
void sensores_promediar(void);

/**
 * @brief Promedio de un pote del maestro en el último tick.
 *
 * @param canal POTE_BASE, POTE_HOMBRO, POTE_CODO o POTE_GARRA.
 * @return Lectura de 0 a 4095.
 */
uint16_t sensores_pote(uint8_t canal);

/**
 * @brief Promedio de la corriente de la garra en los últimos 20 ms.
 *
 * @return Lectura del ADC de 0 a 4095, sin pasar a mA.
 */
uint16_t sensores_corriente(void);

#endif
