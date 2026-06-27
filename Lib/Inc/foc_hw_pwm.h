/**
 * @file    foc_hw_pwm.h
 * @brief   PWM 硬件接口层
 * @version 0.1.0
 * @date    2026-06-27
 *
 * @note    依赖 STM32 HAL 库，需要在 CubeMX 中配置 TIM1
 *          - 中心对齐模式
 *          - 6路互补输出 (CH1/CH1N, CH2/CH2N, CH3/CH3N)
 *          - 死区时间配置
 *          - 更新事件触发 ADC
 */

#ifndef __FOC_HW_PWM_H
#define __FOC_HW_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "foc_types.h"
#include "stm32f4xx_hal.h"

/* PWM 硬件配置 */
typedef struct {
    TIM_HandleTypeDef *htim;        /* 定时器句柄 (TIM1) */
    uint32_t           period;      /* PWM 周期 (ARR 值) */
    f32_t              dead_time;   /* 死区时间 (us) */
    f32_t              v_dc;        /* 母线电压 (V) */
} hw_pwm_config_t;

/* PWM 硬件实例 */
typedef struct {
    hw_pwm_config_t  config;
    svpwm_pwm_t      pwm;          /* 当前 PWM 值 */
    uint8_t          enabled;      /* 输出使能标志 */
} hw_pwm_instance_t;

/**
 * @brief   初始化 PWM 硬件
 * @param   pwm     PWM 实例指针
 * @param   config  配置参数
 * @return  0: 成功, -1: 失败
 */
int hw_pwm_init(hw_pwm_instance_t *pwm, const hw_pwm_config_t *config);

/**
 * @brief   设置 PWM 占空比
 * @param   pwm     PWM 实例指针
 * @param   output  PWM 输出值 (比较值)
 */
void hw_pwm_set_duty(hw_pwm_instance_t *pwm, const svpwm_pwm_t *output);

/**
 * @brief   使能 PWM 输出
 * @param   pwm     PWM 实例指针
 */
void hw_pwm_enable(hw_pwm_instance_t *pwm);

/**
 * @brief   禁用 PWM 输出
 * @param   pwm     PWM 实例指针
 */
void hw_pwm_disable(hw_pwm_instance_t *pwm);

/**
 * @brief   设置 SD 引脚状态 (驱动器使能/禁用)
 * @param   enable  1: 使能, 0: 禁用
 */
void hw_pwm_set_sd(uint8_t enable);

/**
 * @brief   获取 PWM 状态
 * @param   pwm     PWM 实例指针
 * @return  1: 使能, 0: 禁用
 */
uint8_t hw_pwm_is_enabled(const hw_pwm_instance_t *pwm);

/**
 * @brief   设置死区时间
 * @param   pwm         PWM 实例指针
 * @param   dead_time   死区时间 (us)
 */
void hw_pwm_set_deadtime(hw_pwm_instance_t *pwm, f32_t dead_time);

#ifdef __cplusplus
}
#endif

#endif /* __FOC_HW_PWM_H */
