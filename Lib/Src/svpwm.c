/**
 * @file svpwm.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief  SVPWM (7 段式空间矢量脉宽调制)
 * @version 0.1
 * @date 2026-05-31
 *
 * @par 版本记录
 *  - 0.1 初始版本，7 段式 SVPWM
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "svpwm.h"
#include <math.h>

#define SQRT3 1.7320508f

void svpwm_init(svpwm_output_t *svpwm, float v_dc, uint16_t freq, uint16_t period)
{
    svpwm->v_alpha  = 0.0f;
    svpwm->v_beta   = 0.0f;
    svpwm->v_dc     = v_dc;
    svpwm->sector   = 0;

    svpwm->pwm.freq   = freq;
    svpwm->pwm.period = period;
    svpwm->pwm.cmp_a  = period / 2;
    svpwm->pwm.cmp_b  = period / 2;
    svpwm->pwm.cmp_c  = period / 2;
}

/**
 * @brief 扇区判断
 *
 * 利用三个参考量的符号组合查表:
 *   U1 = vβ
 *   U2 = (√3*vα - vβ) / 2
 *   U3 = (-√3*vα - vβ) / 2
 *
 * @return uint8_t 扇区编号 (1~6)
 */
static uint8_t svpwm_get_sector(float v_alpha, float v_beta)
{
    float u1 = v_beta;
    float u2 = (SQRT3 * v_alpha - v_beta) * 0.5f;
    float u3 = (-SQRT3 * v_alpha - v_beta) * 0.5f;

    uint8_t a = (u1 > 0.0f) ? 1 : 0;
    uint8_t b = (u2 > 0.0f) ? 1 : 0;
    uint8_t c = (u3 > 0.0f) ? 1 : 0;

    uint8_t n = (c << 2) | (b << 1) | a;

    static const uint8_t table[8] = {0, 2, 6, 1, 4, 3, 5, 0};
    return table[n];
}

void svpwm_update(svpwm_output_t *svpwm, float v_alpha, float v_beta)
{
    svpwm->v_alpha = v_alpha;
    svpwm->v_beta  = v_beta;

    float v_dc = svpwm->v_dc;
    float Tpwm = (float)svpwm->pwm.period;

    /* 1. 扇区判断 */
    uint8_t sector = svpwm_get_sector(v_alpha, v_beta);
    svpwm->sector = sector;

    /* 2. 计算基本矢量作用时间 */
    float K = SQRT3 * Tpwm / v_dc;
    float a = K * (SQRT3 * v_alpha - v_beta) / SQRT3;  /* case 1/3 */
    float b = K * (SQRT3 * v_alpha + v_beta) / SQRT3;  /* case 2/6 */
    float c = K * v_beta;                               /* case 1/3/4/6 */

    float T1, T2;
    switch (sector) {
        case 1: T1 =  a; T2 =  c; break;
        case 2: T1 =  b; T2 = -a; break;
        case 3: T1 =  c; T2 = -b; break;
        case 4: T1 = -a; T2 = -c; break;
        case 5: T1 = -b; T2 =  a; break;
        case 6: T1 = -c; T2 =  b; break;
        default: T1 = 0; T2 = 0; break;
    }

    /* 过调制限幅: T1 + T2 <= Tpwm */
    float sum = T1 + T2;
    if (sum > Tpwm) {
        T1 = T1 * Tpwm / sum;
        T2 = T2 * Tpwm / sum;
    }
    float T0 = Tpwm - T1 - T2;

    /* 3. 7 段式中心对齐: 计算三段切换点 */
    float t_a = T0 * 0.25f;
    float t_b = t_a + T1 * 0.5f;
    float t_c = t_b + T2 * 0.5f;

    /* 4. 按扇区分配到三相比较值 */
    uint32_t cmp_a, cmp_b, cmp_c;
    switch (sector) {
        case 1: cmp_a = t_a; cmp_b = t_b; cmp_c = t_c; break;
        case 2: cmp_a = t_b; cmp_b = t_a; cmp_c = t_c; break;
        case 3: cmp_a = t_c; cmp_b = t_a; cmp_c = t_b; break;
        case 4: cmp_a = t_c; cmp_b = t_b; cmp_c = t_a; break;
        case 5: cmp_a = t_b; cmp_b = t_c; cmp_c = t_a; break;
        case 6: cmp_a = t_a; cmp_b = t_c; cmp_c = t_b; break;
        default: cmp_a = cmp_b = cmp_c = (uint32_t)(Tpwm * 0.5f); break;
    }

    /* 5. 限幅 [0, period] */
    uint32_t period = svpwm->pwm.period;
    if (cmp_a > period) cmp_a = period;
    if (cmp_b > period) cmp_b = period;
    if (cmp_c > period) cmp_c = period;

    svpwm->pwm.cmp_a = cmp_a;
    svpwm->pwm.cmp_b = cmp_b;
    svpwm->pwm.cmp_c = cmp_c;
}