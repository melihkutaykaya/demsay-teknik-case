/* 1 s ornekleyici: TIM10 kesmesinde encoder sayacini kaydeder */
#ifndef SAMPLER_H
#define SAMPLER_H

#include <stdint.h>

typedef struct {
    uint32_t tick;   /* gecen 1 s periyot sayisi */
    uint32_t count;  /* o anda kaydedilen encoder sayaci */
} SamplerSnapshot;

void Sampler_Init(void);
void Sampler_Read(SamplerSnapshot *out);
void Sampler_IRQHandler(void);

#endif
