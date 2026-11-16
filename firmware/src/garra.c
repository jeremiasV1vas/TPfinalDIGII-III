/**
 * @file garra.c
 * @brief Garra del esclavo: carga medida con el divisor y PID de fuerza.
 *
 * El servo de la garra es de posición: no se le puede pedir una fuerza. Pero
 * cuando la garra ya tocó el objeto, cuanto más se le pide que cierre, más
 * corriente toma y más aprieta. Por eso, con la referencia tomada, la garra
 * trabaja en dos etapas:
 * 1. Buscar: se va cerrando a velocidad constante hasta que la carga pasa
 *    CARGA_CONTACTO durante unos ticks seguidos, o sea, hasta que tocó algo.
 * 2. Apretar: el PID decide cuánto cerrar más allá del punto donde tocó, para
 *    que la carga medida sea la que pide la palanca del maestro.
 *
 * Si se suelta la palanca, la garra se abre y la próxima vez vuelve a buscar.
 * Mientras no se tome la referencia no hay cómo medir la carga, así que la
 * garra va por posición, igual que las otras articulaciones.
 */

#include "config.h"
#include "garra.h"
#include "sensores.h"
#include "servos.h"

/** Tiempo entre dos pasos del PID, en segundos. */
#define DT (TICK_MS / 1000.0f)

/** Con la palanca por debajo de esto (5 %) se toma como suelta y la garra se abre. */
#define CIERRE_MINIMO 50

/**
 * Ticks seguidos con la carga arriba de CARGA_CONTACTO para dar por hecho que
 * tocó algo. Así un pico suelto de corriente no se confunde con el objeto.
 */
#define TICKS_CONTACTO 3

/**
 * Lectura del divisor con la garra libre. Queda en 0 hasta que se aprieta el
 * botón de referencia.
 */
static uint16_t referencia = 0;

static float carga_pedida = 0;   ///< Lo que pide la palanca, en cuentas.
static float carga_anterior = 0; ///< Carga del tick anterior, para la derivada.

static uint8_t en_contacto = 0;     ///< 1 mientras la garra está apretando el objeto.
static uint8_t ticks_arriba = 0;    ///< Ticks seguidos con la carga arriba de CARGA_CONTACTO.
static float cierre_contacto = 0;   ///< Cierre en el que tocó el objeto, de 0 a 1000.
static float cierre_actual = 0;     ///< Cierre que se le manda ahora a la garra, de 0 a 1000.
static float integral = 0;

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

/**
 * @brief Vuelve el control al principio: garra abierta y buscando de nuevo.
 */
static void reiniciar_control(void) {
    en_contacto = 0;
    ticks_arriba = 0;
    integral = 0;
    cierre_actual = 0;
    carga_pedida = 0;
}

/**
 * @brief Un paso del PID.
 *
 * @param carga Carga medida en este tick, en cuentas.
 * @return Cuánto cerrar más allá del punto de contacto (negativo es aflojar).
 */
static float pid(float carga) {
    float error = carga_pedida - carga;
    float integral_nueva = integral + error * DT;

    // La derivada se saca de la carga medida y no del error, así no pega un
    // salto cuando el maestro cambia de golpe lo que pide
    float derivada = -(carga - carga_anterior) / DT;

    float salida = PID_KP * error + PID_KI * integral_nueva + PID_KD * derivada;

    // La salida puede ser negativa: si al tocar se pasó de carga, la garra
    // afloja un poco por detrás del punto de contacto. Lo que no puede es
    // salirse del recorrido de la garra. Si se pasa, se recorta y la integral
    // no sigue acumulando (si no, después tarda mucho en volver).
    float minimo = -cierre_contacto;
    float maximo = 1000 - cierre_contacto;
    if (salida > maximo) {
        salida = maximo;
    } else if (salida < minimo) {
        salida = minimo;
    } else {
        integral = integral_nueva;
    }
    return salida;
}

void garra_init(void) {
    reiniciar_control();
    servo_set_us(SERVO_GARRA, GARRA_ABIERTA_US);
}

void garra_actualizar(uint16_t cierre) {
    // Sin referencia no se puede medir la carga: la garra va por posición
    if (referencia == 0) {
        mover_garra(cierre_a_pulso(cierre));
        return;
    }

    float carga = garra_carga();

    if (cierre < CIERRE_MINIMO) {
        // Palanca suelta: se abre y la próxima vez vuelve a buscar el objeto
        reiniciar_control();
        mover_garra(GARRA_ABIERTA_US);
    } else {
        // Con la palanca apenas apretada pide la carga de contacto, o sea un
        // agarre suave, y a fondo pide CARGA_MAX
        carga_pedida = CARGA_CONTACTO + (CARGA_MAX - CARGA_CONTACTO) * (cierre - CIERRE_MINIMO) / (1000.0f - CIERRE_MINIMO);

        if (!en_contacto) {
            // Etapa 1: cerrar de a poco hasta tocar algo
            cierre_actual += VELOCIDAD_BUSQUEDA;
            if (cierre_actual > 1000) {
                cierre_actual = 1000;
            }

            if (carga >= CARGA_CONTACTO) {
                ticks_arriba++;
            } else {
                ticks_arriba = 0;
            }
            if (ticks_arriba >= TICKS_CONTACTO) {
                en_contacto = 1;
                cierre_contacto = cierre_actual;
                integral = 0;
            }
        } else {
            // Etapa 2: el PID aprieta desde el punto de contacto
            cierre_actual = cierre_contacto + pid(carga);
        }

        mover_garra(cierre_a_pulso((int32_t)cierre_actual));
    }

    carga_anterior = carga;
}

void garra_tomar_referencia(void) {
    // sensores_corriente() ya es el promedio de los últimos 20 ms, o sea de un
    // período entero del servo
    referencia = sensores_corriente();

    // Con la referencia nueva, el control arranca de cero
    reiniciar_control();
}

uint16_t garra_referencia(void) {
    return referencia;
}

int32_t garra_carga(void) {
    if (referencia == 0) {
        return 0;
    }
    // Al apretar, la lectura baja, por eso se resta la lectura de la referencia
    return (int32_t)referencia - (int32_t)sensores_corriente();
}

int32_t garra_carga_pedida(void) {
    return (int32_t)carga_pedida;
}
