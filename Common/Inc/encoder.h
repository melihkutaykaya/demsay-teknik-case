/* TIM2 encoder modu (x4: A ve B'nin tum kenarlari sayilir) */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "board.h"
#include "stm32f4xx_ll_tim.h"

void Encoder_Init(void);

/* 32-bit ham sayac. ISR icinden de cagrilir. */
static inline uint32_t Encoder_GetRawCount(void)
{
    return LL_TIM_GetCounter(ENC_TIM);
}

#endif
