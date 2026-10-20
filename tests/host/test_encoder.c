/* encoder_calc.c ve report.c icin PC birim testleri */
#include <stdint.h>
#include "unit.h"
#include "encoder_config.h"
#include "encoder_calc.h"
#include "report.h"

int g_unit_failures;
int g_unit_checks;

static EncCalc        calc;
static EncMeasurement m;

static void setup(uint32_t start_count)
{
    EncCalc_Init(&calc, ENCODER_COUNTS_PER_REV, ENCODER_PERIOD_MS,
                 ENCODER_STOP_DEADBAND_COUNTS, start_count, 0U);
}

/* ---------------------------------------------------------- RPM hesabi */

static void test_rpm_basic_values(void)
{
    /* 1 s'de 400 sayim = 1 tur/s = 60 RPM */
    CHECK_EQ_INT(6000, EncCalc_RpmX100(400, 1000, 400));
    /* 1 sayim/s = 60/400 = 0.15 RPM (1 s pencerenin cozunurlugu) */
    CHECK_EQ_INT(15, EncCalc_RpmX100(1, 1000, 400));
    /* 3000 RPM -> 20000 sayim/s */
    CHECK_EQ_INT(300000, EncCalc_RpmX100(20000, 1000, 400));
    /* buyuklukte isaret onemli degil */
    CHECK_EQ_INT(6000, EncCalc_RpmX100(-400, 1000, 400));
    CHECK_EQ_INT(0, EncCalc_RpmX100(0, 1000, 400));
}

static void test_rpm_rounding_and_window(void)
{
    /* 3 s'de 1 sayim = 0.05 RPM -> 5 */
    CHECK_EQ_INT(5, EncCalc_RpmX100(1, 3000, 400));
    /* 7 s'de 1 sayim = 0.0214.. RPM -> 2'ye yuvarlanir */
    CHECK_EQ_INT(2, EncCalc_RpmX100(1, 7000, 400));
    /* 2 s pencere, 800 sayim -> yine 60 RPM */
    CHECK_EQ_INT(6000, EncCalc_RpmX100(800, 2000, 400));
    /* sifira bolme korumasi */
    CHECK_EQ_INT(0, EncCalc_RpmX100(400, 0, 400));
}

static void test_rpm_no_overflow_at_high_speed(void)
{
    /* 1e6 sayim/s: 1e6 * 6e6 = 6e12, ara hesapta 64-bit gerekir. */
    CHECK_EQ_INT(15000000, EncCalc_RpmX100(1000000, 1000, 400));
    /* 32 bite sigmayan sonuc basa sarmaz, en buyuk degerde sabitlenir. */
    CHECK_EQ_INT(UINT32_MAX, EncCalc_RpmX100(INT32_MAX, 1000, 400));
    CHECK_EQ_INT(UINT32_MAX, EncCalc_RpmX100(INT32_MIN, 1000, 400));
}

/* ------------------------------------------------------ delta / tasma */

static void test_delta_wraparound(void)
{
    CHECK_EQ_INT(10, EncCalc_Delta(100U, 110U));
    CHECK_EQ_INT(-10, EncCalc_Delta(110U, 100U));
    /* ileri yonde 0xFFFFFFFF -> 0 gecisi */
    CHECK_EQ_INT(20, EncCalc_Delta(0xFFFFFFF6U, 0x0000000AU));
    /* geri yonde 0 -> 0xFFFFFFFF gecisi */
    CHECK_EQ_INT(-20, EncCalc_Delta(0x0000000AU, 0xFFFFFFF6U));
}

/* ------------------------------------------------------ guncelleme / yon */

static void test_forward_rotation_is_cw(void)
{
    setup(0U);
    CHECK_EQ_INT(1, EncCalc_Update(&calc, 400U, 1U, &m));
    CHECK_EQ_INT(ENC_DIR_CW, m.dir);
    CHECK_EQ_INT(400, m.delta);
    CHECK_EQ_INT(400, m.position);
    CHECK_EQ_INT(1000, m.elapsed_ms);
    CHECK_EQ_INT(6000, m.rpm_x100);
}

static void test_reverse_rotation_is_ccw(void)
{
    setup(1000U);
    CHECK_EQ_INT(1, EncCalc_Update(&calc, 1000U - 800U, 1U, &m));
    CHECK_EQ_INT(ENC_DIR_CCW, m.dir);
    CHECK_EQ_INT(-800, m.delta);
    CHECK_EQ_INT(-800, m.position);
    CHECK_EQ_INT(12000, m.rpm_x100);   /* 2 tur/s = 120 RPM, pozitif */
}

static void test_position_accumulates_and_reverses(void)
{
    setup(0U);
    EncCalc_Update(&calc, 400U, 1U, &m);           /* +400 */
    EncCalc_Update(&calc, 1200U, 2U, &m);          /* +800 */
    CHECK_EQ_INT(1200, m.position);
    EncCalc_Update(&calc, 1000U, 3U, &m);          /* -200 */
    CHECK_EQ_INT(1000, m.position);
    CHECK_EQ_INT(ENC_DIR_CCW, m.dir);
    CHECK_EQ_INT(3000, m.rpm_x100);                /* 200/400*60 = 30 RPM */
}

