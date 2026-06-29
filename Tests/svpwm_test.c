/**
 * @file svpwm_test.c
 * @brief SVPWM PC 端单元测试
 * @date 2026-06-29
 *
 * 编译: gcc -std=c99 -Wall -I Lib\Inc -o Tests\svpwm_test.exe Tests\svpwm_test.c Lib\Src\svpwm.c -lm
 * 返回 0 = 全部通过, 非 0 = 失败数
 * CSV 数据输出到 stderr，便于重定向: svpwm_test.exe 2> sweep.csv
 */

#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include "../Lib/Inc/svpwm.h"

static int g_pass = 0;
static int g_fail = 0;

#define LOG(...)  fprintf(stderr, __VA_ARGS__)

#define FAIL_MSG(fmt, ...) do { \
    LOG("  FAIL [%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    g_fail++; \
    return; \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { FAIL_MSG("%s == %s: got %ld, expected %ld", #a, #b, (long)(a), (long)(b)); } \
} while(0)

#define ASSERT_U32_RANGE(val, lo, hi) do { \
    uint32_t _v = (val); \
    if (_v < (lo) || _v > (hi)) { FAIL_MSG("%s = %lu, expected [%lu, %lu]", #val, (unsigned long)_v, (unsigned long)(lo), (unsigned long)(hi)); } \
} while(0)

#ifndef M_PI
#define PI 3.14159265358979323846f
#else
#define PI M_PI
#endif

#define ARR     5249U
#define V_DC    12.0f
#define FREQ    16000U

/* ---------- 测试用例 ---------- */

/* 测试1: 零矢量 -> 50% 占空比 */
static void test_zero_vector(void)
{
    LOG("[TEST] zero vector\n");
    svpwm_output_t s;
    svpwm_init(&s, V_DC, FREQ, ARR);

    svpwm_update(&s, 0.0f, 0.0f);

    if (s.fault) { FAIL_MSG("unexpected fault"); }
    uint32_t half = ARR / 2;
    if (s.pwm.cmp_a < half - 1 || s.pwm.cmp_a > half + 1) { FAIL_MSG("cmp_a=%lu", (unsigned long)s.pwm.cmp_a); }
    if (s.pwm.cmp_b < half - 1 || s.pwm.cmp_b > half + 1) { FAIL_MSG("cmp_b=%lu", (unsigned long)s.pwm.cmp_b); }
    if (s.pwm.cmp_c < half - 1 || s.pwm.cmp_c > half + 1) { FAIL_MSG("cmp_c=%lu", (unsigned long)s.pwm.cmp_c); }
    LOG("  PASS (cmp_a=%lu cmp_b=%lu cmp_c=%lu)\n", (unsigned long)s.pwm.cmp_a, (unsigned long)s.pwm.cmp_b, (unsigned long)s.pwm.cmp_c);
    g_pass++;
}

/* 测试2: 六个典型方向 */
static void test_six_directions(void)
{
    LOG("[TEST] six directions\n");
    float angles[] = {0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f};
    float v_mag = 3.0f;

    for (int i = 0; i < 6; i++) {
        float theta = angles[i] * PI / 180.0f;
        float va = v_mag * cosf(theta);
        float vb = v_mag * sinf(theta);

        svpwm_output_t s;
        svpwm_init(&s, V_DC, FREQ, ARR);
        svpwm_update(&s, va, vb);

        if (s.fault) { FAIL_MSG("fault at %.0f deg", angles[i]); }
        ASSERT_U32_RANGE(s.pwm.cmp_a, 0U, ARR);
        ASSERT_U32_RANGE(s.pwm.cmp_b, 0U, ARR);
        ASSERT_U32_RANGE(s.pwm.cmp_c, 0U, ARR);
        LOG("  %3.0f deg: cmp=(%lu, %lu, %lu)\n", angles[i],
            (unsigned long)s.pwm.cmp_a, (unsigned long)s.pwm.cmp_b, (unsigned long)s.pwm.cmp_c);
    }
    LOG("  PASS\n");
    g_pass++;
}

/* 测试3: 扇区边界连续性 — 在每个 60° 边界前后取微小角度差，检查 CCR 跳变 */
static void test_sector_boundary(void)
{
    LOG("[TEST] sector boundary continuity\n");
    float v_mag = 2.0f;

    /* 在每个边界 deg 和 deg+0.01° 处比较，跳变应很小 */
    for (int deg = 0; deg < 360; deg += 60) {
        float theta1 = (float)deg * PI / 180.0f;
        float theta2 = ((float)deg + 0.01f) * PI / 180.0f;

        svpwm_output_t s1, s2;
        svpwm_init(&s1, V_DC, FREQ, ARR);
        svpwm_update(&s1, v_mag * cosf(theta1), v_mag * sinf(theta1));
        svpwm_init(&s2, V_DC, FREQ, ARR);
        svpwm_update(&s2, v_mag * cosf(theta2), v_mag * sinf(theta2));

        if (s1.fault || s2.fault) { FAIL_MSG("fault at %d deg", deg); }

        int32_t da = (int32_t)s2.pwm.cmp_a - (int32_t)s1.pwm.cmp_a;
        int32_t db = (int32_t)s2.pwm.cmp_b - (int32_t)s1.pwm.cmp_b;
        int32_t dc = (int32_t)s2.pwm.cmp_c - (int32_t)s1.pwm.cmp_c;
        /* 0.01° 对应的 CCR 变化应远小于 ARR/10 */
        int32_t limit = (int32_t)ARR / 10;
        if (da > limit || da < -limit) { FAIL_MSG("jump at %d deg: da=%ld", deg, (long)da); }
        if (db > limit || db < -limit) { FAIL_MSG("jump at %d deg: db=%ld", deg, (long)db); }
        if (dc > limit || dc < -limit) { FAIL_MSG("jump at %d deg: dc=%ld", deg, (long)dc); }
    }
    LOG("  PASS\n");
    g_pass++;
}

/* 测试4: 幅值扫描 */
static void test_amplitude_sweep(void)
{
    LOG("[TEST] amplitude sweep (0 to max, 1000 points)\n");
    float theta = PI / 6.0f;

    for (int i = 0; i <= 1000; i++) {
        float v_mag = (float)i / 1000.0f * (V_DC / 1.7320508f);
        float va = v_mag * cosf(theta);
        float vb = v_mag * sinf(theta);

        svpwm_output_t s;
        svpwm_init(&s, V_DC, FREQ, ARR);
        svpwm_update(&s, va, vb);

        if (s.fault) { FAIL_MSG("fault at i=%d, v_mag=%.3f", i, v_mag); }
        ASSERT_U32_RANGE(s.pwm.cmp_a, 0U, ARR);
        ASSERT_U32_RANGE(s.pwm.cmp_b, 0U, ARR);
        ASSERT_U32_RANGE(s.pwm.cmp_c, 0U, ARR);
        if (!isfinite(s.v_alpha) || !isfinite(s.v_beta)) { FAIL_MSG("NaN/Inf at i=%d", i); }
    }
    LOG("  PASS\n");
    g_pass++;
}

/* 测试5: 全角度扫描，输出 CSV */
static void test_full_angle_sweep(void)
{
    LOG("[TEST] full angle sweep (0~360, 3600 points)\n");

    /* CSV header */
    fprintf(stdout, "angle,v_alpha,v_beta,cmp_a,cmp_b,cmp_c,duty_a,duty_b,duty_c\n");

    float v_mag = 3.0f;
    uint32_t min_a = ARR, max_a = 0;
    int pass = 1;

    for (int i = 0; i < 3600; i++) {
        float angle_deg = (float)i / 10.0f;
        float theta = angle_deg * PI / 180.0f;
        float va = v_mag * cosf(theta);
        float vb = v_mag * sinf(theta);

        svpwm_output_t s;
        svpwm_init(&s, V_DC, FREQ, ARR);
        svpwm_update(&s, va, vb);

        if (s.fault) { pass = 0; }
        if (s.pwm.cmp_a > ARR || s.pwm.cmp_b > ARR || s.pwm.cmp_c > ARR) { pass = 0; }

        if (s.pwm.cmp_a < min_a) min_a = s.pwm.cmp_a;
        if (s.pwm.cmp_a > max_a) max_a = s.pwm.cmp_a;

        fprintf(stdout, "%.1f,%.4f,%.4f,%lu,%lu,%lu,%.4f,%.4f,%.4f\n",
                angle_deg, s.v_alpha, s.v_beta,
                (unsigned long)s.pwm.cmp_a, (unsigned long)s.pwm.cmp_b, (unsigned long)s.pwm.cmp_c,
                (float)s.pwm.cmp_a / ARR, (float)s.pwm.cmp_b / ARR, (float)s.pwm.cmp_c / ARR);
    }

    if (!pass) { FAIL_MSG("fault or out-of-range detected"); }
    LOG("  PASS (cmp_a range: %lu ~ %lu)\n", (unsigned long)min_a, (unsigned long)max_a);
    g_pass++;
}

/* 测试6: 输入异常 -> 安全零矢量 */
static void test_fault_inputs(void)
{
    LOG("[TEST] fault inputs\n");
    svpwm_output_t s;
    uint32_t half = ARR / 2;

    /* v_dc <= 0 */
    svpwm_init(&s, 0.0f, FREQ, ARR);
    svpwm_update(&s, 1.0f, 0.0f);
    if (!s.fault) { FAIL_MSG("v_dc=0 should fault"); }
    if (s.pwm.cmp_a != half || s.pwm.cmp_b != half || s.pwm.cmp_c != half) { FAIL_MSG("not safe zero vector"); }

    /* NaN */
    svpwm_init(&s, V_DC, FREQ, ARR);
    svpwm_update(&s, NAN, 0.0f);
    if (!s.fault) { FAIL_MSG("NaN should fault"); }

    /* Inf */
    svpwm_init(&s, V_DC, FREQ, ARR);
    svpwm_update(&s, 1.0f, 1.0f / 0.0f);
    if (!s.fault) { FAIL_MSG("Inf should fault"); }

    /* NULL (不崩溃即可) */
    svpwm_update(NULL, 1.0f, 0.0f);

    LOG("  PASS\n");
    g_pass++;
}

/* 测试7: 过调制保护 */
static void test_overmodulation(void)
{
    LOG("[TEST] overmodulation\n");
    svpwm_output_t s;
    svpwm_init(&s, V_DC, FREQ, ARR);

    float v_big = V_DC * 10.0f;
    svpwm_update(&s, v_big, v_big);

    if (s.fault) { FAIL_MSG("unexpected fault"); }
    ASSERT_U32_RANGE(s.pwm.cmp_a, 0U, ARR);
    ASSERT_U32_RANGE(s.pwm.cmp_b, 0U, ARR);
    ASSERT_U32_RANGE(s.pwm.cmp_c, 0U, ARR);
    LOG("  PASS (cmp=(%lu,%lu,%lu))\n",
        (unsigned long)s.pwm.cmp_a, (unsigned long)s.pwm.cmp_b, (unsigned long)s.pwm.cmp_c);
    g_pass++;
}

/* ---------- 入口 ---------- */

int main(void)
{
    LOG("=== SVPWM Unit Tests ===\n\n");

    test_zero_vector();
    test_six_directions();
    test_sector_boundary();
    test_amplitude_sweep();
    test_full_angle_sweep();
    test_fault_inputs();
    test_overmodulation();

    LOG("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail;
}
