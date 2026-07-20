/**
 * @file svpwm.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief  SVPWM (零序注入式空间矢量脉宽调制)
 * @version 0.2
 * @date 2026-06-29
 *
 * @par 算法说明
 *  1. αβ 电压 → 三相电压 (等幅值 Clarke 逆变换)
 *  2. 计算零序偏移 v_offset = -0.5*(v_max + v_min)
 *  3. 加入零序: v_x += v_offset
 *  4. 转换为占空比: duty_x = 0.5 + v_x / v_dc
 *  5. CCR = duty * ARR，四舍五入
 *
 * @par 中心对齐 PWM 关系
 *  STM32 TIM1 中心对齐模式: 计数器 0→ARR→0
 *  PWM mode 1: counter < CCR 时输出高
 *  占空比 = CCR / ARR
 *  50% 占空比 → CCR = ARR/2 → 线电压平均为零
 *
 * @par 最大调制
 *  线性调制极限: |V| = Vdc/√3
 *  实际使用 mod_limit (默认 0.95) 保留裕量
 *  超限时对 αβ 电压等比缩放，不改变矢量方向
 *
 * @par 版本记录
 *  - 0.1 初始版本，扇区时间公式
 *  - 0.2 重写为零序注入方式
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "svpwm.h"
#include <math.h>

#define SQRT3_OVER_2  0.8660254f   /* √3/2 */

/**
 * @brief 浮点 CCR 转换 (四舍五入 + 限幅)
 */
static uint32_t svpwm_float_to_ccr(float value, uint32_t period)
{
    if (!isfinite(value)) {
        return period / 2U;
    }
    if (value <= 0.0f) {
        return 0U;
    }
    if (value >= (float)period) {
        return period;
    }
    return (uint32_t)(value + 0.5f);
}

/**
 * @brief 输出安全零矢量 (50% 占空比)
 */
static void svpwm_safe_output(svpwm_output_t *svpwm, svpwm_fault_t fault)
{
    uint32_t half = svpwm->pwm.period / 2U;
    svpwm->pwm.cmp_a = half;
    svpwm->pwm.cmp_b = half;
    svpwm->pwm.cmp_c = half;
    svpwm->fault = (uint8_t)fault;
}

void svpwm_init(svpwm_output_t *svpwm, float v_dc, uint16_t freq, uint16_t period)
{
    svpwm->v_alpha  = 0.0f;
    svpwm->v_beta   = 0.0f;
    svpwm->sector_next = 0;
    svpwm->fault    = 0;

    svpwm->cfg.v_dc            = v_dc;
    svpwm->cfg.period          = period;
    svpwm->cfg.freq            = freq;
    svpwm->cfg.min_pulse_ticks = 0;       /* 默认不启用 */
    svpwm->cfg.mod_limit       = 0.95f;   /* 默认 95% 调制率 */

    svpwm->pwm.freq   = freq;
    svpwm->pwm.period = period;
    svpwm->pwm.cmp_a  = period / 2;
    svpwm->pwm.cmp_b  = period / 2;
    svpwm->pwm.cmp_c  = period / 2;
}

void svpwm_set_mod_limit(svpwm_output_t *svpwm, float mod_limit)
{
    if (svpwm == NULL) {
        return;
    }
    if (mod_limit < 0.0f) mod_limit = 0.0f;
    if (mod_limit > 1.0f) mod_limit = 1.0f;
    svpwm->cfg.mod_limit = mod_limit;
}

