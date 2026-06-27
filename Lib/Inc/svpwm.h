/**
 * @file svpwm.h
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
 * @brief SVPWM 输出
 *
 * 成员:
 *   v_alpha  - α 轴电压
 *   v_beta   - β 轴电压
 *   v_dc     - 直流母线电压
 *   sector   - 当前扇区 (1~6)
 *   pwm      - PWM 输出
 */
typedef struct {
    float v_alpha;
    float v_beta;
    float v_dc;
    uint8_t sector;
    pwm_output_t pwm;
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
 * @brief 根据 αβ 电压计算 SVPWM 占空比
 *
 * 7 段式 SVPWM，中心对齐 PWM。
 * 最大线性调制: |V| = Vdc/√3
 *
 * @param svpwm   : SVPWM 实例指针
 * @param v_alpha : α 轴电压 (V)
 * @param v_beta  : β 轴电压 (V)
 */
void svpwm_update(svpwm_output_t *svpwm, float v_alpha, float v_beta);

#endif