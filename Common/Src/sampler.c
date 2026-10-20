/* 1 s ornekleyici (TIM10) */
#include "sampler.h"
#include "board.h"
#include "bsp.h"
#include "encoder.h"
#include "stm32f4xx_ll_tim.h"

static TIM_HandleTypeDef s_htim;
static volatile uint32_t s_tick;
static volatile uint32_t s_count;

void Sampler_Init(void)
{
    SAMPLE_TIM_CLK_ENABLE();

    s_htim.Instance               = SAMPLE_TIM;
    s_htim.Init.Prescaler         = SAMPLE_TIM_PRESCALER;
    s_htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim.Init.Period            = SAMPLE_TIM_PERIOD;
    s_htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&s_htim) != HAL_OK) {
        Error_Handler();
    }

    s_tick  = 0U;
    s_count = Encoder_GetRawCount();

    /* Init sirasinda set olan bayrak temizlenmezse ilk kesme hemen gelir */
    LL_TIM_ClearFlag_UPDATE(SAMPLE_TIM);

    HAL_NVIC_SetPriority(SAMPLE_TIM_IRQn, IRQ_PRIO_SAMPLE_TIM, 0U);
    HAL_NVIC_EnableIRQ(SAMPLE_TIM_IRQn);

    if (HAL_TIM_Base_Start_IT(&s_htim) != HAL_OK) {
        Error_Handler();
    }
}

/* tick ve count'u tutarli okur: arada kesme gelirse tekrar okur */
void Sampler_Read(SamplerSnapshot *out)
{
    uint32_t tick;

    do {
        tick       = s_tick;
        out->count = s_count;
    } while (tick != s_tick);

    out->tick = tick;
}

void Sampler_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(SAMPLE_TIM)) {
        LL_TIM_ClearFlag_UPDATE(SAMPLE_TIM);
        s_count = Encoder_GetRawCount();
        s_tick  = s_tick + 1U;
    }
}
