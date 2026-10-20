/* UART satiri: "T=12 PULSE=4800 DELTA=+400 DIR=CW RPM=60.00" */
#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdint.h>
#include "encoder_calc.h"

#define REPORT_LINE_MAX  96U

size_t Report_Format(char *buf, size_t size, uint32_t t_s, const EncMeasurement *m);

#endif
