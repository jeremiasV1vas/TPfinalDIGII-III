/**
 * @file servos.h
 * @brief Servos del brazo esclavo, manejados con el PWM1.
 */

#ifndef SERVOS_H_
#define SERVOS_H_

#include <stdint.h>

/**
 * @name Número de cada servo
 * Es el mismo que su canal de PWM (PWM1.1 a PWM1.4, en P2.0 a P2.3).
 * @{
 */
#define SERVO_BASE   1
#define SERVO_HOMBRO 2
#define SERVO_CODO   3
#define SERVO_GARRA  4
/** @} */

/**
 * @brief Configura el PWM1 con período de 20 ms y deja cada servo en el medio
 * de su recorrido.
 */
void servos_init(void);

/**
 * @brief Manda un servo a la posición que corresponde a ese ancho de pulso.
 *
 * Si el valor se pasa de los límites del servo, se queda en el límite.
 *
 * @param servo    SERVO_BASE, SERVO_HOMBRO, SERVO_CODO o SERVO_GARRA.
 * @param pulso_us Ancho del pulso en µs.
 */
void servo_set_us(uint8_t servo, uint32_t pulso_us);

/**
 * @brief Devuelve el último pulso que se le mandó a un servo.
 *
 * @param servo SERVO_BASE, SERVO_HOMBRO, SERVO_CODO o SERVO_GARRA.
 * @return Ancho del pulso en µs, ya recortado a los límites.
 */
uint32_t servo_get_us(uint8_t servo);

#endif
