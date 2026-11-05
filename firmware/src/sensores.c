#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"

#include "config.h"
#include "sensores.h"

// El ADC convierte los 5 canales todo el tiempo, uno atrás del otro (modo burst).
// A 100 kHz, a cada canal le tocan unas 20000 conversiones por segundo, o sea
// unas 400 cada 20 ms, que es un período de los servos.
#define FRECUENCIA_ADC     100000
#define MUESTRAS_CORRIENTE 400

// Acá el DMA va guardando las lecturas de la corriente. Cuando llega al final
// vuelve a empezar, así siempre están las de los últimos 20 ms.
static volatile uint32_t muestras[MUESTRAS_CORRIENTE];

// La lista del DMA apunta a sí misma: cuando termina de llenar el buffer,
// arranca de nuevo con la misma configuración y no para nunca.
static GPDMA_LLI_T lista;

// Una sola lectura del ADC tiene bastante ruido y el servo tiembla. Por eso los
// potes se leen todo el tiempo y cada 20 ms nos quedamos con el promedio de las
// unas 400 lecturas que se juntaron.
static const uint8_t canales_potes[4] = {POTE_BASE, POTE_HOMBRO, POTE_CODO, POTE_GARRA};
static uint32_t suma[8];
static uint32_t cantidad[8];
static uint16_t promedio[8];

void sensores_init(void) {
    ADC_Init(FRECUENCIA_ADC);

    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_1);
    ADC_PinConfig(ADC_CHANNEL_2);
    ADC_PinConfig(ADC_CHANNEL_4);
    ADC_PinConfig(ADC_CHANNEL_5);

    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_ChannelEnable(ADC_CHANNEL_1);
    ADC_ChannelEnable(ADC_CHANNEL_2);
    ADC_ChannelEnable(ADC_CHANNEL_4);
    ADC_ChannelEnable(ADC_CHANNEL_5);

    // El pedido de DMA sale de la misma señal que la interrupción del ADC.
    // La habilitamos solo para el canal 5, así el DMA lee una vez por cada
    // conversión nueva de la corriente. La interrupción del ADC en el NVIC
    // queda apagada, solo la usamos para disparar el DMA.
    // La interrupción global viene prendida de fábrica y en modo burst el
    // manual pide que esté apagada.
    ADC_IntDisable(ADC_INT_GLOBAL);
    ADC_IntEnable(ADC_INT_CH5);

    lista.srcAddr = (uint32_t)&LPC_ADC->ADDR5;
    lista.dstAddr = (uint32_t)muestras;
    lista.nextLLI = (uint32_t)&lista;
    lista.control = GPDMA_DMACCxControl_TransferSize(MUESTRAS_CORRIENTE)
                  | GPDMA_DMACCxControl_SWidth(GPDMA_WORD)
                  | GPDMA_DMACCxControl_DWidth(GPDMA_WORD)
                  | GPDMA_DMACCxControl_DI;

    GPDMA_Init();

    GPDMA_Channel_CFG_T dma;
    dma.channelNum = GPDMA_CH_0;
    dma.transferSize = MUESTRAS_CORRIENTE;
    dma.type = GPDMA_P2M;
    dma.srcMemAddr = 0; // no se usa, la fuente es el ADC
    dma.dstMemAddr = (uint32_t)muestras;
    dma.srcConn = GPDMA_ADC;
    dma.dstConn = GPDMA_ADC; // no se usa en P2M
    dma.src.width = GPDMA_WORD;
    dma.src.burst = GPDMA_BSIZE_1;
    dma.src.increment = DISABLE;
    dma.dst.width = GPDMA_WORD;
    dma.dst.burst = GPDMA_BSIZE_1;
    dma.dst.increment = ENABLE;
    dma.intTC = DISABLE;
    dma.intErr = DISABLE;
    dma.linkedList = (uint32_t)&lista;
    GPDMA_SetupChannel(&dma);

    // El driver pone como fuente el registro global del ADC (ADGDR), pero
    // queremos el del canal 5: al leerlo se borra su bit de DONE y el ADC deja
    // de pedir DMA hasta la próxima conversión de ese canal.
    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)&LPC_ADC->ADDR5;

    GPDMA_ChannelStart(GPDMA_CH_0);

    ADC_BurstEnable();
}

void sensores_acumular(void) {
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t canal = canales_potes[i];

        // Solo sumamos si el canal terminó una conversión desde la última vez que lo miramos
        if (ADC_ChannelGetStatus(canal, ADC_DATA_DONE) == SET) {
            suma[canal] += ADC_ChannelGetData(canal);
            cantidad[canal]++;
        }
    }
}

void sensores_promediar(void) {
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t canal = canales_potes[i];

        if (cantidad[canal] > 0) {
            promedio[canal] = suma[canal] / cantidad[canal];
        }
        suma[canal] = 0;
        cantidad[canal] = 0;
    }
}

uint16_t sensores_pote(uint8_t canal) {
    return promedio[canal];
}

uint16_t sensores_corriente(void) {
    uint32_t suma = 0;
    for (uint32_t i = 0; i < MUESTRAS_CORRIENTE; i++) {
        suma += ADC_DR_RESULT(muestras[i]);
    }
    return suma / MUESTRAS_CORRIENTE;
}
