/**
 * @file    foc_svpwm.c
 * @brief   SVPWM 空间矢量脉宽调制算法实现
 * @version 0.1.0
 * @date    2026-06-27
 */

#include "foc_svpwm.h"
#include <math.h>

/* 常量定义 */
#define SQRT3       1.732050808f
#define ONE_BY_SQRT3 (1.0f / SQRT3)
#define TWO_BY_SQRT3 (2.0f / SQRT3)
#define PI          3.141592654f
#define TWO_PI      (2.0f * PI)

/* 默认配置 */
#define SVPWM_DEFAULT_MAX_MODULATION    1.0f

/**
 * @brief   初始化 SVPWM 实例
 */
void svpwm_init(svpwm_instance_t *svpwm, const svpwm_config_t *config)
{
    if (svpwm == NULL || config == NULL) {
        return;
    }

    svpwm->config = *config;

    /* 设置默认最大调制比 */
    if (svpwm->config.max_modulation <= 0.0f) {
        svpwm->config.max_modulation = SVPWM_DEFAULT_MAX_MODULATION;
    }

    /* 初始化输出为零 */
    svpwm->output.cmp_a = 0;
    svpwm->output.cmp_b = 0;
    svpwm->output.cmp_c = 0;

    svpwm->sector = SECTOR_1;
    svpwm->v_alpha = 0.0f;
    svpwm->v_beta = 0.0f;
}

/**
 * @brief   设置母线电压
 */
void svpwm_set_vdc(svpwm_instance_t *svpwm, f32_t v_dc)
{
    if (svpwm != NULL && v_dc > 0.0f) {
        svpwm->config.v_dc = v_dc;
    }
}

/**
 * @brief   判断扇区
 * @details 根据 alpha-beta 电压判断所在扇区 (1-6)
 */
svpwm_sector_t svpwm_calc_sector(f32_t v_alpha, f32_t v_beta)
{
    f32_t u1 = v_beta;
    f32_t u2 = SQRT3 * 0.5f * v_alpha - 0.5f * v_beta;
    f32_t u3 = -SQRT3 * 0.5f * v_alpha - 0.5f * v_beta;

    uint8_t a = (u1 > 0.0f) ? 1 : 0;
    uint8_t b = (u2 > 0.0f) ? 1 : 0;
    uint8_t c = (u3 > 0.0f) ? 1 : 0;

    uint8_t sector = 4 * c + 2 * b + a;

    /* 映射到扇区 1-6 */
    switch (sector) {
        case 3: return SECTOR_1;
        case 1: return SECTOR_2;
        case 5: return SECTOR_3;
        case 4: return SECTOR_4;
        case 6: return SECTOR_5;
        case 2: return SECTOR_6;
        default: return SECTOR_1;  /* 零矢量，默认扇区 1 */
    }
}

/**
 * @brief   计算占空比
 * @details 根据扇区和 alpha-beta 电压计算三相 PWM 占空比
 */
