/**
 * @file garra.c
 * @brief Garra del esclavo por posición.
 */

#include "config.h"
#include "garra.h"
#include "servos.h"

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
