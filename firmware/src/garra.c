/**
 * @file garra.c
 * @brief Fuerza de agarre a partir de la corriente y PID de fuerza.
 *
 * El servo de la garra es de posición: no se le puede pedir una fuerza. Pero
 * cuando la garra ya tocó el objeto, cuanto más se le pide que cierre, más
 * corriente toma y más aprieta. Por eso en modo fuerza la garra trabaja en
 * dos etapas:
 * 1. Buscar: se va cerrando a velocidad constante hasta que la fuerza medida
 *    pasa FUERZA_CONTACTO_G, o sea, hasta que tocó el objeto.
 * 2. Apretar: el PID decide cuánto cerrar más allá del punto donde tocó, para
 *    que la fuerza medida sea la que pide el maestro.
 *
 * Si se suelta la palanca, la garra se abre y la próxima vez vuelve a buscar.
 */

#include "config.h"
#include "garra.h"
#include "sensores.h"
#include "servos.h"

/** Tiempo entre dos pasos del PID, en segundos. */
#define DT (TICK_MS / 1000.0f)

/** Con la palanca por debajo de esto (5 %) se toma como suelta y la garra se abre. */
#define CIERRE_MINIMO 50

static uint8_t modo_fuerza = 0;

static float ganancia_p = PID_KP;
static float ganancia_i = PID_KI;
static float ganancia_d = PID_KD;

static float corriente_ma = 0;
static float fuerza_g = 0;
static float fuerza_anterior_g = 0;
static float referencia_g = 0;

/** 1 mientras la garra está tocando el objeto (etapa de apretar). */
static uint8_t en_contacto = 0;
/** Cierre en el que tocó el objeto, de 0 a 1000. */
static float cierre_contacto = 0;
/** Cierre que manda el control ahora, de 0 a 1000. */
static float cierre_actual = 0;
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
 * @brief Un paso del PID.
 * @return Cuánto cerrar más allá del punto de contacto (negativo es aflojar).
 */
static float pid(void) {
    float error = referencia_g - fuerza_g;
    float integral_nueva = integral + error * DT;

    // La derivada se saca de la fuerza medida y no del error, así no pega un
    // salto cuando el maestro cambia de golpe la fuerza pedida
    float derivada = -(fuerza_g - fuerza_anterior_g) / DT;

    float salida = ganancia_p * error + ganancia_i * integral_nueva + ganancia_d * derivada;

    // La salida puede ser negativa: si al tocar se pasó de fuerza, la garra
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
    modo_fuerza = 0;
    garra_abrir();
}

void garra_medir(void) {
    // Lectura del ADC -> tensión a la salida del amplificador -> corriente en el shunt
    float tension = sensores_corriente() * 3.3f / 4095.0f;
    corriente_ma = tension / (GANANCIA_AMPLI * R_SHUNT_OHM) * 1000.0f;

    fuerza_anterior_g = fuerza_g;
    fuerza_g = (corriente_ma - CORRIENTE_REPOSO_MA) * GRAMOS_POR_MA;
    if (fuerza_g < 0) {
        fuerza_g = 0;
    }
}

void garra_actualizar(uint16_t cierre) {
    if (!modo_fuerza) {
        referencia_g = 0;
        mover_garra(cierre_a_pulso(cierre));
        return;
    }

    // Palanca suelta: se abre y la próxima vez vuelve a buscar el objeto
    if (cierre < CIERRE_MINIMO) {
        referencia_g = 0;
        en_contacto = 0;
        integral = 0;
        cierre_actual = 0;
        mover_garra(GARRA_ABIERTA_US);
        return;
    }

    referencia_g = FUERZA_MAX_G * cierre / 1000.0f;

    if (!en_contacto) {
        // Etapa 1: cerrar de a poco hasta tocar algo
        cierre_actual += VELOCIDAD_BUSQUEDA;
        if (cierre_actual > 1000) {
            cierre_actual = 1000;
        }
        if (fuerza_g >= FUERZA_CONTACTO_G) {
            en_contacto = 1;
            cierre_contacto = cierre_actual;
            integral = 0;
        }
    } else {
        // Etapa 2: el PID aprieta desde el punto de contacto
        cierre_actual = cierre_contacto + pid();
    }

    mover_garra(cierre_a_pulso((int32_t)cierre_actual));
}

void garra_abrir(void) {
    en_contacto = 0;
    integral = 0;
    cierre_actual = 0;
    referencia_g = 0;
    servo_set_us(SERVO_GARRA, GARRA_ABIERTA_US);
}

void garra_set_modo_fuerza(uint8_t activo) {
    modo_fuerza = activo;
    en_contacto = 0;
    integral = 0;
    cierre_actual = 0;
}

uint8_t garra_modo_fuerza(void) {
    return modo_fuerza;
}

void garra_set_ganancias(float kp, float ki, float kd) {
    ganancia_p = kp;
    ganancia_i = ki;
    ganancia_d = kd;
    integral = 0;
}

float garra_corriente(void) {
    return corriente_ma;
}

float garra_fuerza(void) {
    return fuerza_g;
}

float garra_referencia(void) {
    return referencia_g;
}
