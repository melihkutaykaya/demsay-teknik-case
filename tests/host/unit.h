/* Basit birim test makrolari */
#ifndef UNIT_H
#define UNIT_H

#include <stdio.h>
#include <string.h>

extern int g_unit_failures;
extern int g_unit_checks;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_unit_checks++;                                                     \
        if (!(cond)) {                                                       \
            g_unit_failures++;                                               \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
        }                                                                    \
    } while (0)

#define CHECK_EQ_INT(exp, act)                                               \
    do {                                                                     \
        long long e_ = (long long)(exp), a_ = (long long)(act);              \
        g_unit_checks++;                                                     \
        if (e_ != a_) {                                                      \
            g_unit_failures++;                                               \
            printf("  FAIL %s:%d: %s == %s (expected %lld, got %lld)\n",     \
                   __FILE__, __LINE__, #exp, #act, e_, a_);                  \
        }                                                                    \
    } while (0)

#define CHECK_EQ_STR(exp, act)                                               \
    do {                                                                     \
        g_unit_checks++;                                                     \
        if (strcmp((exp), (act)) != 0) {                                     \
            g_unit_failures++;                                               \
            printf("  FAIL %s:%d:\n    expected \"%s\"\n    got      \"%s\"\n", \
                   __FILE__, __LINE__, (exp), (act));                        \
        }                                                                    \
    } while (0)

#define RUN_TEST(fn)                                                         \
    do {                                                                     \
        int before_ = g_unit_failures;                                       \
        fn();                                                                \
        printf("%s %s\n", (g_unit_failures == before_) ? "[ OK ]" : "[FAIL]", #fn); \
    } while (0)

#endif /* UNIT_H */
