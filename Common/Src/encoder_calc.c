/* Pulse, yon ve RPM hesabi */
#include "encoder_calc.h"

void EncCalc_Init(EncCalc *c, uint32_t counts_per_rev, uint32_t period_ms,
                  uint32_t deadband, uint32_t raw_count, uint32_t tick)
{
    c->last_count     = raw_count;
    c->last_tick      = tick;
    c->position       = 0;
    c->counts_per_rev = counts_per_rev;
    c->period_ms      = period_ms;
    c->deadband       = deadband;
}

/* Isaretsiz cikarma sayesinde sayac tassa da sonuc dogru cikar */
int32_t EncCalc_Delta(uint32_t prev, uint32_t now)
{
    return (int32_t)(now - prev);
}

/* RPM = |delta| / counts_per_rev * 60000 / elapsed_ms, x100 ve yuvarlanmis */
uint32_t EncCalc_RpmX100(int32_t delta, uint32_t elapsed_ms, uint32_t counts_per_rev)
{
    uint64_t mag = (delta < 0) ? (uint64_t)(-(int64_t)delta) : (uint64_t)delta;
    uint64_t num = mag * 60ULL * 1000ULL * 100ULL;
    uint64_t den = (uint64_t)counts_per_rev * (uint64_t)elapsed_ms;
    uint64_t rpm_x100;

    if (den == 0U) {
        return 0U;
    }
    rpm_x100 = (num + den / 2U) / den;
    return (rpm_x100 > UINT32_MAX) ? UINT32_MAX : (uint32_t)rpm_x100;
}

int EncCalc_Update(EncCalc *c, uint32_t raw_count, uint32_t tick, EncMeasurement *out)
{
    uint32_t ticks = tick - c->last_tick;
    int32_t  delta;
    uint32_t mag;

    if (ticks == 0U) {
        return 0;
    }

    delta = EncCalc_Delta(c->last_count, raw_count);
    c->last_count = raw_count;
    c->last_tick  = tick;
    c->position  += delta;

    out->position   = c->position;
    out->delta      = delta;
    out->elapsed_ms = ticks * c->period_ms;   /* tick kacirildiysa pencere uzar */

    mag = (delta < 0) ? (uint32_t)(-(int64_t)delta) : (uint32_t)delta;
    if (mag <= c->deadband) {
        out->dir      = ENC_DIR_STOP;
        out->rpm_x100 = 0U;
    } else {
        out->dir      = (delta > 0) ? ENC_DIR_CW : ENC_DIR_CCW;
        out->rpm_x100 = EncCalc_RpmX100(delta, out->elapsed_ms, c->counts_per_rev);
    }
    return 1;
}

const char *EncCalc_DirName(EncDirection dir)
{
    switch (dir) {
    case ENC_DIR_CW:  return "CW";
    case ENC_DIR_CCW: return "CCW";
    default:          return "STOP";
    }
}
