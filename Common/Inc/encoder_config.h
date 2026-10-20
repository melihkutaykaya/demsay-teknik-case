/* Encoder ve olcum parametreleri */
#ifndef ENCODER_CONFIG_H
#define ENCODER_CONFIG_H

#define ENCODER_PPR                  100U   /* tur basina pulse */
#define ENCODER_COUNTS_PER_PULSE     4U     /* x4 sayim */
#define ENCODER_COUNTS_PER_REV       (ENCODER_PPR * ENCODER_COUNTS_PER_PULSE)
#define ENCODER_PERIOD_MS            1000U  /* olcum penceresi */

/* |delta| bu degerden kucuk/esitse motor durmus kabul edilir (titresim icin) */
#define ENCODER_STOP_DEADBAND_COUNTS 1U

#endif
