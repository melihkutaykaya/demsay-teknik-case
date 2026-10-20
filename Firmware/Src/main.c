/* Encoder pulse, yon ve RPM olcumu. Sonuclar her saniye UART'tan gonderilir. */
#include "bsp.h"
#include "encoder.h"
#include "encoder_calc.h"
#include "encoder_config.h"
#include "report.h"
#include "sampler.h"
#include "uart_tx.h"

int main(void)
{
    SamplerSnapshot snap;
    EncCalc         calc;
    EncMeasurement  meas;
    char            line[REPORT_LINE_MAX];

    BSP_Init();
    UartTx_Init();
    Encoder_Init();
    Sampler_Init();

    UartTx_Printf("\r\nDemsay encoder case | PPR=%u x%u=%u counts/rev | period=%u ms\r\n",
                  ENCODER_PPR, ENCODER_COUNTS_PER_PULSE, ENCODER_COUNTS_PER_REV,
                  ENCODER_PERIOD_MS);

    Sampler_Read(&snap);
    EncCalc_Init(&calc, ENCODER_COUNTS_PER_REV, ENCODER_PERIOD_MS,
                 ENCODER_STOP_DEADBAND_COUNTS, snap.count, snap.tick);

    for (;;) {
        Sampler_Read(&snap);
        if (EncCalc_Update(&calc, snap.count, snap.tick, &meas)) {
            size_t len = Report_Format(line, sizeof(line),
                                       snap.tick * ENCODER_PERIOD_MS / 1000U, &meas);
            (void)UartTx_Write(line, len);
            BSP_LedToggle();
        }
        __WFI();   /* bir sonraki kesmeye kadar uyu */
    }
}
