/* NUCLEO-F401RE pin, timer ve kesme tanimlari */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx_hal.h"

/* Encoder: TIM2 encoder modu, A = PA0 (A0), B = PA1 (A1) */
#define ENC_TIM                 TIM2
#define ENC_TIM_CLK_ENABLE()    __HAL_RCC_TIM2_CLK_ENABLE()
#define ENC_GPIO_PORT           GPIOA
#define ENC_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define ENC_A_PIN               GPIO_PIN_0
#define ENC_B_PIN               GPIO_PIN_1
#define ENC_GPIO_AF             GPIO_AF1_TIM2
#define ENC_INPUT_FILTER        0x0FU   /* ~3 us dijital filtre */

/* 1 s olcum timer'i: 84 MHz / 8400 / 10000 = 1 Hz */
#define SAMPLE_TIM              TIM10
#define SAMPLE_TIM_CLK_ENABLE() __HAL_RCC_TIM10_CLK_ENABLE()
#define SAMPLE_TIM_IRQn         TIM1_UP_TIM10_IRQn
#define SAMPLE_TIM_PRESCALER    (8400U - 1U)
#define SAMPLE_TIM_PERIOD       (10000U - 1U)

/* UART: USART2 (ST-LINK sanal COM portu), TX DMA1 Stream6 Ch4 */
#define UART_INSTANCE           USART2
#define UART_CLK_ENABLE()       __HAL_RCC_USART2_CLK_ENABLE()
#define UART_IRQn               USART2_IRQn
#define UART_BAUDRATE           115200U
#define UART_GPIO_PORT          GPIOA
#define UART_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define UART_TX_PIN             GPIO_PIN_2
#define UART_RX_PIN             GPIO_PIN_3
#define UART_GPIO_AF            GPIO_AF7_USART2
#define UART_DMA_CLK_ENABLE()   __HAL_RCC_DMA1_CLK_ENABLE()
#define UART_TX_DMA_STREAM      DMA1_Stream6
#define UART_TX_DMA_CHANNEL     DMA_CHANNEL_4
#define UART_TX_DMA_IRQn        DMA1_Stream6_IRQn

/* LED LD2 */
#define LED_GPIO_PORT           GPIOA
#define LED_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define LED_PIN                 GPIO_PIN_5

/* Test programi sinyal ureteci: TIM3, A = PA6 (D12), B = PA7 (D11) */
#define SIM_TIM                 TIM3
#define SIM_TIM_CLK_ENABLE()    __HAL_RCC_TIM3_CLK_ENABLE()
#define SIM_TIM_CLK_HZ          84000000UL
#define SIM_GPIO_PORT           GPIOA
#define SIM_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define SIM_A_PIN               GPIO_PIN_6
#define SIM_B_PIN               GPIO_PIN_7
#define SIM_GPIO_AF             GPIO_AF2_TIM3

/* Kesme oncelikleri (0 = en yuksek) */
#define IRQ_PRIO_SAMPLE_TIM     1U
#define IRQ_PRIO_UART           5U

#endif
