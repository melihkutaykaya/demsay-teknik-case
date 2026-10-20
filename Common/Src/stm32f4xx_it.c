/* Kesme fonksiyonlari */
#include "stm32f4xx_hal.h"
#include "bsp.h"
#include "sampler.h"
#include "uart_tx.h"

void NMI_Handler(void)        { for (;;) {} }
void HardFault_Handler(void)  { Error_Handler(); }
void MemManage_Handler(void)  { Error_Handler(); }
void BusFault_Handler(void)   { Error_Handler(); }
void UsageFault_Handler(void) { Error_Handler(); }
void SVC_Handler(void)        {}
void DebugMon_Handler(void)   {}
void PendSV_Handler(void)     {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    Sampler_IRQHandler();
}

void USART2_IRQHandler(void)
{
    UartTx_UartIRQHandler();
}

void DMA1_Stream6_IRQHandler(void)
{
    UartTx_DmaIRQHandler();
}
