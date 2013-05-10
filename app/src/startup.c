/* startup.c */
#include <stdint.h>

/* extern symbols */
extern unsigned int _end_stack;
extern unsigned int _end_text;
extern unsigned int _start_data;
extern unsigned int _end_data;
extern unsigned int _start_bss;
extern unsigned int _end_bss;
extern unsigned int _end;
 
/* extern functions */
extern int main(void);
extern void SystemInit(void);
 
/* Cortex M3 core interrupt handlers */
void Reset_Handler(void);
void NMI_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void HardFault_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void MemManage_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void BusFault_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void UsageFault_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void SVC_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void DebugMon_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void PendSV_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void SysTick_Handler(void) __attribute__ ((weak, alias ("Dummy_Handler")));

/* LPC13xx specific interrupt handlers */
void WDT_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void TIMER0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void TIMER1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void TIMER2_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void TIMER3_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void UART0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void UART1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void UART2_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void UART3_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void PWM1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void I2C0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void I2C1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void I2C2_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void SPI_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void SSP0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void SSP1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void PLL0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void RTC_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void EINT0_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void EINT1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void EINT2_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void EINT3_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void ADC_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void BOD_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void USB_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void CAN_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void DMA_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void I2S_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void ENET_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void RIT_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void MCPWM_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void QEI_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));
void PLL1_IRQHandler(void) __attribute__ ((weak, alias ("Dummy_Handler")));

/* prototypes */
void Dummy_Handler(void);
 
/* interrupt vector table */
void *vector_table[] __attribute__ ((section(".vectors"))) = {
    
    /* ARM Cortex-M3 interrupt vectors */
    &_end_stack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
 
    /* LPC1768 specific interrupt vectors */
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
};

/* startup function */
void Reset_Handler(void) 
{
    unsigned int *src, *dst;
     
    /* Copy data section from flash to RAM */
    src = &_end_text;
    dst = &_start_data;
    while (dst < &_end_data)
       *dst++ = *src++;
 
    /* Clear the bss section */
    dst = &_start_bss;
    while (dst < &_end)
        *dst++ = 0;
     
    SystemInit();
    
    main();
}

/* default interrupt handler */
void Dummy_Handler(void)
{

}

