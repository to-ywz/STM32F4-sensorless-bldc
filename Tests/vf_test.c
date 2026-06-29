/**
 * @file vf_test.c
 * @brief V/F 状态机 PC 端单元测试 (无 printf 依赖)
 * @date 2026-06-29
 *
 * 编译 (PC): gcc -std=c99 -Wall -I Lib\Inc -o Tests\vf_test.exe Tests\vf_test.c Lib\Src\open_loop_vf.c -lm
 * 运行: ./Tests\vf_test.exe ; echo $?
 * 返回 0 = 全部通过, 非 0 = 失败数
 */

#include <math.h>
#include <stddef.h>
#include "../Lib/Inc/open_loop_vf.h"

static int g_pass = 0;
static int g_fail = 0;

#define FAIL() do { g_fail++; return; } while(0)

#define ASSERT_FINITE(val) do { \
    if (!isfinite((float)(val))) { FAIL(); } \
} while(0)

#define ASSERT_RANGE(val, lo, hi) do { \
    float _v = (float)(val); \
    if (_v < (lo) - 1e-6f || _v > (hi) + 1e-6f) { FAIL(); } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    float _d = (float)(a) - (float)(b); \
    if (_d > 1e-5f || _d < -1e-5f) { FAIL(); } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { FAIL(); } \
} while(0)

/* ---------- 测试用例 ---------- */

/* 测试1: 初始化后为 STOP，v_out = 0 */
static void test_init_stop(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);

    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_STOP);
    ASSERT_FLOAT_EQ(vf.v_out, 0.0f);
    ASSERT_FLOAT_EQ(vf.freq, 0.0f);
    ASSERT_EQ(vf.enable, 0);
    g_pass++;
}

/* 测试2: 启动后经过 ALIGN → RAMP → RUN */
static void test_state_transitions(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    open_loop_vf_set_align(&vf, 0.5f, 0.3f);

    open_loop_vf_start(&vf);
    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_ALIGN);
    ASSERT_EQ(vf.enable, 1);

    float dt = 1.0f / 16000.0f;

    /* ALIGN 阶段: 0.3s */
    for (int i = 0; i < (int)(0.3f / dt) + 1; i++) {
        open_loop_vf_step(&vf, dt);
        ASSERT_FINITE(vf.v_out);
        ASSERT_FINITE(vf.freq);
        ASSERT_FINITE(vf.theta_e);
    }
    /* 应已进入 RAMP */
    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_RAMP);

    /* RAMP 阶段: 频率单调增加 */
    float prev_freq = 0.0f;
    int ramp_steps = (int)(6.0f / dt);  /* 足够长到达到 target_freq */
    int reached_run = 0;
    for (int i = 0; i < ramp_steps; i++) {
        open_loop_vf_step(&vf, dt);
        ASSERT_FINITE(vf.v_out);
        ASSERT_RANGE(vf.v_out, 0.0f, 6.93f);
        ASSERT_FINITE(vf.freq);
        if (vf.state == OPEN_LOOP_VF_STATE_RUN) {
            reached_run = 1;
            break;
        }
        /* 频率单调 */
        if (vf.freq < prev_freq - 1e-6f) {
            FAIL();
        }
        prev_freq = vf.freq;
    }
    ASSERT_EQ(reached_run, 1);
    ASSERT_FLOAT_EQ(vf.freq, 50.0f);

    /* RUN 阶段: 维持 target_freq */
    for (int i = 0; i < 1000; i++) {
        open_loop_vf_step(&vf, dt);
        ASSERT_FLOAT_EQ(vf.freq, 50.0f);
        ASSERT_RANGE(vf.v_out, 0.0f, 6.93f);
        ASSERT_FINITE(vf.theta_e);
    }
    g_pass++;
}

