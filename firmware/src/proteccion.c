/**
 * @file proteccion.c
 * @brief Umbral de sobrecorriente con el DAC y aviso por EINT3.
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_exti.h"

#include "config.h"
#include "proteccion.h"
#include "servos.h"

/**
 * Corriente que da 3,3 V a la salida del amplificador, en mA. Es lo máximo que
 * se puede medir y lo máximo que puede poner el DAC: con 0,22 Ω y ganancia 19
 * da unos 789 mA.
 */
#define CORRIENTE_MAX_MA (3.3f * 1000.0f / (R_SHUNT_OHM * GANANCIA_AMPLI))

static uint32_t umbral_actual_ma;

/** La pone en 1 la interrupción. La lee y la borra proteccion_disparo(). */
static volatile uint8_t disparo = 0;

void proteccion_init(void) {
    DAC_Init(); // ya configura el pin P0.26 como salida AOUT
    proteccion_set_umbral(UMBRAL_INICIAL_MA);

    // El pin lo maneja el comparador. El pull-down es para que, si el
    // comparador todavía no está conectado, el pin no quede al aire y salte solo.
    EXTI_PinConfig(EXTI_EINT3, EXTI_PULLDOWN);

    EXTI_CFG_T eint;
    eint.line = EXTI_EINT3;
    eint.mode = EXTI_EDGE_SENSITIVE;
    eint.polarity = EXTI_RISING_EDGE;
    EXTI_ConfigEnable(&eint);
}

void proteccion_set_umbral(uint32_t umbral_ma) {
    if (umbral_ma > CORRIENTE_MAX_MA) {
        umbral_ma = (uint32_t)CORRIENTE_MAX_MA;
    }
    umbral_actual_ma = umbral_ma;

    // Corriente -> tensión en la salida del amplificador -> valor del DAC (10 bits, 3,3 V)
    float tension = umbral_ma / 1000.0f * R_SHUNT_OHM * GANANCIA_AMPLI;
    DAC_UpdateValue((uint32_t)(tension / 3.3f * 1023.0f));
}

uint32_t proteccion_get_umbral(void) {
    return umbral_actual_ma;
}

uint8_t proteccion_disparo(void) {
    if (disparo) {
        disparo = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief La corriente de la garra pasó el umbral.
 *
 * Lo primero es abrir la garra para que deje de forzar. Del resto (pasar a
 * modo falla y avisar a la PC) se encarga el main.
 */
void EINT3_IRQHandler(void) {
    EXTI_ClearFlag(EXTI_EINT3);
    servo_set_us(SERVO_GARRA, GARRA_ABIERTA_US);
    disparo = 1;
}
