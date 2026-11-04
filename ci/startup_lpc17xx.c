// Arranque mínimo para compilar el firmware fuera de MCUXpresso (lo usa el CI).
// Para grabar la placa se usa el proyecto de MCUXpresso, que trae su propio arranque.

#include <stdint.h>

extern uint32_t _etext, _sdata, _edata, _sbss, _ebss, _estack;

extern void SystemInit(void);
extern int main(void);

void ResetISR(void);

// Cualquier interrupción que no tenga su handler cae acá y se queda trabada,
// así es fácil verla con el debugger.
void Default_Handler(void) {
    while (1) {
    }
}

#define ALIAS(f) __attribute__((weak, alias(#f)))

void NMI_Handler(void) ALIAS(Default_Handler);
void HardFault_Handler(void) ALIAS(Default_Handler);
void MemManage_Handler(void) ALIAS(Default_Handler);
void BusFault_Handler(void) ALIAS(Default_Handler);
void UsageFault_Handler(void) ALIAS(Default_Handler);
void SVC_Handler(void) ALIAS(Default_Handler);
void DebugMon_Handler(void) ALIAS(Default_Handler);
void PendSV_Handler(void) ALIAS(Default_Handler);
void SysTick_Handler(void) ALIAS(Default_Handler);

void WDT_IRQHandler(void) ALIAS(Default_Handler);
void TIMER0_IRQHandler(void) ALIAS(Default_Handler);
void TIMER1_IRQHandler(void) ALIAS(Default_Handler);
void TIMER2_IRQHandler(void) ALIAS(Default_Handler);
void TIMER3_IRQHandler(void) ALIAS(Default_Handler);
void UART0_IRQHandler(void) ALIAS(Default_Handler);
void UART1_IRQHandler(void) ALIAS(Default_Handler);
void UART2_IRQHandler(void) ALIAS(Default_Handler);
void UART3_IRQHandler(void) ALIAS(Default_Handler);
void PWM1_IRQHandler(void) ALIAS(Default_Handler);
void I2C0_IRQHandler(void) ALIAS(Default_Handler);
void I2C1_IRQHandler(void) ALIAS(Default_Handler);
void I2C2_IRQHandler(void) ALIAS(Default_Handler);
void SPI_IRQHandler(void) ALIAS(Default_Handler);
void SSP0_IRQHandler(void) ALIAS(Default_Handler);
void SSP1_IRQHandler(void) ALIAS(Default_Handler);
void PLL0_IRQHandler(void) ALIAS(Default_Handler);
void RTC_IRQHandler(void) ALIAS(Default_Handler);
void EINT0_IRQHandler(void) ALIAS(Default_Handler);
void EINT1_IRQHandler(void) ALIAS(Default_Handler);
void EINT2_IRQHandler(void) ALIAS(Default_Handler);
void EINT3_IRQHandler(void) ALIAS(Default_Handler);
void ADC_IRQHandler(void) ALIAS(Default_Handler);
void BOD_IRQHandler(void) ALIAS(Default_Handler);
void USB_IRQHandler(void) ALIAS(Default_Handler);
void CAN_IRQHandler(void) ALIAS(Default_Handler);
void DMA_IRQHandler(void) ALIAS(Default_Handler);
void I2S_IRQHandler(void) ALIAS(Default_Handler);
void ENET_IRQHandler(void) ALIAS(Default_Handler);
void RIT_IRQHandler(void) ALIAS(Default_Handler);
void MCPWM_IRQHandler(void) ALIAS(Default_Handler);
void QEI_IRQHandler(void) ALIAS(Default_Handler);
void PLL1_IRQHandler(void) ALIAS(Default_Handler);
void USBActivity_IRQHandler(void) ALIAS(Default_Handler);
void CANActivity_IRQHandler(void) ALIAS(Default_Handler);

__attribute__((section(".isr_vector"), used)) void (*const vectores[])(void) = {
    (void (*)(void))&_estack,
    ResetISR,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, // acá va el checksum que pide el bootloader del LPC
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
    WDT_IRQHandler,
    TIMER0_IRQHandler,
    TIMER1_IRQHandler,
    TIMER2_IRQHandler,
    TIMER3_IRQHandler,
    UART0_IRQHandler,
    UART1_IRQHandler,
    UART2_IRQHandler,
    UART3_IRQHandler,
    PWM1_IRQHandler,
    I2C0_IRQHandler,
    I2C1_IRQHandler,
    I2C2_IRQHandler,
    SPI_IRQHandler,
    SSP0_IRQHandler,
    SSP1_IRQHandler,
    PLL0_IRQHandler,
    RTC_IRQHandler,
    EINT0_IRQHandler,
    EINT1_IRQHandler,
    EINT2_IRQHandler,
    EINT3_IRQHandler,
    ADC_IRQHandler,
    BOD_IRQHandler,
    USB_IRQHandler,
    CAN_IRQHandler,
    DMA_IRQHandler,
    I2S_IRQHandler,
    ENET_IRQHandler,
    RIT_IRQHandler,
    MCPWM_IRQHandler,
    QEI_IRQHandler,
    PLL1_IRQHandler,
    USBActivity_IRQHandler,
    CANActivity_IRQHandler,
};

void ResetISR(void) {
    // Copiamos las variables inicializadas de la flash a la RAM
    uint32_t* origen = &_etext;
    uint32_t* destino = &_sdata;
    while (destino < &_edata) {
        *destino++ = *origen++;
    }

    // Y ponemos en cero las que no tienen valor inicial
    destino = &_sbss;
    while (destino < &_ebss) {
        *destino++ = 0;
    }

    SystemInit();
    main();

    while (1) {
    }
}
