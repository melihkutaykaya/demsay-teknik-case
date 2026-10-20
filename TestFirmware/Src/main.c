/* Kart uzerinde test.
 * Baglanti: PA6 (D12) -> PA0 (A0), PA7 (D11) -> PA1 (A1)
 * TIM3 bilinen hizlarda sinyal uretir, ana programin modulleriyle olculur ve
 * her 1 s pencere beklenen degerle karsilastirilir.
 * Sonunda LD2 surekli yaniyorsa hepsi PASS, yanip sonuyorsa FAIL var. */
#include <stdbool.h>
#include <string.h>
#include "bsp.h"
#include "encoder.h"
#include "encoder_calc.h"
#include "encoder_config.h"
#include "quad_sim.h"
#include "report.h"
#include "sampler.h"
#include "uart_tx.h"

#define SETTLE_WINDOWS    1U   /* hiz degisiminden sonra atlanan pencere */
#define MEASURE_WINDOWS   3U   /* senaryo basina kontrol edilen pencere */
#define COUNT_TOLERANCE   1    /* +-1 sayim */
#define PERIOD_TOL_MS     2U

typedef struct {
    const char *name;
    int32_t     rpm;
} Scenario;

static const Scenario k_scenarios[] = {
    { "No signal -> STOP",      0     },
    { "CW 60 RPM",              60    },
    { "CW 600 RPM",             600   },
    { "CW 3000 RPM",            3000  },
    { "CW 6000 RPM",            6000  },
    { "CCW 120 RPM",            -120  },
    { "CCW 1500 RPM",           -1500 },
    { "CW 1 RPM (slow)",        1     },
    { "Motor stops -> RPM = 0", 0     },
};
#define SCENARIO_COUNT  (sizeof(k_scenarios) / sizeof(k_scenarios[0]))

static EncCalc  s_calc;
static uint32_t s_last_ms;

/* Bir sonraki 1 s pencereyi bekle */
static void WaitWindow(EncMeasurement *m, uint32_t *t_s, uint32_t *dt_ms)
{
    SamplerSnapshot snap;
    uint32_t        now_ms;

    for (;;) {
        Sampler_Read(&snap);
        if (EncCalc_Update(&s_calc, snap.count, snap.tick, m)) {
            break;
        }
        __WFI();
    }
    now_ms    = HAL_GetTick();
    *dt_ms    = now_ms - s_last_ms;
    s_last_ms = now_ms;
    *t_s      = snap.tick * ENCODER_PERIOD_MS / 1000U;
}

