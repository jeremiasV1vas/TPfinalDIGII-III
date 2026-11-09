/**
 * @file main.c
 * @brief TP final Digital III: brazos robóticos maestro-esclavo.
 *
 * Cada 20 ms (un tick del SysTick):
 * 1. Se promedian los potes del maestro y se mide la corriente de la garra.
 * 2. Según el modo, se decide adónde va el esclavo.
 * 3. Cada dos ticks se manda el estado a la PC.
 *
 * Los modos son:
 * - COPIA: el esclavo sigue al maestro.
 * - GRABAR: igual que copia, pero además guarda cada paso. Se entra y se sale
 *   con el botón de grabar.
 * - REPRODUCIR: repite la secuencia grabada una y otra vez, sin hacerle caso al
 *   maestro. Se entra y se sale con el botón de reproducir.
 * - MANUAL: los servos los mueve la PC. Sirve para calibrar y para caracterizar
 *   la corriente de la garra.
 * - FALLA: saltó la protección de sobrecorriente. La garra se abre y todo queda
 *   quieto. Se sale con el botón de grabar o desde la PC.
 *
 * Cada vez que cambia el modo, el esclavo va despacio (en 1 segundo) desde
 * donde está hasta donde tiene que ir, para que no pegue un salto.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"

#include "botones.h"
#include "config.h"
#include "garra.h"
#include "proteccion.h"
#include "secuencia.h"
#include "sensores.h"
#include "serie.h"
#include "servos.h"

/** Ticks que tarda el esclavo en llegar a la posición nueva al cambiar de modo (1 s). */
#define TICKS_TRANSICION (1000 / TICK_MS)

