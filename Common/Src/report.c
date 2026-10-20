/* UART satir formati */
#include "report.h"
#include <stdio.h>

size_t Report_Format(char *buf, size_t size, uint32_t t_s, const EncMeasurement *m)
{
    int len;

    if (size == 0U) {
        return 0U;
    }

    len = snprintf(buf, size, "T=%lu PULSE=%lld DELTA=%+ld DIR=%s RPM=%lu.%02lu\r\n",
                   (unsigned long)t_s,
                   (long long)m->position,
                   (long)m->delta,
                   EncCalc_DirName(m->dir),
                   (unsigned long)(m->rpm_x100 / 100U),
                   (unsigned long)(m->rpm_x100 % 100U));

    if (len < 0) {
        return 0U;
    }
    return ((size_t)len < size) ? (size_t)len : size - 1U;
}