static uint32_t AbsDiff(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static bool RunScenario(const Scenario *sc, uint32_t index)
{
    EncMeasurement m;
    QuadSimRate    rate;
    uint32_t       t_s;
    uint32_t       dt_ms;
    bool           pass = true;
    char           line[REPORT_LINE_MAX];

    EncDirection exp_dir = (sc->rpm > 0) ? ENC_DIR_CW
                         : (sc->rpm < 0) ? ENC_DIR_CCW : ENC_DIR_STOP;

    UartTx_Printf("\r\n[%lu/%u] %s\r\n", (unsigned long)(index + 1U),
                  (unsigned)SCENARIO_COUNT, sc->name);

    rate = QuadSim_SetRpm(sc->rpm);

    for (uint32_t i = 0U; i < SETTLE_WINDOWS; i++) {
        WaitWindow(&m, &t_s, &dt_ms);
    }

    for (uint32_t i = 0U; i < MEASURE_WINDOWS; i++) {
        bool ok = true;

        WaitWindow(&m, &t_s, &dt_ms);

        /* Beklenen sayim = num/den * sure. Tam sayida kalmak icin
         * iki taraf den*1000 ile carpilarak karsilastirilir. */
        int64_t  scale    = (int64_t)rate.den * 1000;
        int64_t  exp_x    = rate.num * (int64_t)m.elapsed_ms;
        int64_t  err_x    = (int64_t)m.delta * scale - exp_x;
        int64_t  exp_x100 = (exp_x * 100) / scale;
        uint64_t exp_abs  = (uint64_t)((exp_x100 < 0) ? -exp_x100 : exp_x100);
        uint64_t num_abs  = (uint64_t)((rate.num < 0) ? -rate.num : rate.num);
        uint64_t rpm_den  = (uint64_t)rate.den * ENCODER_COUNTS_PER_REV;
        uint32_t exp_rpm_x100 = (uint32_t)((num_abs * 6000U + rpm_den / 2U) / rpm_den);

        if ((err_x > COUNT_TOLERANCE * scale) || (err_x < -COUNT_TOLERANCE * scale)) {
            ok = false;
        }
        if (m.dir != exp_dir) {
            ok = false;
        }
        if ((exp_dir == ENC_DIR_STOP) && (m.rpm_x100 != 0U)) {
            ok = false;
        }
        if (AbsDiff(m.rpm_x100, exp_rpm_x100) >
            EncCalc_RpmX100(COUNT_TOLERANCE + 1, m.elapsed_ms, ENCODER_COUNTS_PER_REV)) {
            ok = false;
        }
        if (AbsDiff(dt_ms, ENCODER_PERIOD_MS) > PERIOD_TOL_MS) {
            ok = false;
        }

        (void)Report_Format(line, sizeof(line), t_s, &m);
        (void)UartTx_Write("  ", 2U);
        (void)UartTx_Write(line, strlen(line));
        UartTx_Printf("    expect: DELTA=%c%lu.%02lu DIR=%s RPM=%lu.%02lu | period=%lu ms -> %s\r\n",
                      (exp_x100 < 0) ? '-' : '+',
                      (unsigned long)(exp_abs / 100U), (unsigned long)(exp_abs % 100U),
                      EncCalc_DirName(exp_dir),
                      (unsigned long)(exp_rpm_x100 / 100U), (unsigned long)(exp_rpm_x100 % 100U),
                      (unsigned long)dt_ms,
                      ok ? "ok" : "FAIL");
        pass = pass && ok;
    }

    UartTx_Printf("  => %s\r\n", pass ? "PASS" : "FAIL");
    return pass;
}

static bool EncPinHigh(uint16_t pin)
{
    return HAL_GPIO_ReadPin(ENC_GPIO_PORT, pin) == GPIO_PIN_SET;
}

/* Uretec cikislarini sirayla 0/1 yap, encoder girislerinin takip ettigini kontrol et */
static bool CheckWiring(void)
{
    static const bool k_levels[4][2] = {
        { false, false }, { true, false }, { false, true }, { true, true }
    };
    bool a_ok = true;
    bool b_ok = true;

    for (uint32_t i = 0U; i < 4U; i++) {
        QuadSim_ForceLevels(k_levels[i][0], k_levels[i][1]);
        HAL_Delay(2U);
        if (EncPinHigh(ENC_A_PIN) != k_levels[i][0]) {
            a_ok = false;
        }
        if (EncPinHigh(ENC_B_PIN) != k_levels[i][1]) {
            b_ok = false;
        }
    }
    QuadSim_ForceLevels(false, false);

    UartTx_Printf("Wiring A  PA6(D12) -> PA0(A0): %s\r\n", a_ok ? "OK" : "NOT CONNECTED / SWAPPED");
    UartTx_Printf("Wiring B  PA7(D11) -> PA1(A1): %s\r\n", b_ok ? "OK" : "NOT CONNECTED / SWAPPED");
    return a_ok && b_ok;
}

int main(void)
{
    SamplerSnapshot snap;
    uint32_t        passed = 0U;

    BSP_Init();
    UartTx_Init();
    Encoder_Init();
    QuadSim_Init();
    Sampler_Init();

    Sampler_Read(&snap);
    EncCalc_Init(&s_calc, ENCODER_COUNTS_PER_REV, ENCODER_PERIOD_MS,
                 ENCODER_STOP_DEADBAND_COUNTS, snap.count, snap.tick);
    s_last_ms = HAL_GetTick();

    UartTx_Printf("\r\n==== Demsay encoder case - on-target self test ====\r\n");
    UartTx_Printf("Wiring: PA6(D12)->PA0(A0), PA7(D11)->PA1(A1)\r\n");
    UartTx_Printf("Encoder: PPR=%u x%u = %u counts/rev, window %u ms\r\n",
                  ENCODER_PPR, ENCODER_COUNTS_PER_PULSE, ENCODER_COUNTS_PER_REV,
                  ENCODER_PERIOD_MS);

    if (!CheckWiring()) {
        UartTx_Printf("\r\nERROR: fix the jumper(s) above and press RESET.\r\n");
        for (;;) {
            BSP_LedToggle();
            HAL_Delay(100U);
        }
    }

    for (uint32_t i = 0U; i < SCENARIO_COUNT; i++) {
        if (RunScenario(&k_scenarios[i], i)) {
            passed++;
        }
    }

    UartTx_Printf("\r\n==== RESULT: %lu/%u scenarios PASS -> %s ====\r\n",
                  (unsigned long)passed, (unsigned)SCENARIO_COUNT,
                  (passed == SCENARIO_COUNT) ? "ALL PASS" : "FAILED");
    if (UartTx_DroppedBytes() != 0U) {
        UartTx_Printf("(warning: %lu UART bytes dropped)\r\n",
                      (unsigned long)UartTx_DroppedBytes());
    }

    for (;;) {
        if (passed == SCENARIO_COUNT) {
            BSP_LedSet(true);
            __WFI();
        } else {
            BSP_LedToggle();
            HAL_Delay(250U);
        }
    }
}
