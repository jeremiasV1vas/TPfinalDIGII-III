/**
 * @file secuencia.c
 * @brief Guarda los pasos en un arreglo en RAM.
 *
 * Cada paso ocupa 8 bytes, así que 1500 pasos son 12 kB. Se pierde al
 * apagar la placa, pero para mostrar el funcionamiento alcanza.
 */

#include "config.h"
#include "secuencia.h"

static paso_t pasos[SECUENCIA_MAX];
static uint16_t cantidad = 0;

void secuencia_borrar(void) {
    cantidad = 0;
}

uint8_t secuencia_agregar(const paso_t* paso) {
    if (cantidad >= SECUENCIA_MAX) {
        return 0;
    }
    pasos[cantidad] = *paso;
    cantidad++;
    return 1;
}

uint16_t secuencia_largo(void) {
    return cantidad;
}

const paso_t* secuencia_paso(uint16_t numero) {
    return &pasos[numero];
}
