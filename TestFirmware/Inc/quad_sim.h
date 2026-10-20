/* TIM3 ile sahte encoder sinyali (A = PA6, B = PA7) */
#ifndef QUAD_SIM_H
#define QUAD_SIM_H

#include <stdbool.h>
#include <stdint.h>

/* Uretilen hiz: saniyedeki sayim = num / den (isaret = yon) */
typedef struct {
    int64_t  num;
    uint32_t den;
} QuadSimRate;

void        QuadSim_Init(void);
QuadSimRate QuadSim_SetRpm(int32_t rpm);   /* negatif = CCW, 0 = dur */
void        QuadSim_ForceLevels(bool a_high, bool b_high);

#endif
