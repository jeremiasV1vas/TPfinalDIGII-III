/**
 * @file main.c
 * @brief TP final Digital III: brazos robóticos maestro-esclavo.
 *
 * Cada 20 ms (un tick del SysTick) se promedian los potes del maestro y,
 * según el modo, se decide adónde va el esclavo.
 *
 * Los modos son:
 * - COPIA: el esclavo sigue al maestro.
 * - GRABAR: igual que copia, pero además guarda cada paso. Se entra y se sale
 *   con el botón de grabar.
 * - REPRODUCIR: repite la secuencia grabada una y otra vez, sin hacerle caso al
 *   maestro. Se entra y se sale con el botón de reproducir.
 *
 * Cada vez que cambia el modo, el esclavo va despacio (en 1 segundo) desde
 * donde está hasta donde tiene que ir, para que no pegue un salto.
 *
 * Cada TELEMETRIA_CADA ticks se manda el estado a la PC por la UART, para
 * verlo en pc/interfaz.html.
 */

#include <stdio.h>

#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"

#include "botones.h"
#include "config.h"
#include "garra.h"
#include "secuencia.h"
#include "sensores.h"
#include "serie.h"
#include "servos.h"

/** Ticks que tarda el esclavo en llegar a la posición nueva al cambiar de modo (1 s). */
#define TICKS_TRANSICION (1000 / TICK_MS)

/** Modos de funcionamiento. */
typedef enum {
    MODO_COPIA = 0,
    MODO_GRABAR,
    MODO_REPRODUCIR
} modo_t;

/** Cuenta cuántas veces interrumpió el SysTick (una vez cada TICK_MS). */
static volatile uint32_t ticks = 0;

static modo_t modo = MODO_COPIA;

/** Lo último que se le pidió al esclavo. */
static paso_t actual;

/** Desde dónde arrancó la transición y cuántos ticks lleva. */
static paso_t desde;
static uint16_t transicion = 0;

/** Paso de la secuencia que toca reproducir. */
static uint16_t paso_actual = 0;

/* ---------------------------------------------------------------------------
 * LED
 * ------------------------------------------------------------------------- */

/** @brief Configura un pin del LED como salida y lo deja apagado. */
static void led_pin_init(LPC_PORT port, uint32_t pin) {
    PINSEL_CFG_T led;
    led.port = port;
    led.pin = pin;
    led.func = PINSEL_FUNC_00;
    led.mode = PINSEL_PULLUP;
    led.openDrain = DISABLE;
    PINSEL_ConfigPin(&led);

    GPIO_SetDir(port, 1UL << pin, GPIO_OUTPUT);
    GPIO_SetPins(port, 1UL << pin); // con un 1 está apagado
}

static void led_init(void) {
    led_pin_init(LED_ROJO_PORT, LED_ROJO_PIN);
    led_pin_init(LED_VERDE_PORT, LED_VERDE_PIN);
}

/**
 * @brief Muestra el modo con el LED.
 *
 * Copia: rojo parpadeando lento. Grabar: rojo fijo. Reproducir: verde fijo.
 */
static void led_actualizar(void) {
    switch (modo) {
        case MODO_COPIA:
            GPIO_SetPins(LED_VERDE_PORT, 1UL << LED_VERDE_PIN);
            if (ticks % (500 / TICK_MS) == 0) {
                GPIO_TogglePins(LED_ROJO_PORT, 1UL << LED_ROJO_PIN);
            }
            break;

        case MODO_GRABAR:
            GPIO_SetPins(LED_VERDE_PORT, 1UL << LED_VERDE_PIN);
            GPIO_ClearPins(LED_ROJO_PORT, 1UL << LED_ROJO_PIN);
            break;

        case MODO_REPRODUCIR:
            GPIO_SetPins(LED_ROJO_PORT, 1UL << LED_ROJO_PIN);
            GPIO_ClearPins(LED_VERDE_PORT, 1UL << LED_VERDE_PIN);
            break;
    }
}

/* ---------------------------------------------------------------------------
 * Cuentas
 * ------------------------------------------------------------------------- */

/**
 * @brief Regla de tres entre dos puntos: cuando x vale x_a da y_a, y cuando
 * vale x_b da y_b.
 */