void svpwm_update(svpwm_output_t *svpwm, float v_alpha, float v_beta)
{
    if (svpwm == NULL) {
        return;
    }

    svpwm->v_alpha = v_alpha;
    svpwm->v_beta  = v_beta;
    svpwm->fault   = 0;

    float v_dc   = svpwm->cfg.v_dc;
    float period = (float)svpwm->cfg.period;

    /* ---- 输入校验 ---- */
    if (!isfinite(v_alpha) || !isfinite(v_beta)) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_INVALID_INPUT);
        return;
    }

    if (!isfinite(v_dc) || v_dc <= 0.0f) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_INVALID_BUS);
        return;
    }

    if (period <= 0.0f || !isfinite(svpwm->cfg.mod_limit) ||
        svpwm->cfg.mod_limit <= 0.0f) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_INVALID_CONFIG);
        return;
    }

    /* ---- 过调制保护: 等比缩放 αβ 电压 ---- */
    float v_mag_sq = v_alpha * v_alpha + v_beta * v_beta;
    float v_max = v_dc * svpwm->cfg.mod_limit / 1.7320508f; /* Vdc * mod_limit / √3 */
    if (!isfinite(v_mag_sq) || !isfinite(v_max)) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_NUMERIC);
        return;
    }
    if (v_max <= 0.0f) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_INVALID_CONFIG);
        return;
    }

    float v_max_sq = v_max * v_max;
    if (v_mag_sq > v_max_sq) {
        float scale = v_max / sqrtf(v_mag_sq);
        v_alpha *= scale;
        v_beta  *= scale;
    }

    /* ---- αβ → 三相电压 (等幅值 Clarke 逆变换) ---- */
    /* v_a = v_alpha
       v_b = -0.5 * v_alpha + (√3/2) * v_beta
       v_c = -0.5 * v_alpha - (√3/2) * v_beta */
    float v_a = v_alpha;
    float v_b = -0.5f * v_alpha + SQRT3_OVER_2 * v_beta;
    float v_c = -0.5f * v_alpha - SQRT3_OVER_2 * v_beta;

    /* ---- 零序注入 (DPWM/SVPWM) ---- */
    /* v_offset = -0.5 * (v_max_phase + v_min_phase)
       效果: 将三相电压居中，最大化线性调制范围 */
    float v_max_phase = v_a;
    float v_min_phase = v_a;
    if (v_b > v_max_phase) v_max_phase = v_b;
    if (v_c > v_max_phase) v_max_phase = v_c;
    if (v_b < v_min_phase) v_min_phase = v_b;
    if (v_c < v_min_phase) v_min_phase = v_c;

    float v_offset = -0.5f * (v_max_phase + v_min_phase);

    v_a += v_offset;
    v_b += v_offset;
    v_c += v_offset;

    /* ---- 转换为 CCR ---- */
    /* duty = 0.5 + v_x / v_dc
       CCR = duty * ARR */
    float duty_a = 0.5f + v_a / v_dc;
    float duty_b = 0.5f + v_b / v_dc;
    float duty_c = 0.5f + v_c / v_dc;

    float ccr_a = duty_a * period;
    float ccr_b = duty_b * period;
    float ccr_c = duty_c * period;

    if (!isfinite(ccr_a) || !isfinite(ccr_b) || !isfinite(ccr_c)) {
        svpwm_safe_output(svpwm, SVPWM_FAULT_NUMERIC);
        return;
    }

    svpwm->pwm.cmp_a = svpwm_float_to_ccr(ccr_a, svpwm->cfg.period);
    svpwm->pwm.cmp_b = svpwm_float_to_ccr(ccr_b, svpwm->cfg.period);
    svpwm->pwm.cmp_c = svpwm_float_to_ccr(ccr_c, svpwm->cfg.period);

    /* ---- 扇区记录 (仅用于调试) ---- */
    /* 通过 αβ 符号判断大致扇区 */
    if (v_beta >= 0.0f) {
        if (v_alpha >= 0.0f) {
            svpwm->sector_next = (v_beta * 0.5773503f > v_alpha) ? 2 : 1; /* √3/3 ≈ 0.577 */
        } else {
            svpwm->sector_next = (v_beta * 0.5773503f > -v_alpha) ? 2 : 3;
        }
    } else {
        if (v_alpha < 0.0f) {
            svpwm->sector_next = (-v_beta * 0.5773503f > -v_alpha) ? 5 : 4;
        } else {
            svpwm->sector_next = (-v_beta * 0.5773503f > v_alpha) ? 5 : 6;
        }
    }
}
