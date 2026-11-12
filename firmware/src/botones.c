/**
 * @file botones.c
 * @brief Pulsadores con interrupción externa y antirrebote por ticks.
 *
 * Un pulsador rebota varias veces al apretarlo y al soltarlo, y cada rebote es
 * un flanco nuevo. Para no contar de más:
 * - la interrupción solo avisa que hubo un flanco;
 * - en el tick siguiente se mira el pin, y si sigue en 0 es una pulsación de verdad
 *   (si está en 1 era el rebote de soltarlo);
 * - después de aceptar una pulsación, ese botón se ignora un rato.
 */

#include "LPC17xx.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_gpio.h"

#include "botones.h"
#include "config.h"

/** Ticks que se ignora un botón después de apretarlo (300 ms). */
#define TICKS_ANTIRREBOTE (300 / TICK_MS)

/** Las interrupciones ponen en 1 su aviso cuando ven un flanco de bajada. */
static volatile uint8_t aviso[2];

/** Ticks que faltan para volver a hacerle caso a cada botón. */
static uint8_t bloqueo[2];

static const uint32_t pin_boton[2] = {BOTON_GRABAR_PIN, BOTON_REPRODUCIR_PIN};

void botones_init(void) {
    EXTI_PinConfig(EXTI_EINT1, EXTI_PULLUP);
    EXTI_PinConfig(EXTI_EINT2, EXTI_PULLUP);

    EXTI_CFG_T eint;
    eint.mode = EXTI_EDGE_SENSITIVE;
    eint.polarity = EXTI_FALLING_EDGE;

    eint.line = EXTI_EINT1;
    EXTI_ConfigEnable(&eint);

    eint.line = EXTI_EINT2;
    EXTI_ConfigEnable(&eint);
}

uint8_t botones_apretado(uint8_t boton) {
    if (bloqueo[boton] > 0) {
        bloqueo[boton]--;
        aviso[boton] = 0;
        return 0;
    }

    if (!aviso[boton]) {
        return 0;
    }
    aviso[boton] = 0;

    // El pin se puede leer aunque esté configurado como EINT
    uint8_t pin_en_cero = (GPIO_ReadValue(PORT_2) & (1UL << pin_boton[boton])) == 0;
    if (pin_en_cero) {
        bloqueo[boton] = TICKS_ANTIRREBOTE;
        return 1;
    }
    return 0;
}

/** @brief Flanco de bajada en P2.11 (grabar). */
void EINT1_IRQHandler(void) {
    EXTI_ClearFlag(EXTI_EINT1);
    aviso[BOTON_GRABAR] = 1;
}

/** @brief Flanco de bajada en P2.12 (reproducir). */
void EINT2_IRQHandler(void) {
    EXTI_ClearFlag(EXTI_EINT2);
    aviso[BOTON_REPRODUCIR] = 1;
}
