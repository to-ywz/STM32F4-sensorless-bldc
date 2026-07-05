/**
 * @file pid_test.c
 * @brief PID 控制器 PC 端单元测试 (无 printf 依赖)
 * @date 2026-06-29
 *
 * 编译 (PC): gcc -std=c99 -Wall -o pid_test.exe pid_test.c ../foc/Src/pid.c -lm
 * 运行: ./pid_test.exe ; echo $?
 * 返回 0 = 全部通过, 非 0 = 失败数
 *
 * 嵌入式: 直接编译进工程，main() 返回值可通过调试器观察。
 *         或将 main() 改为 void pid_test_run(void)，用 g_fail 判断结果。
 */

#include <math.h>
#include <stddef.h>
#include "../foc/Inc/pid.h"

static int g_pass = 0;
static int g_fail = 0;

/* 失败时记录行号，便于调试器定位 */
static int g_last_fail_line = 0;

#define FAIL_RETURN()  do { g_fail++; g_last_fail_line = __LINE__; return; } while(0)

#define ASSERT_FINITE(val) do { \
    if (!isfinite((float)(val))) { FAIL_RETURN(); } \
} while(0)

#define ASSERT_RANGE(val, lo, hi) do { \
    float _v = (float)(val); \
    if (_v < (lo) || _v > (hi)) { FAIL_RETURN(); } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    float _diff = (float)(a) - (float)(b); \
    if (_diff > 1e-5f || _diff < -1e-5f) { FAIL_RETURN(); } \
} while(0)

/* ---------- 测试用例 ---------- */

static void test_normal(void)
{
    pid_t pid;
    pid_init(&pid, 1.0f, 0.5f, 0.1f, -10.0f, 10.0f);
    float dt = 1.0f / 16000.0f;

    float out = pid_compute(&pid, 1.0f, 0.0f, dt);
    ASSERT_FINITE(out);
    ASSERT_RANGE(out, -10.0f, 10.0f);

    for (int i = 0; i < 10000; i++) {
        out = pid_compute(&pid, 1.0f, 0.5f, dt);
        ASSERT_FINITE(out);
        ASSERT_RANGE(out, -10.0f, 10.0f);
    }
    g_pass++;
}

static void test_dt_zero(void)
{
    pid_t pid;
    pid_init(&pid, 1.0f, 1.0f, 1.0f, -10.0f, 10.0f);
    pid.prev_error = 0.5f;

    float out = pid_compute(&pid, 1.0f, 0.0f, 0.0f);
    ASSERT_FINITE(out);
    ASSERT_RANGE(out, -10.0f, 10.0f);
    ASSERT_FLOAT_EQ(pid.integral, 0.0f);
    ASSERT_FLOAT_EQ(pid.prev_error, 1.0f);
    g_pass++;
}

static void test_dt_negative(void)
{
    pid_t pid;
    pid_init(&pid, 1.0f, 1.0f, 1.0f, -10.0f, 10.0f);

    float out = pid_compute(&pid, 1.0f, 0.0f, -0.001f);
    ASSERT_FINITE(out);
    ASSERT_RANGE(out, -10.0f, 10.0f);
    ASSERT_FLOAT_EQ(pid.integral, 0.0f);
    g_pass++;
}

static void test_dt_nan(void)
{
    pid_t pid;
    pid_init(&pid, 1.0f, 1.0f, 1.0f, -10.0f, 10.0f);

    float out = pid_compute(&pid, 1.0f, 0.0f, NAN);
    ASSERT_FINITE(out);
    ASSERT_RANGE(out, -10.0f, 10.0f);
    ASSERT_FLOAT_EQ(pid.integral, 0.0f);
    g_pass++;
}

static void test_kd_zero(void)
{
    pid_t pid;
    pid_init(&pid, 1.0f, 0.5f, 0.0f, -10.0f, 10.0f);
    float dt = 1.0f / 16000.0f;

    float out = pid_compute(&pid, 1.0f, 0.0f, dt);
    ASSERT_FINITE(out);
    ASSERT_RANGE(out, -10.0f, 10.0f);
    g_pass++;
}

static void test_positive_saturation(void)
{
    pid_t pid;
    pid_init(&pid, 100.0f, 50.0f, 0.0f, -5.0f, 5.0f);
    float dt = 1.0f / 16000.0f;

    for (int i = 0; i < 1000; i++) {
        float out = pid_compute(&pid, 10.0f, 0.0f, dt);
        ASSERT_FINITE(out);
        ASSERT_RANGE(out, -5.0f, 5.0f);
    }
    g_pass++;
}

static void test_negative_saturation(void)
{
    pid_t pid;
    pid_init(&pid, 100.0f, 50.0f, 0.0f, -5.0f, 5.0f);
    float dt = 1.0f / 16000.0f;

    for (int i = 0; i < 1000; i++) {
        float out = pid_compute(&pid, -10.0f, 0.0f, dt);
        ASSERT_FINITE(out);
        ASSERT_RANGE(out, -5.0f, 5.0f);
    }
    g_pass++;
}

static void test_null_pid(void)
{
    float out = pid_compute(NULL, 1.0f, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(out, 0.0f);
    g_pass++;
}

/* ---------- 入口 ---------- */

int main(void)
{
    test_normal();
    test_dt_zero();
    test_dt_negative();
    test_dt_nan();
    test_kd_zero();
    test_positive_saturation();
    test_negative_saturation();
    test_null_pid();

    /* 返回失败数: 0 = 全部通过 */
    return g_fail;
}
