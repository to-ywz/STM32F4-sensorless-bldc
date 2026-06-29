/**
 * @file svpwm.h
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief  SVPWM (零序注入式空间矢量脉宽调制)
 * @version 0.2
 * @date 2026-06-29
 *
 * @par 版本记录
 *  - 0.1 初始版本，扇区时间公式
 *  - 0.2 重写为 αβ→三相+零序注入方式，增加输入校验和安全输出
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef SVPWM_H
#define SVPWM_H

#include <stdint.h>

/**
 * @brief PWM 输出
 *
 * 成员:
 *   freq     - PWM 频率 (Hz)
 *   period   - PWM 周期计数值 (定时器 ARR)
 *   cmp_a/b/c - 三相比较值 (定时器 CCR)
 */
typedef struct {
    uint16_t freq;
    uint16_t period;
    uint32_t cmp_a;
    uint32_t cmp_b;
    uint32_t cmp_c;
} pwm_output_t;

/**
 * @brief SVPWM 配置
 *
 * 成员:
 *   v_dc            - 直流母线电压 (V)
 *   period          - PWM 周期 (ARR 值)
 *   freq            - PWM 频率 (Hz)
 *   min_pulse_ticks - 最小脉宽 (ticks)，默认 0 不启用
 *   mod_limit       - 最大调制率 (0.0~1.0)，建议 0.90~0.95
 */
typedef struct {
    float    v_dc;
    uint16_t period;
    uint16_t freq;
    uint16_t min_pulse_ticks;
    float    mod_limit;
} svpwm_config_t;

/**
 * @brief SVPWM 输出
 *
 * 成员:
 *   v_alpha  - α 轴电压
 *   v_beta   - β 轴电压
 *   sector   - 当前扇区 (1~6, 仅用于调试)
 *   pwm      - PWM 输出
 *   fault    - 故障标志: 1=输入异常，输出安全零矢量
 */
typedef struct {
    float v_alpha;
    float v_beta;
    uint8_t sector;
    pwm_output_t pwm;
    uint8_t fault;
    svpwm_config_t cfg;
} svpwm_output_t;

/**
 * @brief 初始化 SVPWM
 *
 * @param svpwm  : SVPWM 实例指针
 * @param v_dc   : 直流母线电压 (V)
 * @param freq   : PWM 频率 (Hz)
 * @param period : PWM 周期计数值 (定时器 ARR)
 */
void svpwm_init(svpwm_output_t *svpwm, float v_dc, uint16_t freq, uint16_t period);

/**
 * @brief 设置 SVPWM 调制限制
 *
 * @param svpwm      : SVPWM 实例指针
 * @param mod_limit  : 最大调制率 (0.0~1.0)，建议 0.90~0.95
 */
void svpwm_set_mod_limit(svpwm_output_t *svpwm, float mod_limit);

/**
 * @brief 根据 αβ 电压计算 SVPWM 占空比
 *
 * 采用 αβ→三相+零序注入方式，适用于 STM32 TIM1 中心对齐 PWM。
 *
 * 输入异常 (NULL、v_dc<=0、NaN/Inf) 时输出安全零矢量 (50% 占空比)，
 * 并设置 fault 标志。
 *
 * @param svpwm   : SVPWM 实例指针
 * @param v_alpha : α 轴电压 (V)
 * @param v_beta  : β 轴电压 (V)
 */
void svpwm_update(svpwm_output_t *svpwm, float v_alpha, float v_beta);

#endif
