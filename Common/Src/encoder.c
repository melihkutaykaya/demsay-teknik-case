/* TIM2 encoder modu ayari */
#include "encoder.h"
#include "bsp.h"

static TIM_HandleTypeDef s_htim;

void Encoder_Init(void)
{
    GPIO_InitTypeDef        gpio = {0};
    TIM_Encoder_InitTypeDef enc  = {0};

    ENC_GPIO_CLK_ENABLE();
    ENC_TIM_CLK_ENABLE();

    /* Pull-up: open-collector encoder'lar icin */
    gpio.Pin       = ENC_A_PIN | ENC_B_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = ENC_GPIO_AF;
    HAL_GPIO_Init(ENC_GPIO_PORT, &gpio);

    s_htim.Instance               = ENC_TIM;
    s_htim.Init.Prescaler         = 0U;
    s_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim.Init.Period            = 0xFFFFFFFFU;
    s_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    enc.EncoderMode  = TIM_ENCODERMODE_TI12;
    enc.IC1Polarity  = TIM_ICPOLARITY_RISING;
    enc.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    enc.IC1Prescaler = TIM_ICPSC_DIV1;
    enc.IC1Filter    = ENC_INPUT_FILTER;
    enc.IC2Polarity  = TIM_ICPOLARITY_RISING;
    enc.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    enc.IC2Prescaler = TIM_ICPSC_DIV1;
    enc.IC2Filter    = ENC_INPUT_FILTER;

    if (HAL_TIM_Encoder_Init(&s_htim, &enc) != HAL_OK) {
        Error_Handler();
    }
    __HAL_TIM_SET_COUNTER(&s_htim, 0U);

    if (HAL_TIM_Encoder_Start(&s_htim, TIM_CHANNEL_ALL) != HAL_OK) {
        Error_Handler();
    }
}