static void test_position_across_counter_wrap(void)
{
    setup(0xFFFFFF00U);
    EncCalc_Update(&calc, 0x00000100U, 1U, &m);    /* tasma uzerinden +512 */
    CHECK_EQ_INT(512, m.delta);
    CHECK_EQ_INT(512, m.position);
    CHECK_EQ_INT(ENC_DIR_CW, m.dir);
    CHECK_EQ_INT(7680, m.rpm_x100);                /* 512/400*60 = 76.8 */
}

/* -------------------------------------------------------- durma tespiti */

static void test_stopped_motor_reports_zero(void)
{
    setup(0U);
    EncCalc_Update(&calc, 4000U, 1U, &m);          /* 600 RPM donuyor */
    CHECK_EQ_INT(60000, m.rpm_x100);
    EncCalc_Update(&calc, 4000U, 2U, &m);          /* hareket yok */
    CHECK_EQ_INT(ENC_DIR_STOP, m.dir);
    CHECK_EQ_INT(0, m.rpm_x100);
    CHECK_EQ_INT(0, m.delta);
    CHECK_EQ_INT(4000, m.position);                /* konum korunur */
}

static void test_jitter_within_deadband_is_stop(void)
{
    setup(500U);
    EncCalc_Update(&calc, 501U, 1U, &m);           /* +1: titresim */
    CHECK_EQ_INT(ENC_DIR_STOP, m.dir);
    CHECK_EQ_INT(0, m.rpm_x100);
    EncCalc_Update(&calc, 500U, 2U, &m);           /* -1: titresim */
    CHECK_EQ_INT(ENC_DIR_STOP, m.dir);
    CHECK_EQ_INT(0, m.rpm_x100);
    CHECK_EQ_INT(0, m.position);                   /* ama sayimlar kaybolmaz */
    EncCalc_Update(&calc, 502U, 3U, &m);           /* +2: gercek yavas hareket */
    CHECK_EQ_INT(ENC_DIR_CW, m.dir);
    CHECK_EQ_INT(30, m.rpm_x100);                  /* 0.30 RPM */
}

/* ------------------------------------------------------ tick isleme */

static void test_no_new_tick_produces_nothing(void)
{
    setup(0U);
    m.delta = 12345;
    CHECK_EQ_INT(0, EncCalc_Update(&calc, 999U, 0U, &m));
    CHECK_EQ_INT(12345, m.delta);                  /* degismedi */
}

static void test_missed_ticks_stretch_window(void)
{
    /* Ana dongu gecikti, tick 0 -> 2 gordu: 2 s pencere, RPM ayni. */
    setup(0U);
    EncCalc_Update(&calc, 800U, 2U, &m);
    CHECK_EQ_INT(2000, m.elapsed_ms);
    CHECK_EQ_INT(6000, m.rpm_x100);
}

/* ------------------------------------------------------------- rapor */

static void test_report_format(void)
{
    char buf[REPORT_LINE_MAX];
    EncMeasurement r = { 4800, 400, 1000, ENC_DIR_CW, 6000 };

    Report_Format(buf, sizeof(buf), 12U, &r);
    CHECK_EQ_STR("T=12 PULSE=4800 DELTA=+400 DIR=CW RPM=60.00\r\n", buf);

    r.position = -150; r.delta = -1; r.dir = ENC_DIR_STOP; r.rpm_x100 = 0;
    Report_Format(buf, sizeof(buf), 3U, &r);
    CHECK_EQ_STR("T=3 PULSE=-150 DELTA=-1 DIR=STOP RPM=0.00\r\n", buf);

    r.position = -1200; r.delta = -803; r.dir = ENC_DIR_CCW; r.rpm_x100 = 12045;
    Report_Format(buf, sizeof(buf), 7U, &r);
    CHECK_EQ_STR("T=7 PULSE=-1200 DELTA=-803 DIR=CCW RPM=120.45\r\n", buf);

    r.rpm_x100 = 5;
    Report_Format(buf, sizeof(buf), 7U, &r);
    CHECK(strstr(buf, "RPM=0.05") != NULL);
}

static void test_report_truncates_safely(void)
{
    char buf[10];
    EncMeasurement r = { 4800, 400, 1000, ENC_DIR_CW, 6000 };

    CHECK_EQ_INT(9, Report_Format(buf, sizeof(buf), 12U, &r));
    CHECK_EQ_INT('\0', buf[9]);
    CHECK_EQ_INT(0, Report_Format(buf, 0U, 12U, &r));
}

int main(void)
{
    RUN_TEST(test_rpm_basic_values);
    RUN_TEST(test_rpm_rounding_and_window);
    RUN_TEST(test_rpm_no_overflow_at_high_speed);
    RUN_TEST(test_delta_wraparound);
    RUN_TEST(test_forward_rotation_is_cw);
    RUN_TEST(test_reverse_rotation_is_ccw);
    RUN_TEST(test_position_accumulates_and_reverses);
    RUN_TEST(test_position_across_counter_wrap);
    RUN_TEST(test_stopped_motor_reports_zero);
    RUN_TEST(test_jitter_within_deadband_is_stop);
    RUN_TEST(test_no_new_tick_produces_nothing);
    RUN_TEST(test_missed_ticks_stretch_window);
    RUN_TEST(test_report_format);
    RUN_TEST(test_report_truncates_safely);

    printf("\n%d checks, %d failures\n", g_unit_checks, g_unit_failures);
    return (g_unit_failures == 0) ? 0 : 1;
}