static int32_t regla_de_tres(int32_t x, int32_t x_a, int32_t x_b, int32_t y_a, int32_t y_b) {
    return y_a + (x - x_a) * (y_b - y_a) / (x_b - x_a);
}

/** @brief Deja el valor entre mínimo y máximo. */
static int32_t limitar(int32_t valor, int32_t minimo, int32_t maximo) {
    if (valor < minimo) {
        return minimo;
    }
    if (valor > maximo) {
        return maximo;
    }
    return valor;
}

/* ---------------------------------------------------------------------------
 * Movimiento del esclavo
 * ------------------------------------------------------------------------- */

/**
 * @brief Lee el maestro y lo pasa a pulsos con la calibración de config.h.
 */
static void leer_maestro(paso_t* paso) {
    int32_t base = regla_de_tres(sensores_pote(POTE_BASE), CAL_BASE_POTE_A, CAL_BASE_POTE_B,
                                 CAL_BASE_PULSO_A, CAL_BASE_PULSO_B);
    int32_t hombro = regla_de_tres(sensores_pote(POTE_HOMBRO), CAL_HOMBRO_POTE_A, CAL_HOMBRO_POTE_B,
                                   CAL_HOMBRO_PULSO_A, CAL_HOMBRO_PULSO_B);
    int32_t codo = regla_de_tres(sensores_pote(POTE_CODO), CAL_CODO_POTE_A, CAL_CODO_POTE_B,
                                 CAL_CODO_PULSO_A, CAL_CODO_PULSO_B);
    int32_t cierre = regla_de_tres(sensores_pote(POTE_GARRA), CAL_GARRA_POTE_SUELTA,
                                   CAL_GARRA_POTE_APRETADA, 0, 1000);

    // Los límites finos de cada servo los pone servos.c. Acá solo se evita que
    // una calibración mal puesta dé un número negativo.
    paso->base = limitar(base, 0, 3000);
    paso->hombro = limitar(hombro, 0, 3000);
    paso->codo = limitar(codo, 0, 3000);
    paso->cierre = limitar(cierre, 0, 1000);
}

/**
 * @brief Manda el pulso al servo solo si cambió más que la zona muerta.
 */
static void mover_si_cambia(uint8_t servo, uint32_t pulso) {
    int32_t diferencia = (int32_t)pulso - (int32_t)servo_get_us(servo);
    if (diferencia >= ZONA_MUERTA_US || diferencia <= -ZONA_MUERTA_US) {
        servo_set_us(servo, pulso);
    }
}

/**
 * @brief Arranca una transición desde donde está el esclavo ahora.
 */
static void empezar_transicion(void) {
    desde.base = servo_get_us(SERVO_BASE);
    desde.hombro = servo_get_us(SERVO_HOMBRO);
    desde.codo = servo_get_us(SERVO_CODO);
    desde.cierre = actual.cierre;
    transicion = 0;
}

/** @brief Punto intermedio entre a y b según cuánto lleva la transición. */
static uint16_t interpolar(uint16_t a, uint16_t b) {
    return a + ((int32_t)b - (int32_t)a) * transicion / TICKS_TRANSICION;
}

/**
 * @brief Mueve el esclavo hacia el objetivo, pasando por la transición si
 * todavía no terminó.
 */
static void mover_esclavo(const paso_t* objetivo) {
    if (transicion < TICKS_TRANSICION) {
        actual.base = interpolar(desde.base, objetivo->base);
        actual.hombro = interpolar(desde.hombro, objetivo->hombro);
        actual.codo = interpolar(desde.codo, objetivo->codo);
        actual.cierre = interpolar(desde.cierre, objetivo->cierre);
        transicion++;
    } else {
        actual = *objetivo;
    }

    mover_si_cambia(SERVO_BASE, actual.base);
    mover_si_cambia(SERVO_HOMBRO, actual.hombro);
    mover_si_cambia(SERVO_CODO, actual.codo);
    garra_actualizar(actual.cierre);
}

/**
 * @brief Siguiente paso de la secuencia grabada. Al llegar al final vuelve a
 * empezar, pasando de nuevo por una transición.
 */
