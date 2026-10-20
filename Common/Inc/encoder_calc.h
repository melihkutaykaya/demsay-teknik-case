/* Pulse, yon ve RPM hesabi (donanimdan bagimsiz, PC'de test edilir) */
#ifndef ENCODER_CALC_H
#define ENCODER_CALC_H

#include <stdint.h>

typedef enum {
    ENC_DIR_STOP = 0,
    ENC_DIR_CW,
    ENC_DIR_CCW
} EncDirection;

typedef struct {
    int64_t      position;    /* toplam sayim */
    int32_t      delta;       /* bu penceredeki sayim */
    uint32_t     elapsed_ms;  /* pencere suresi */
    EncDirection dir;
    uint32_t     rpm_x100;    /* RPM x 100 (12345 = 123.45 RPM) */
} EncMeasurement;

typedef struct {
    uint32_t last_count;
    uint32_t last_tick;
    int64_t  position;
    uint32_t counts_per_rev;
    uint32_t period_ms;
    uint32_t deadband;
} EncCalc;

void EncCalc_Init(EncCalc *c, uint32_t counts_per_rev, uint32_t period_ms,
                  uint32_t deadband, uint32_t raw_count, uint32_t tick);

/* Yeni tick varsa olcumu doldurur ve 1 dondurur, yoksa 0. */
int EncCalc_Update(EncCalc *c, uint32_t raw_count, uint32_t tick, EncMeasurement *out);

uint32_t EncCalc_RpmX100(int32_t delta, uint32_t elapsed_ms, uint32_t counts_per_rev);
int32_t  EncCalc_Delta(uint32_t prev, uint32_t now);
const char *EncCalc_DirName(EncDirection dir);

#endif