/* 测试3: ALIGN 持续时间误差不超过一个控制周期 */
static void test_align_duration(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    open_loop_vf_set_align(&vf, 0.5f, 0.2f);

    open_loop_vf_start(&vf);
    float dt = 1.0f / 16000.0f;
    int steps = 0;

    while (vf.state == OPEN_LOOP_VF_STATE_ALIGN) {
        open_loop_vf_step(&vf, dt);
        steps++;
        if (steps > 10000) break;  /* 安全退出 */
    }

    float elapsed = steps * dt;
    float error = elapsed - 0.2f;
    if (error < 0.0f) error = -error;
    /* 误差不超过一个控制周期 */
    if (error > dt * 1.5f) {
        FAIL();
    }
    g_pass++;
}

/* 测试4: 停止后 freq=0, v_out=0 */
static void test_stop(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    open_loop_vf_set_align(&vf, 0.5f, 0.1f);

    open_loop_vf_start(&vf);
    float dt = 1.0f / 16000.0f;

    /* 运行一段时间 */
    for (int i = 0; i < 10000; i++) {
        open_loop_vf_step(&vf, dt);
    }

    open_loop_vf_stop(&vf);
    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_STOP);
    ASSERT_FLOAT_EQ(vf.v_out, 0.0f);
    ASSERT_FLOAT_EQ(vf.freq, 0.0f);
    ASSERT_EQ(vf.enable, 0);

    /* 持续运行保持 STOP */
    for (int i = 0; i < 1000; i++) {
        open_loop_vf_step(&vf, dt);
        ASSERT_FLOAT_EQ(vf.v_out, 0.0f);
        ASSERT_FLOAT_EQ(vf.freq, 0.0f);
    }
    g_pass++;
}

/* 测试5: 无预定位直接进入 RAMP */
static void test_no_align(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    /* 不调用 set_align */

    open_loop_vf_start(&vf);
    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_RAMP);
    g_pass++;
}

/* 测试6: enable=0 时 step 应自动回到 STOP */
static void test_enable_off(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);

    open_loop_vf_start(&vf);
    float dt = 1.0f / 16000.0f;
    for (int i = 0; i < 1000; i++) {
        open_loop_vf_step(&vf, dt);
    }

    /* 直接关闭 enable */
    vf.enable = 0;
    open_loop_vf_step(&vf, dt);

    ASSERT_EQ(vf.state, OPEN_LOOP_VF_STATE_STOP);
    ASSERT_FLOAT_EQ(vf.v_out, 0.0f);
    ASSERT_FLOAT_EQ(vf.freq, 0.0f);
    g_pass++;
}

/* 测试7: v_out 始终在 [0, v_max] */
static void test_voltage_bounds(void)
{
    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    open_loop_vf_set_align(&vf, 2.0f, 0.1f);

    open_loop_vf_start(&vf);
    float dt = 1.0f / 16000.0f;

    for (int i = 0; i < 100000; i++) {
        open_loop_vf_step(&vf, dt);
        ASSERT_FINITE(vf.v_out);
        ASSERT_RANGE(vf.v_out, 0.0f, 6.93f);
        ASSERT_FINITE(vf.freq);
        ASSERT_FINITE(vf.theta_e);
    }
    g_pass++;
}

/* 测试8: get_state 接口 */
static void test_get_state(void)
{
    ASSERT_EQ(open_loop_vf_get_state(NULL), OPEN_LOOP_VF_STATE_STOP);

    open_loop_vf_t vf;
    open_loop_vf_init(&vf, 50.0f, 10.0f, 0.14f, 6.93f, 0.5f);
    ASSERT_EQ(open_loop_vf_get_state(&vf), OPEN_LOOP_VF_STATE_STOP);
    g_pass++;
}

/* ---------- 入口 ---------- */

int main(void)
{
    test_init_stop();
    test_state_transitions();
    test_align_duration();
    test_stop();
    test_no_align();
    test_enable_off();
    test_voltage_bounds();
    test_get_state();

    return g_fail;
}
