/**
 * @file serie.c
 * @brief UART2 a 115200 baudios. Recibe por interrupción y transmite esperando.
 *
 * La interrupción solo guarda los caracteres que llegan en un buffer circular.
 * El main los va sacando y arma las líneas. Así no se pierde nada aunque
 * lleguen varios comandos seguidos mientras el main está ocupado mandando el
 * estado.
 */

#include <string.h>

#include "LPC17xx.h"
#include "lpc17xx_uart.h"

#include "config.h"
#include "serie.h"

/** Tamaño del buffer circular de recepción. Entran unos 10 comandos. */
#define TAMANIO_BUFFER 128

/** Buffer circular: la interrupción escribe en "entrada" y el main lee en "salida". */
static volatile char buffer[TAMANIO_BUFFER];
static volatile uint8_t entrada = 0;
static volatile uint8_t salida = 0;

/** Línea que se está armando en el main. */
static char linea[SERIE_LARGO_LINEA];
static uint8_t largo = 0;

void serie_init(void) {
    UART_PinConfig(UART_TX2_P0_10);
    UART_PinConfig(UART_RX2_P0_11);

    UART_CFG_T uart;
    uart.baudRate = UART_BAUDIOS;
    uart.parity = UART_PARITY_NONE;
    uart.dataBits = UART_DBITS_8;
    uart.stopBits = UART_STOPBIT_1;
    UART_Init(UART2, &uart);

    // UART_Init deja las FIFO apagadas. Las prendemos para que UART_Send pueda
    // cargar 16 bytes de una, y que la recepción interrumpa con cada carácter.
    UART_FIFO_CFG_T fifo;
    fifo.resetRxBuf = ENABLE;
    fifo.resetTxBuf = ENABLE;
    fifo.dmaMode = DISABLE;
    fifo.level = UART_FIFO_TRGLEV0;
    UART_FIFOConfig(UART2, &fifo);

    UART_TxEnable(UART2);

    UART_IntConfig(UART2, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART2_IRQn);
}

void serie_enviar(const char* texto) {
    UART_Send(UART2, (const uint8_t*)texto, strlen(texto), BLOCKING);
}

uint8_t serie_leer_linea(char* destino) {
    while (salida != entrada) {
        char c = buffer[salida];
        salida = (salida + 1) % TAMANIO_BUFFER;

        if (c == '\n' || c == '\r') {
            // Una línea vacía (por ejemplo el \n después de un \r) no cuenta
            if (largo > 0) {
                linea[largo] = '\0';
                strcpy(destino, linea);
                largo = 0;
                return 1;
            }
        } else if (largo < SERIE_LARGO_LINEA - 1) {
            linea[largo] = c;
            largo++;
        }
    }
    return 0;
}

/**
 * @brief Llega un carácter de la PC: se guarda en el buffer circular.
 *
 * Si el buffer está lleno, el carácter se pierde.
 */
void UART2_IRQHandler(void) {
    while (UART_GetLineStatus(UART2) & UART_LINESTAT_RDR) {
        char c = UART_ReceiveByte(UART2);
        uint8_t siguiente = (entrada + 1) % TAMANIO_BUFFER;

        if (siguiente != salida) {
            buffer[entrada] = c;
            entrada = siguiente;
        }
    }
}
