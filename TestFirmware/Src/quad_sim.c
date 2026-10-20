/* TIM3 ile sahte encoder sinyali.
 * Iki kanal output-compare toggle modunda, aralarinda yarim timer periyodu
 * (90 derece) fark var. Her toggle bir kenar oldugu icin:
 *   sayim/s = 2 * f_cnt / (ARR+1) */
#include "quad_sim.h"
#include "board.h"
#include "encoder_config.h"
#include "stm32f4xx_ll_tim.h"

/* Periyot 16 bite sigana kadar sirayla denenen sayac frekanslari */
static const uint32_t k_cnt_hz[] = { 1000000U, 100000U, 10000U };

#define MIN_PERIOD_TICKS  4U
#define MAX_PERIOD_TICKS  65536U

void QuadSim_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    SIM_GPIO_CLK_ENABLE();
    SIM_TIM_CLK_ENABLE();

    gpio.Pin       = SIM_A_PIN | SIM_B_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = SIM_GPIO_AF;
    HAL_GPIO_Init(SIM_GPIO_PORT, &gpio);

    LL_TIM_DisableCounter(SIM_TIM);
    LL_TIM_SetCounterMode(SIM_TIM, LL_TIM_COUNTERMODE_UP);
    LL_TIM_OC_SetPolarity(SIM_TIM, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetPolarity(SIM_TIM, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH);
    QuadSim_ForceLevels(false, false);
    LL_TIM_CC_EnableChannel(SIM_TIM, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);
}

void QuadSim_ForceLevels(bool a_high, bool b_high)
{
    LL_TIM_DisableCounter(SIM_TIM);
    LL_TIM_OC_SetMode(SIM_TIM, LL_TIM_CHANNEL_CH1,
                      a_high ? LL_TIM_OCMODE_FORCED_ACTIVE : LL_TIM_OCMODE_FORCED_INACTIVE);
    LL_TIM_OC_SetMode(SIM_TIM, LL_TIM_CHANNEL_CH2,
                      b_high ? LL_TIM_OCMODE_FORCED_ACTIVE : LL_TIM_OCMODE_FORCED_INACTIVE);
}

QuadSimRate QuadSim_SetRpm(int32_t rpm)
{
    QuadSimRate rate   = { 0, 1U };
    uint32_t    mag    = (rpm < 0) ? (uint32_t)(-rpm) : (uint32_t)rpm;
    uint32_t    cnt_hz = 0U;
    uint32_t    period = 0U;
    uint32_t    first;
    uint32_t    second;

    /* Her calisma ayni durumdan (A=0, B=0) baslasin */
    QuadSim_ForceLevels(false, false);

    if (mag == 0U) {
        return rate;
    }

    /* periyot = cnt_hz * 120 / (rpm * sayim/tur) */
    for (uint32_t i = 0U; i < (sizeof(k_cnt_hz) / sizeof(k_cnt_hz[0])); i++) {
        uint64_t den = (uint64_t)mag * ENCODER_COUNTS_PER_REV;
        uint64_t p   = ((uint64_t)k_cnt_hz[i] * 120U + den / 2U) / den;

        if (p <= MAX_PERIOD_TICKS) {
            cnt_hz = k_cnt_hz[i];
            period = (p < MIN_PERIOD_TICKS) ? MIN_PERIOD_TICKS : (uint32_t)p;
            break;
        }
    }
    if (cnt_hz == 0U) {
        cnt_hz = k_cnt_hz[(sizeof(k_cnt_hz) / sizeof(k_cnt_hz[0])) - 1U];
        period = MAX_PERIOD_TICKS;
    }

    LL_TIM_SetPrescaler(SIM_TIM, (SIM_TIM_CLK_HZ / cnt_hz) - 1U);
    LL_TIM_SetAutoReload(SIM_TIM, period - 1U);

    /* Once degisen kanal onde olur: A onde = CW, B onde = CCW.
     * 0 yerine T/4 kullanildi; CCR = 0 eslesmesi ilk turda tetiklenmiyor. */
    first  = period / 4U;
    second = first + period / 2U;
    if (rpm > 0) {
        LL_TIM_OC_SetCompareCH1(SIM_TIM, first);
        LL_TIM_OC_SetCompareCH2(SIM_TIM, second);
    } else {
        LL_TIM_OC_SetCompareCH1(SIM_TIM, second);
        LL_TIM_OC_SetCompareCH2(SIM_TIM, first);
    }

    LL_TIM_GenerateEvent_UPDATE(SIM_TIM);
    LL_TIM_ClearFlag_UPDATE(SIM_TIM);

    LL_TIM_OC_SetMode(SIM_TIM, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_TOGGLE);
    LL_TIM_OC_SetMode(SIM_TIM, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_TOGGLE);
    LL_TIM_EnableCounter(SIM_TIM);

    rate.num = (int64_t)2 * (int64_t)cnt_hz * ((rpm < 0) ? -1 : 1);
    rate.den = period;
    return rate;
}