static void siguiente_paso(paso_t* paso) {
    *paso = *secuencia_paso(paso_actual);

    // Mientras dura la transición se queda en el primer paso
    if (transicion < TICKS_TRANSICION) {
        return;
    }

    paso_actual++;
    if (paso_actual >= secuencia_largo()) {
        paso_actual = 0;
        empezar_transicion();
    }
}

/* ---------------------------------------------------------------------------
 * Modos
 * ------------------------------------------------------------------------- */

/**
 * @brief Pasa a otro modo.
 * @return 1 si se pudo, 0 si no (por ejemplo, reproducir sin nada grabado).
 */
static uint8_t cambiar_modo(modo_t nuevo) {
    if (nuevo == MODO_REPRODUCIR && secuencia_largo() == 0) {
        return 0;
    }

    if (nuevo == MODO_GRABAR) {
        secuencia_borrar();
    }
    if (nuevo == MODO_REPRODUCIR) {
        paso_actual = 0;
    }

    modo = nuevo;
    empezar_transicion();
    return 1;
}

static void atender_botones(void) {
    // Hay que llamar a botones_apretado() con los dos botones en cada tick,
    // por eso se guardan antes de preguntar
    uint8_t grabar = botones_apretado(BOTON_GRABAR);
    uint8_t reproducir = botones_apretado(BOTON_REPRODUCIR);

    if (grabar) {
        if (modo == MODO_COPIA) {
            cambiar_modo(MODO_GRABAR);
        } else if (modo == MODO_GRABAR) {
            cambiar_modo(MODO_COPIA);
        }
    }

    if (reproducir) {
        if (modo == MODO_COPIA) {
            cambiar_modo(MODO_REPRODUCIR);
        } else if (modo == MODO_REPRODUCIR) {
            cambiar_modo(MODO_COPIA);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Comunicación con la PC
 * ------------------------------------------------------------------------- */

/**
 * @brief Manda el estado a la PC en una línea:
 *
 * $T,modo,pote_base,pote_hombro,pote_codo,pote_garra,
 * pulso_base,pulso_hombro,pulso_codo,pulso_garra,pasos_grabados
 */
static void enviar_estado(void) {
    char texto[80];

    sprintf(texto, "$T,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            (int)modo,
            (int)sensores_pote(POTE_BASE), (int)sensores_pote(POTE_HOMBRO),
            (int)sensores_pote(POTE_CODO), (int)sensores_pote(POTE_GARRA),
            (int)servo_get_us(SERVO_BASE), (int)servo_get_us(SERVO_HOMBRO),
            (int)servo_get_us(SERVO_CODO), (int)servo_get_us(SERVO_GARRA),
            (int)secuencia_largo());

    serie_enviar(texto);
}

/* ---------------------------------------------------------------------------
 * Programa principal
 * ------------------------------------------------------------------------- */

/**
 * @brief Todo lo que se hace una vez por tick.
 */
static void ciclo(void) {
    sensores_promediar();
    atender_botones();

    paso_t objetivo;

    switch (modo) {
        case MODO_COPIA:
            leer_maestro(&objetivo);
            mover_esclavo(&objetivo);
            break;

        case MODO_GRABAR:
            leer_maestro(&objetivo);
            mover_esclavo(&objetivo);
            // Si se llenó la memoria, se deja de grabar y queda lo que entró
            if (!secuencia_agregar(&actual)) {
                cambiar_modo(MODO_COPIA);
            }
            break;

        case MODO_REPRODUCIR:
            siguiente_paso(&objetivo);
            mover_esclavo(&objetivo);
            break;
    }

    led_actualizar();

    if (ticks % TELEMETRIA_CADA == 0) {
        enviar_estado();
    }
}

void SysTick_Handler(void) {
    ticks++;
}

int main(void) {
    // SystemInit() ya lo llama el código de arranque de MCUXpresso antes de main
    led_init();
    servos_init();
    sensores_init();
    garra_init();
    botones_init();
    serie_init();

    // Arranca en modo copia, yendo despacio desde el medio hasta donde está el maestro
    cambiar_modo(MODO_COPIA);

    SYSTICK_InternalInit(TICK_MS);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    uint32_t ultimo_tick = 0;

    while (1) {
        sensores_acumular();

        if (ticks != ultimo_tick) {
            ultimo_tick = ticks;
            ciclo();
        }
    }

    return 0;
}
