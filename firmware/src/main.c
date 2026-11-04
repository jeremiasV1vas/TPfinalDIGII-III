// TP final Digital III: brazos robóticos maestro-esclavo.
//
// Por ahora es una prueba de los servos y del ADC: cada servo del esclavo
// sigue directo al pote del maestro que le corresponde, sin calibrar.
// El LED parpadea para ver que el programa sigue andando.

#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"

#include "config.h"
#include "sensores.h"
#include "servos.h"

// Cuenta cuántas veces interrumpió el SysTick (una vez cada TICK_MS).
static volatile uint32_t ticks = 0;

static void led_init(void) {
    PINSEL_CFG_T led;
    led.port = LED_PORT;
    led.pin = LED_PIN;
    led.func = PINSEL_FUNC_00;
    led.mode = PINSEL_PULLUP;
    led.openDrain = DISABLE;
    PINSEL_ConfigPin(&led);

    GPIO_SetDir(LED_PORT, 1UL << LED_PIN, GPIO_OUTPUT);
    GPIO_SetPins(LED_PORT, 1UL << LED_PIN); // arranca apagado
}

// Pasa la lectura de un pote (0 a 4095) a un pulso de servo (1000 a 2000 µs)
static uint32_t pote_a_pulso(uint16_t lectura) {
    return 1000 + (uint32_t)lectura * 1000 / 4095;
}

void SysTick_Handler(void) {
    ticks++;
}

int main(void) {
    // SystemInit() ya lo llama el código de arranque de MCUXpresso antes de main
    led_init();
    servos_init();
    sensores_init();

    SYSTICK_InternalInit(TICK_MS);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    uint32_t ultimo_tick = 0;
    uint32_t ultimo_cambio_led = 0;

    while (1) {
        // Una vez por tick (20 ms) actualizamos los servos
        if (ticks != ultimo_tick) {
            ultimo_tick = ticks;

            servo_set_us(SERVO_BASE, pote_a_pulso(sensores_pote(POTE_BASE)));
            servo_set_us(SERVO_HOMBRO, pote_a_pulso(sensores_pote(POTE_HOMBRO)));
            servo_set_us(SERVO_CODO, pote_a_pulso(sensores_pote(POTE_CODO)));
            servo_set_us(SERVO_GARRA, pote_a_pulso(sensores_pote(POTE_GARRA)));
        }

        // Cambiamos el LED cada medio segundo
        if (ticks - ultimo_cambio_led >= 500 / TICK_MS) {
            ultimo_cambio_led = ticks;
            GPIO_TogglePins(LED_PORT, 1UL << LED_PIN);
        }
    }

    return 0;
}
