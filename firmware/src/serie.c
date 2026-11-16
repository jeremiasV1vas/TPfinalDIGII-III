/**
 * @file serie.c
 * @brief UART2 a 115200 baudios: transmite esperando a que salga cada línea y
 * recibe por interrupción.
 *
 * Una línea de estado tiene unos 60 caracteres, que a 115200 tardan unos 5 ms.
 * Como se manda una cada 40 ms, esperar no molesta y no hace falta usar
 * interrupciones ni DMA para transmitir.
 *
 * Para recibir sí se usa la interrupción, porque un comando puede llegar en
 * cualquier momento. La interrupción solo junta las letras hasta el fin de
 * línea; el comando lo ejecuta el main en el tick siguiente.
 */

#include <string.h>

#include "LPC17xx.h"
#include "lpc17xx_uart.h"

#include "config.h"
#include "serie.h"

static char comando_recibido[COMANDO_LARGO_MAX + 1]; ///< Letras del comando que está llegando.
static volatile uint8_t largo = 0;                   ///< Cuántas letras llegaron.
static volatile uint8_t comando_listo = 0;           ///< 1 cuando llegó el fin de línea.

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
    // cargar 16 bytes de una en vez de esperar carácter por carácter.
    UART_FIFO_CFG_T fifo;
    fifo.resetRxBuf = ENABLE;
    fifo.resetTxBuf = ENABLE;
    fifo.dmaMode = DISABLE;
    fifo.level = UART_FIFO_TRGLEV0;
    UART_FIFOConfig(UART2, &fifo);

    UART_TxEnable(UART2);

    // Interrupción cada vez que llega una letra
    UART_IntConfig(UART2, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART2_IRQn);
}

void serie_enviar(const char* texto) {
    UART_Send(UART2, (const uint8_t*)texto, strlen(texto), BLOCKING);
}

uint8_t serie_recibir(char* comando) {
    if (!comando_listo) {
        return 0;
    }

    strcpy(comando, comando_recibido);
    largo = 0;
    comando_listo = 0;
    return 1;
}

/**
 * @brief Llegó al menos una letra por la UART2.
 *
 * Con la FIFO prendida puede haber más de una esperando, así que se leen
 * mientras haya (leerlas es lo que limpia la interrupción).
 */
void UART2_IRQHandler(void) {
    while (UART_GetLineStatus(UART2) & UART_LSR_RDR) {
        char letra = UART_ReceiveByte(UART2);

        // Si el main todavía no atendió el comando anterior, se descarta
        if (comando_listo) {
            continue;
        }

        if (letra == '\n' || letra == '\r') {
            // Una línea vacía no es un comando (pasa con el '\n' que viene
            // después del '\r' en algunas terminales)
            if (largo > 0) {
                comando_recibido[largo] = '\0';
                comando_listo = 1;
            }
        } else if (largo < COMANDO_LARGO_MAX) {
            comando_recibido[largo] = letra;
            largo++;
        }
    }
}
