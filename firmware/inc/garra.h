/**
 * @file garra.h
 * @brief Garra del esclavo y su lectura de carga.
 *
 * La palanca del maestro da un "cierre" de 0 (suelta) a 1000 (apretada a fondo).
 * Por ahora el cierre va directo a la posición de la garra. Más adelante acá
 * se suma el PID de fuerza.
 *
 * La carga sale del divisor 3,3k/2,2k que mide la tensión del servo de la garra
 * después de R_SENSE (3 resistencias de 1 Ω en paralelo, 0,33 Ω). Cuando la
 * garra aprieta algo, el servo pide más corriente, cae más tensión en R_SENSE
 * y la lectura del ADC baja. Como esa lectura también sube y baja con la
 * fuente de 5 V, no se mira el valor solo sino la diferencia contra una
 * referencia tomada con la garra libre (con el botón de P2.13).
 *
 * Cada cuenta de diferencia son unos 6 mA:
 * 3,3 V / 4096 = 0,8 mV en el ADC, que son 2 mV antes del divisor (0,4), y
 * 2 mV / 0,33 Ω = 6 mA.
 */

#ifndef GARRA_H_
#define GARRA_H_

#include <stdint.h>

/**
 * @brief Deja la garra abierta.
 */
void garra_init(void);

/**
 * @brief Mueve la garra según lo que pide el maestro.
 *
 * Se llama una vez por tick.
 *
 * @param cierre De 0 (palanca suelta) a 1000 (apretada a fondo).
 */
void garra_actualizar(uint16_t cierre);

/**
 * @brief Guarda la lectura actual del divisor como referencia.
 *
 * Hay que llamarla con la garra libre, sin agarrar nada y sin forzar.
 */
void garra_tomar_referencia(void);

/**
 * @brief Lectura del divisor que se guardó como referencia.
 *
 * @return Cuentas del ADC, o 0 si todavía no se tomó.
 */
uint16_t garra_referencia(void);

/**
 * @brief Cuánto bajó la lectura del divisor respecto de la referencia.
 *
 * Cuanto más aprieta la garra, más grande da. Con la garra libre da cerca de 0,
 * y puede dar un poco negativo por el ruido.
 *
 * @return Diferencia en cuentas del ADC (unos 6 mA por cuenta), o 0 si
 * todavía no se tomó la referencia.
 */
int32_t garra_carga(void);

#endif
