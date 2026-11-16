/**
 * @file serie.c
 * @brief UART2 a 115200 baudios. Por ahora solo transmite, esperando a que
 * salga cada línea.
 *
 * Una línea de estado tiene unos 50 caracteres, que a 115200 tardan unos 4 ms.
 * Como se manda una cada 40 ms, esperar no molesta y no hace falta usar
 * interrupciones ni DMA para transmitir.
 */

#include <string.h>

#include "LPC17xx.h"
#include "lpc17xx_uart.h"

#include "config.h"
#include "serie.h"

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
}

void serie_enviar(const char* texto) {
    UART_Send(UART2, (const uint8_t*)texto, strlen(texto), BLOCKING);
}
