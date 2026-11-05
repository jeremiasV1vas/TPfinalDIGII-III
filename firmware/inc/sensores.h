// Lecturas del ADC: los potes del brazo maestro y la corriente de la garra del esclavo.

#ifndef SENSORES_H_
#define SENSORES_H_

#include <stdint.h>

void sensores_init(void);

// Suma las lecturas nuevas de los potes. Hay que llamarla todo el tiempo desde el while.
void sensores_acumular(void);

// Saca el promedio de lo que se juntó desde la última vez. Se llama una vez por tick.
void sensores_promediar(void);

// Promedio de un pote del maestro (POTE_BASE, POTE_HOMBRO, etc.) en el último tick, de 0 a 4095.
uint16_t sensores_pote(uint8_t canal);

// Promedio de la corriente de la garra en los últimos 20 ms, en cuentas del ADC (0 a 4095).
uint16_t sensores_corriente(void);

#endif
