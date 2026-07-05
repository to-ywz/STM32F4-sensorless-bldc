/**
 * @file foc_math.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief FOC 数学函数实现
 * @version 0.1
 * @date 2026-05-26
 *
 * @par 版本记录
 *  - 0.1 初始版本
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "foc_math.h"
#include <math.h>

#define USING_LOG 0
#if USING_LOG
#define LOG_TAG "FOC_MATH"
#define LOG_I(...)                               \
    printf("[DEBUG] " LOG_TAG ": " __VA_ARGS__); \
    printf("\n")
#endif

#ifndef M_PI
#define PI 3.14159265358979323846f
#else
#define PI M_PI
#endif



/**
 * @brief 角度归一化到 [0, 2π)
 *
 * @param angle  : 输入角度 (rad)
 * @return float : 归一化后的角度 (rad)
 */
static float angle_wrap(float angle)
{
#if USING_LOG
    LOG_I("Original angle: %.4f radians", angle);
#endif
    angle = fmodf(angle, 2.0f * PI);
    if (angle < 0.0f)
        angle += 2.0f * PI;
#if USING_LOG
    LOG_I("Wrapped angle: %.4f radians", angle);
#endif
    return angle;
}

/* foc_clarke: 见 foc_math.h */
void foc_clarke(float ia, float ib, float *i_alpha, float *i_beta)
{
    *i_alpha = ia;
    *i_beta = (ia + 2.0f * ib) / sqrtf(3.0f);
}

/* foc_park: 见 foc_math.h */
void foc_park(float i_alpha, float i_beta, float theta_e, float *i_d, float *i_q)
{
    float theta_e_wrap = angle_wrap(theta_e);
    *i_d = i_alpha * cosf(theta_e_wrap) + i_beta * sinf(theta_e_wrap);
    *i_q = -i_alpha * sinf(theta_e_wrap) + i_beta * cosf(theta_e_wrap);
}

/* foc_inv_park: 见 foc_math.h */
void foc_inv_park(float v_d, float v_q, float theta_e, float *v_alpha, float *v_beta)
{
    float theta_e_wrap = angle_wrap(theta_e);
    *v_alpha = v_d * cosf(theta_e_wrap) - v_q * sinf(theta_e_wrap);
    *v_beta = v_d * sinf(theta_e_wrap) + v_q * cosf(theta_e_wrap);
}

void foc_pi_init(pid_t *pid, float kp, float ki, float v_max)
{
    pid_init(pid, kp, ki, 0.0f, -v_max, v_max);
}

float foc_pi_update(pid_t *pid, float ref, float actual, float dt)
{
    return pid_compute(pid, ref, actual, dt);
}

/* foc_vector_limit: 见 foc_math.h */
void foc_vector_limit(float *v_d, float *v_q, float v_max)
{
    float v_mag_sq = (*v_d) * (*v_d) + (*v_q) * (*v_q);
    if (v_mag_sq > v_max * v_max) {
        float v_mag = sqrtf(v_mag_sq);
        float scale = v_max / v_mag;
        *v_d *= scale;
        *v_q *= scale;
    }
}

// EOF ================================================================
/**

cd "f:\Demo\Doc\lab\tests\" ; if ($?) { gcc foc_math_test.c ../firmware/control/foc_math.c -o foc_math_test } ; if ($?) { .\foc_math_test }
 */