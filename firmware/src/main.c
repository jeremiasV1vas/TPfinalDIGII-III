// TP final Digital III: brazos robóticos maestro-esclavo.
//
// Por ahora solo hace parpadear el LED rojo, para comprobar que el
// proyecto compila, se graba y que el SysTick anda.

#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"

#include "config.h"

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

void SysTick_Handler(void) {
    ticks++;
}

int main(void) {
    // SystemInit() ya lo llama el código de arranque de MCUXpresso antes de main
    led_init();

    SYSTICK_InternalInit(TICK_MS);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    uint32_t ultimo_cambio = 0;

    while (1) {
        // Cambiamos el LED cada medio segundo
        if (ticks - ultimo_cambio >= 500 / TICK_MS) {
            ultimo_cambio = ticks;
            GPIO_TogglePins(LED_PORT, 1UL << LED_PIN);
        }
    }

    return 0;
}
