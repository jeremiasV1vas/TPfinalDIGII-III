// Servos del brazo esclavo, manejados con el PWM1.

#ifndef SERVOS_H_
#define SERVOS_H_

#include <stdint.h>

// El número de cada servo es el mismo que su canal de PWM (PWM1.1 a PWM1.4).
#define SERVO_BASE   1
#define SERVO_HOMBRO 2
#define SERVO_CODO   3
#define SERVO_GARRA  4

void servos_init(void);

// Manda el servo a la posición que corresponde a ese ancho de pulso, en µs.
// Si el valor se pasa de los límites del servo, se queda en el límite.
void servo_set_us(uint8_t servo, uint32_t pulso_us);

#endif
