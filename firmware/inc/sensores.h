// Lecturas del ADC: los potes del brazo maestro y la corriente de la garra del esclavo.

#ifndef SENSORES_H_
#define SENSORES_H_

#include <stdint.h>

void sensores_init(void);

// Última lectura de un pote del maestro (POTE_BASE, POTE_HOMBRO, etc.), de 0 a 4095.
uint16_t sensores_pote(uint8_t canal);

// Promedio de la corriente de la garra en los últimos 20 ms, en cuentas del ADC (0 a 4095).
uint16_t sensores_corriente(void);

#endif