/** Modos de funcionamiento. El número es el que se usa en los mensajes con la PC. */
typedef enum {
    MODO_COPIA = 0,
    MODO_GRABAR,
    MODO_REPRODUCIR,
    MODO_MANUAL,
    MODO_FALLA
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

/**
 * @brief Muestra el modo con el LED.
 *
 * Copia: parpadeo lento. Grabar: prendido. Reproducir: parpadeo rápido.
 * Manual: apagado. Falla: parpadeo muy rápido.
 */
static void led_actualizar(void) {
    uint32_t cada = 0; // cada cuántos ticks cambia el LED

    switch (modo) {
        case MODO_GRABAR:
            GPIO_ClearPins(LED_PORT, 1UL << LED_PIN);
            return;
        case MODO_MANUAL:
            GPIO_SetPins(LED_PORT, 1UL << LED_PIN);
            return;
        case MODO_COPIA: cada = 500 / TICK_MS; break;
        case MODO_REPRODUCIR: cada = 100 / TICK_MS; break;
        case MODO_FALLA: cada = 2; break;
    }

    if (ticks % cada == 0) {
        GPIO_TogglePins(LED_PORT, 1UL << LED_PIN);
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
    if (nuevo == MODO_FALLA) {
        garra_abrir();
        actual.cierre = 0;
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
        } else if (modo == MODO_GRABAR || modo == MODO_FALLA) {
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
 * @brief Saca los números que vienen después de la letra del comando.
 *
 * Por ejemplo, de "K,500,5000,0" saca 500, 5000 y 0.
 *
 * @return Cuántos números encontró.
 */
static uint8_t leer_numeros(const char* linea, int32_t* numeros, uint8_t maximo) {
    uint8_t cantidad = 0;
    const char* coma = strchr(linea, ',');

    while (coma != NULL && cantidad < maximo) {
        numeros[cantidad] = atoi(coma + 1);
        cantidad++;
        coma = strchr(coma + 1, ',');
    }
    return cantidad;
}

/**
 * @brief Ejecuta un comando que llegó de la PC y contesta $OK o $ERROR.
 *
 * Comandos:
 * - M,modo: cambia de modo (0 copia, 1 grabar, 2 reproducir, 3 manual).
 * - F,0 o F,1: garra en modo posición o en modo fuerza.
 * - K,kp,ki,kd: ganancias del PID multiplicadas por 1000.
 * - U,mA: corriente a la que salta la protección.
 * - S,servo,pulso: mueve un servo (1 a 4) a ese pulso. Solo en modo manual.
 */
static void atender_comando(const char* linea) {
    int32_t n[3] = {0, 0, 0};
    uint8_t cantidad = leer_numeros(linea, n, 3);
    uint8_t ok = 0;

    switch (linea[0]) {
        case 'M':
            if (cantidad == 1 && n[0] >= MODO_COPIA && n[0] <= MODO_MANUAL) {
                ok = cambiar_modo(n[0]);
            }
            break;

        case 'F':
            if (cantidad == 1) {
                garra_set_modo_fuerza(n[0] != 0);
                ok = 1;
            }
            break;

        case 'K':
            if (cantidad == 3) {
                garra_set_ganancias(n[0] / 1000.0f, n[1] / 1000.0f, n[2] / 1000.0f);
                ok = 1;
            }
            break;

        case 'U':
            if (cantidad == 1 && n[0] > 0) {
                proteccion_set_umbral(n[0]);
                ok = 1;
            }
            break;

        case 'S':
            if (cantidad == 2 && modo == MODO_MANUAL && n[0] >= SERVO_BASE && n[0] <= SERVO_GARRA) {
                servo_set_us(n[0], n[1]);
                ok = 1;
            }
            break;
    }

    char respuesta[SERIE_LARGO_LINEA + 10];
    sprintf(respuesta, ok ? "$OK,%s\n" : "$ERROR,%s\n", linea);
    serie_enviar(respuesta);
}

/**
 * @brief Manda el estado a la PC en una línea.
 *
 * $T,modo,fuerza_on,pote_base,pote_hombro,pote_codo,pote_garra,
 * pulso_base,pulso_hombro,pulso_codo,pulso_garra,referencia_g,fuerza_g,
 * corriente_mA,umbral_mA,pasos_grabados
 */
static void enviar_telemetria(void) {
    char texto[120];

    sprintf(texto, "$T,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            (int)modo, (int)garra_modo_fuerza(),
            (int)sensores_pote(POTE_BASE), (int)sensores_pote(POTE_HOMBRO),
            (int)sensores_pote(POTE_CODO), (int)sensores_pote(POTE_GARRA),
            (int)servo_get_us(SERVO_BASE), (int)servo_get_us(SERVO_HOMBRO),
            (int)servo_get_us(SERVO_CODO), (int)servo_get_us(SERVO_GARRA),
            (int)garra_referencia(), (int)garra_fuerza(), (int)garra_corriente(),
            (int)proteccion_get_umbral(), (int)secuencia_largo());

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
    garra_medir();
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

        case MODO_MANUAL:
        case MODO_FALLA:
            // Los servos se quedan donde están (en manual los mueve la PC)
            break;
    }

    led_actualizar();

    if (ticks % TELEMETRIA_CADA == 0) {
        enviar_telemetria();
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
    proteccion_init();
    botones_init();
    serie_init();

    // Arranca en modo copia, yendo despacio desde el medio hasta donde está el maestro
    cambiar_modo(MODO_COPIA);

    SYSTICK_InternalInit(TICK_MS);
    SYSTICK_IntCmd(ENABLE);
    SYSTICK_Cmd(ENABLE);

    uint32_t ultimo_tick = 0;
    char linea[SERIE_LARGO_LINEA];

    while (1) {
        sensores_acumular();

        if (serie_leer_linea(linea)) {
            atender_comando(linea);
        }

        if (proteccion_disparo()) {
            cambiar_modo(MODO_FALLA);
            serie_enviar("$FALLA\n");
        }

        if (ticks != ultimo_tick) {
            ultimo_tick = ticks;
            ciclo();
        }
    }

    return 0;
}