svpwm_pwm_t svpwm_calc_duty(svpwm_instance_t *svpwm, svpwm_sector_t sector,
                            f32_t v_alpha, f32_t v_beta)
{
    svpwm_pwm_t pwm = {0, 0, 0};

    if (svpwm->config.v_dc <= 0.0f) {
        return pwm;
    }

    f32_t period = (f32_t)svpwm->config.period;
    f32_t v_dc = svpwm->config.v_dc;
    f32_t max_mod = svpwm->config.max_modulation;

    /* 计算电压矢量幅值 (标幺值) */
    f32_t v_mag = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    f32_t v_mag_pu = v_mag / (v_dc * ONE_BY_SQRT3);

    /* 限幅 */
    if (v_mag_pu > max_mod) {
        f32_t scale = max_mod / v_mag_pu;
        v_alpha *= scale;
        v_beta *= scale;
    }

    /* 计算各矢量作用时间 */
    f32_t t1, t2, t0;

    switch (sector) {
        case SECTOR_1:
            t1 = SQRT3 * period / v_dc * ( ONE_BY_SQRT3 * v_beta);
            t2 = SQRT3 * period / v_dc * ( 0.5f * v_alpha + ONE_BY_SQRT3 * 0.5f * v_beta);
            break;
        case SECTOR_2:
            t1 = SQRT3 * period / v_dc * ( 0.5f * v_alpha + ONE_BY_SQRT3 * 0.5f * v_beta);
            t2 = SQRT3 * period / v_dc * (-0.5f * v_alpha + ONE_BY_SQRT3 * 0.5f * v_beta);
            break;
        case SECTOR_3:
            t1 = SQRT3 * period / v_dc * (-0.5f * v_alpha + ONE_BY_SQRT3 * 0.5f * v_beta);
            t2 = SQRT3 * period / v_dc * ( ONE_BY_SQRT3 * v_beta);
            break;
        case SECTOR_4:
            t1 = SQRT3 * period / v_dc * (-ONE_BY_SQRT3 * v_beta);
            t2 = SQRT3 * period / v_dc * (-0.5f * v_alpha - ONE_BY_SQRT3 * 0.5f * v_beta);
            break;
        case SECTOR_5:
            t1 = SQRT3 * period / v_dc * (-0.5f * v_alpha - ONE_BY_SQRT3 * 0.5f * v_beta);
            t2 = SQRT3 * period / v_dc * ( 0.5f * v_alpha - ONE_BY_SQRT3 * 0.5f * v_beta);
            break;
        case SECTOR_6:
            t1 = SQRT3 * period / v_dc * ( 0.5f * v_alpha - ONE_BY_SQRT3 * 0.5f * v_beta);
            t2 = SQRT3 * period / v_dc * (-ONE_BY_SQRT3 * v_beta);
            break;
        default:
            t1 = 0.0f;
            t2 = 0.0f;
            break;
    }

    /* 零矢量作用时间 */
    t0 = period - t1 - t2;

    /* 确保非负 */
    if (t0 < 0.0f) {
        f32_t scale = period / (t1 + t2);
        t1 *= scale;
        t2 *= scale;
        t0 = 0.0f;
    }

    /* 七段式 SVPWM (中心对齐) */
    f32_t ta, tb, tc;

    switch (sector) {
        case SECTOR_1:
            ta = t0 * 0.25f;
            tb = ta + t1 * 0.5f;
            tc = tb + t2 * 0.5f;
            break;
        case SECTOR_2:
            ta = t0 * 0.25f + t2 * 0.5f;
            tb = t0 * 0.25f;
            tc = ta + t1 * 0.5f;
            break;
        case SECTOR_3:
            ta = t0 * 0.25f + t1 * 0.5f + t2 * 0.5f;
            tb = t0 * 0.25f;
            tc = tb + t2 * 0.5f;
            break;
        case SECTOR_4:
            ta = t0 * 0.25f + t1 * 0.5f + t2 * 0.5f;
            tb = t0 * 0.25f + t2 * 0.5f;
            tc = t0 * 0.25f;
            break;
        case SECTOR_5:
            ta = t0 * 0.25f + t1 * 0.5f;
            tb = t0 * 0.25f + t1 * 0.5f + t2 * 0.5f;
            tc = t0 * 0.25f;
            break;
        case SECTOR_6:
            ta = t0 * 0.25f;
            tb = t0 * 0.25f + t1 * 0.5f + t2 * 0.5f;
            tc = tb + t2 * 0.5f;
            break;
        default:
            ta = tb = tc = period * 0.5f;
            break;
    }

    /* 转换为整数比较值，确保在有效范围内 */
    int32_t cmp_a = (int32_t)(ta + 0.5f);
    int32_t cmp_b = (int32_t)(tb + 0.5f);
    int32_t cmp_c = (int32_t)(tc + 0.5f);

    /* 限幅 */
    if (cmp_a < 0) cmp_a = 0;
    if (cmp_a > (int32_t)period) cmp_a = (int32_t)period;
    if (cmp_b < 0) cmp_b = 0;
    if (cmp_b > (int32_t)period) cmp_b = (int32_t)period;
    if (cmp_c < 0) cmp_c = 0;
    if (cmp_c > (int32_t)period) cmp_c = (int32_t)period;

    pwm.cmp_a = (uint32_t)cmp_a;
    pwm.cmp_b = (uint32_t)cmp_b;
    pwm.cmp_c = (uint32_t)cmp_c;

    return pwm;
}

/**
 * @brief   SVPWM 计算 (主函数)
 */
svpwm_pwm_t svpwm_update(svpwm_instance_t *svpwm, f32_t v_alpha, f32_t v_beta)
{
    svpwm_pwm_t pwm = {0, 0, 0};

    if (svpwm == NULL) {
        return pwm;
    }

    /* 保存输入电压 */
    svpwm->v_alpha = v_alpha;
    svpwm->v_beta = v_beta;

    /* 判断扇区 */
    svpwm->sector = svpwm_calc_sector(v_alpha, v_beta);

    /* 计算占空比 */
    svpwm->output = svpwm_calc_duty(svpwm, svpwm->sector, v_alpha, v_beta);

    return svpwm->output;
}

/**
 * @brief   获取当前扇区
 */
svpwm_sector_t svpwm_get_sector(const svpwm_instance_t *svpwm)
{
    if (svpwm != NULL) {
        return svpwm->sector;
    }
    return SECTOR_1;
}
