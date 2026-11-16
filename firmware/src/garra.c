/**
 * @file garra.c
 * @brief Garra del esclavo por posición, y su carga medida con el divisor.
 */

#include "config.h"
#include "garra.h"
#include "sensores.h"
#include "servos.h"

/**
 * Lectura del divisor con la garra libre. Queda en 0 hasta que se aprieta el
 * botón de referencia.
 */
static uint16_t referencia = 0;

/**
 * @brief Pasa un cierre (0 a 1000) a pulso del servo de la garra.
 *
 * Sirve aunque la garra cierre bajando el pulso: en ese caso la resta da negativa.
 */
static uint32_t cierre_a_pulso(int32_t cierre) {
    int32_t recorrido = GARRA_CERRADA_US - GARRA_ABIERTA_US;
    return GARRA_ABIERTA_US + recorrido * cierre / 1000;
}

/**
 * @brief Manda el pulso a la garra solo si cambió más que la zona muerta.
 */
static void mover_garra(uint32_t pulso) {
    int32_t diferencia = (int32_t)pulso - (int32_t)servo_get_us(SERVO_GARRA);
    if (diferencia >= ZONA_MUERTA_US || diferencia <= -ZONA_MUERTA_US) {
        servo_set_us(SERVO_GARRA, pulso);
    }
}

void garra_init(void) {
    servo_set_us(SERVO_GARRA, GARRA_ABIERTA_US);
}

void garra_actualizar(uint16_t cierre) {
    mover_garra(cierre_a_pulso(cierre));
}

void garra_tomar_referencia(void) {
    // sensores_corriente() ya es el promedio de los últimos 20 ms, o sea de un
    // período entero del servo
    referencia = sensores_corriente();
}

uint16_t garra_referencia(void) {
    return referencia;
}

int32_t garra_carga(void) {
    if (referencia == 0) {
        return 0;
    }
    // Al apretar, la lectura baja, por eso se resta la lectura de la referencia
    return (int32_t)referencia - (int32_t)sensores_corriente();
}
