/**
 * @file secuencia.h
 * @brief Secuencia de movimientos grabada en RAM, para después reproducirla.
 */

#ifndef SECUENCIA_H_
#define SECUENCIA_H_

#include <stdint.h>

/**
 * Un paso de la secuencia: lo que pedía el maestro en un tick.
 */
typedef struct {
    uint16_t base;   ///< Pulso de la base, en µs.
    uint16_t hombro; ///< Pulso del hombro, en µs.
    uint16_t codo;   ///< Pulso del codo, en µs.
    uint16_t cierre; ///< Cuánto estaba apretada la palanca de la garra, de 0 a 1000.
} paso_t;

/**
 * @brief Borra la secuencia para empezar a grabar una nueva.
 */
void secuencia_borrar(void);

/**
 * @brief Agrega un paso al final de la secuencia.
 *
 * @param paso Paso a guardar.
 * @return 1 si se guardó, 0 si ya no hay lugar.
 */
uint8_t secuencia_agregar(const paso_t* paso);

/**
 * @brief Cantidad de pasos grabados.
 *
 * @return De 0 a SECUENCIA_MAX.
 */
uint16_t secuencia_largo(void);

/**
 * @brief Devuelve un paso de la secuencia.
 *
 * @param numero Número de paso, de 0 a secuencia_largo() - 1.
 * @return Puntero al paso.
 */
const paso_t* secuencia_paso(uint16_t numero);

#endif
