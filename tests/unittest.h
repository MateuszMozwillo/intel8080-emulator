#ifndef UNITTEST_H
#define UNITTEST_H

#ifndef UT_MAX_TEST_COUNT
#define UT_MAX_TEST_COUNT 1000
#endif

#ifndef UT_EPSILON
#define UT_EPSILON 0.00001
#endif

#ifndef UT_ONLY_SHOW_FAILED
#define UT_ONLY_SHOW_FAILED 0
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

typedef void (*test_func_t)(void);

extern test_func_t unittest_test_registry[UT_MAX_TEST_COUNT];
extern const char* unittest_test_names[UT_MAX_TEST_COUNT];
extern int unittest_test_count;
extern int unittest_failed_tests;
extern int unittest_passed_tests;

#define UT_FORMAT(val) _Generic((val), \
    int: "%d", \
    long: "%ld", \
    float: "%.6f", \
    double: "%.6f", \
    char*: "\"%s\"", \
    const char*: "\"%s\"", \
    default: "%p")

#define TEST(name) \
    void unittest_test_func_##name(void); \
    __attribute__((constructor)) void unittest_register_test_##name(void) { \
    if (unittest_test_count >= UT_MAX_TEST_COUNT) { \
        fprintf(stderr, "\x1b[31m[FATAL]\x1b[0m Maximum test count (%d) exceeded!\n", UT_MAX_TEST_COUNT); \
        exit(1); \
    } \
    unittest_test_registry[unittest_test_count] = unittest_test_func_##name; \
    unittest_test_names[unittest_test_count] = #name; \
    unittest_test_count++; \
    } \
    void unittest_test_func_##name(void)

#define _UT_SAFE_DBL(x) __builtin_choose_expr( \
    __builtin_types_compatible_p(__typeof__(x), char*) || \
    __builtin_types_compatible_p(__typeof__(x), const char*), \
    0.0, x)

#define EXPECT_EQ(expected, actual) do  { \
    __typeof__((expected) + 0) _ut_exp = (expected) + 0; \
    __typeof__((actual) + 0) _ut_act = (actual) + 0; \
    int _ut_pass = 0; \
    \
    _ut_pass = __builtin_choose_expr( \
        __builtin_types_compatible_p(__typeof__(_ut_exp), float) || \
        __builtin_types_compatible_p(__typeof__(_ut_exp), double), \
        fabs((double)_UT_SAFE_DBL(_ut_exp) - (double)_UT_SAFE_DBL(_ut_act)) < UT_EPSILON, \
        __builtin_choose_expr( \
            __builtin_types_compatible_p(__typeof__(_ut_exp), char*) || \
            __builtin_types_compatible_p(__typeof__(_ut_exp), const char*), \
            strcmp((const char*)(uintptr_t)_ut_exp, (const char*)(uintptr_t)_ut_act) == 0, \
            _ut_exp == _ut_act \
        ) \
    ); \
    \
    if(_ut_pass == 1) { \
        if (!UT_ONLY_SHOW_FAILED) { \
            printf("\t\x1b[32m[PASS]\x1b[0m %s : %d\n", __FILE__, __LINE__); \
        } \
        unittest_passed_tests++; \
    } else { \
        printf("\t\x1b[31m[FAIL]\x1b[0m %s : %d\n", __FILE__, __LINE__); \
        printf("\t       expected: "); \
        printf(UT_FORMAT(_ut_exp), _ut_exp); \
        printf(" got "); \
        printf(UT_FORMAT(_ut_act), _ut_act); \
        printf("\n"); \
        unittest_failed_tests++; \
    } \
} while(0)

int run_all_tests(void);

#ifdef UNITTEST_IMPLEMENTATION
test_func_t unittest_test_registry[UT_MAX_TEST_COUNT];
const char* unittest_test_names[UT_MAX_TEST_COUNT];
int unittest_test_count = 0;
int unittest_failed_tests = 0;
int unittest_passed_tests = 0;

int run_all_tests(void) {
    printf("RUNNING %d TESTS\n", unittest_test_count);
    for (int i = 0; i < unittest_test_count; i++) {
        int _ut_failed_start = unittest_failed_tests;
        printf("\nRunning test group: \x1b[36m%s\x1b[0m\n", unittest_test_names[i]);
        unittest_test_registry[i]();
        if (unittest_failed_tests == _ut_failed_start) {
            printf("\x1b[32m[PASSED]\x1b[0m\n");
        } else {
            printf("\x1b[31m[FAILED]\x1b[0m with \x1b[31m%d\x1b[0m tests\n", unittest_failed_tests - _ut_failed_start);
        }
    }
    printf("\nENDED WITH \e[32m%d\e[0m TESTS PASSED AND \e[31m%d\e[0m TESTS FAILED\n", unittest_passed_tests, unittest_failed_tests);
    return unittest_failed_tests;
}
#endif
#endif
