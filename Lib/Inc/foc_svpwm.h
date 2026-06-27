/**
 * @file    foc_svpwm.h
 * @brief   SVPWM 空间矢量脉宽调制算法
 * @version 0.1.0
 * @date    2026-06-27
 */

#ifndef __FOC_SVPWM_H
#define __FOC_SVPWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "foc_types.h"

/* SVPWM 配置结构体 */
typedef struct {
    uint32_t period;            /* PWM 周期 (ARR 值) */
    f32_t   v_dc;               /* 母线电压 (V) */
    f32_t   max_modulation;     /* 最大调制比 (默认 1.0, 过调制可到 2/sqrt(3)) */
} svpwm_config_t;

/* SVPWM 实例 */
typedef struct {
    svpwm_config_t  config;
    svpwm_pwm_t     output;     /* PWM 输出值 */
    svpwm_sector_t  sector;     /* 当前扇区 */
    f32_t           v_alpha;    /* alpha 轴电压 */
    f32_t           v_beta;     /* beta 轴电压 */
} svpwm_instance_t;

/**
 * @brief   初始化 SVPWM 实例
 * @param   svpwm   SVPWM 实例指针
 * @param   config  配置参数
 */
void svpwm_init(svpwm_instance_t *svpwm, const svpwm_config_t *config);

/**
 * @brief   设置母线电压
 * @param   svpwm   SVPWM 实例指针
 * @param   v_dc    母线电压 (V)
 */
void svpwm_set_vdc(svpwm_instance_t *svpwm, f32_t v_dc);

/**
 * @brief   SVPWM 计算 (主函数)
 * @param   svpwm       SVPWM 实例指针
 * @param   v_alpha     alpha 轴电压
 * @param   v_beta      beta 轴电压
 * @return  PWM 输出值
 */
svpwm_pwm_t svpwm_update(svpwm_instance_t *svpwm, f32_t v_alpha, f32_t v_beta);

/**
 * @brief   获取当前扇区
 * @param   svpwm   SVPWM 实例指针
 * @return  扇区编号 (1-6)
 */
svpwm_sector_t svpwm_get_sector(const svpwm_instance_t *svpwm);

/**
 * @brief   判断扇区 (内部函数)
 * @param   v_alpha alpha 轴电压
 * @param   v_beta  beta 轴电压
 * @return  扇区编号 (1-6)
 */
svpwm_sector_t svpwm_calc_sector(f32_t v_alpha, f32_t v_beta);

/**
 * @brief   计算占空比 (内部函数)
 * @param   svpwm   SVPWM 实例指针
 * @param   sector  扇区编号
 * @param   v_alpha alpha 轴电压
 * @param   v_beta  beta 轴电压
 * @return  PWM 输出值
 */
svpwm_pwm_t svpwm_calc_duty(svpwm_instance_t *svpwm, svpwm_sector_t sector,
                            f32_t v_alpha, f32_t v_beta);

#ifdef __cplusplus
}
#endif

#endif /* __FOC_SVPWM_H */
