/**
 * @file servos.c
 * @brief Servos del brazo esclavo con el PWM1, de a 1 µs.
 */

#include "lpc17xx_pwm.h"

#include "config.h"
#include "servos.h"

/**
 * Límites de cada servo en µs, para que ninguno choque con la estructura.
 * Por ahora 1000 a 2000 en todos; hay que ajustarlos con el brazo armado.
 * La posición 0 no se usa, así el número de servo sirve directo como índice.
 */
static const uint32_t pulso_min[5] = {0, 1000, 1000, 1000, 1000};
static const uint32_t pulso_max[5] = {0, 2000, 2000, 2000, 2000};

/** Último pulso que se le mandó a cada servo. */
static uint32_t pulso_actual[5];

void servos_init(void) {
    // El contador del PWM avanza de a 1 µs, así los valores de match quedan en µs
    PWM_TIMERCFG_T timer;
    timer.prescaleOpt = PWM_US;
    timer.prescaleValue = 1;
    PWM_InitTimer(&timer);

    // El match 0 marca el período: a los 20 ms el contador vuelve a cero
    PWM_MATCHCFG_T match;
    match.channel = PWM_MATCH_0;
    match.intEn = DISABLE;
    match.stopEn = DISABLE;
    match.resetEn = ENABLE;
    match.matchValue = SERVO_PERIODO_US;
    PWM_ConfigMatch(&match);

    PWM_PinConfig(PWM1_P2_0);
    PWM_PinConfig(PWM2_P2_1);
    PWM_PinConfig(PWM3_P2_2);
    PWM_PinConfig(PWM4_P2_3);

    // Cada servo arranca en el medio de su recorrido
    for (uint8_t servo = SERVO_BASE; servo <= SERVO_GARRA; servo++) {
        PWM_ChannelConfig(servo, PWM_SINGLE_EDGE);

        pulso_actual[servo] = (pulso_min[servo] + pulso_max[servo]) / 2;

        match.channel = servo;
        match.resetEn = DISABLE;
        match.matchValue = pulso_actual[servo];
        PWM_ConfigMatch(&match);

        PWM_ChannelEnable(servo);
    }

    PWM_ResetCounter();
    PWM_CounterEnable();
    PWM_Enable();
}

void servo_set_us(uint8_t servo, uint32_t pulso_us) {
    if (servo < SERVO_BASE || servo > SERVO_GARRA) {
        return;
    }

    if (pulso_us < pulso_min[servo]) {
        pulso_us = pulso_min[servo];
    }
    if (pulso_us > pulso_max[servo]) {
        pulso_us = pulso_max[servo];
    }

    pulso_actual[servo] = pulso_us;

    // El driver deja el valor nuevo esperando y el PWM lo toma al empezar el
    // próximo período, así nunca sale un pulso cortado a la mitad.
    PWM_MatchUpdateSingle(servo, pulso_us);
}

uint32_t servo_get_us(uint8_t servo) {
    if (servo < SERVO_BASE || servo > SERVO_GARRA) {
        return 0;
    }
    return pulso_actual[servo];
}
