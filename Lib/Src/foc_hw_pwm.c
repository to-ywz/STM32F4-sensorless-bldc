/**
 * @file    foc_hw_pwm.c
 * @brief   PWM 硬件接口层实现
 * @version 0.2.0
 * @date    2026-06-29
 *
 * @note    死区时间计算统一使用 timer ticks，不再使用 ns 中间量。
 *          DTG 编码遵循 STM32F4 RM0090 四段编码规则。
 */

#include "foc_hw_pwm.h"
#include "main.h"
#include <math.h>

/* SD 引脚定义，使用 CubeMX 生成的宏 */
#define SD_GPIO_PORT    DRIVER_SD_GPIO_Port
#define SD_GPIO_PIN     DRIVER_SD_Pin

/**
 * @brief   us 转换为 timer ticks (向上取整)
 * @param   time_us      时间 (us)
 * @param   timer_clk_hz 定时器时钟频率 (Hz)
 * @return  对应的 timer ticks
 */
static uint32_t us_to_timer_ticks(float time_us, uint32_t timer_clk_hz)
{
    if (time_us <= 0.0f || timer_clk_hz == 0U) {
        return 0U;
    }

    float ticks_f = time_us * ((float)timer_clk_hz / 1000000.0f);
    return (uint32_t)ceilf(ticks_f);
}

/**
 * @brief   从 timer ticks 计算 DTG 寄存器值 (STM32F4 四段编码)
 *
 * RM0090 DTG[7:0] 编码:
 *   DTG[7:0] < 128:   DT = DTG[7:0] * Tdtg,           Tdtg = Tdts
 *   128 <= DTG[7:6]==10: DT = (64 + DTG[5:0]) * Tdtg, Tdtg = 2*Tdts
 *   128 <= DTG[7:6]==11 (DTG[7:5]==110): DT = (32 + DTG[4:0]) * Tdtg, Tdtg = 8*Tdts
 *   128 <= DTG[7:5]==111: DT = (32 + DTG[4:0]) * Tdtg, Tdtg = 16*Tdts
 *
 * @param   ticks   死区时间 (timer ticks)
 * @return  DTG 寄存器值 (0~255)
 */
static uint32_t calc_deadtime_reg_from_ticks(uint32_t ticks)
{
    uint32_t n;

    /* 段1: DTG[7:0] < 128, 步长 = 1 tick, 最大 127 ticks */
    if (ticks <= 127U) {
        return ticks;
    }

    /* 段2: DTG[7:6] = 10, 步长 = 2 ticks, 范围 128~254 ticks */
    if (ticks <= 254U) {
        n = (ticks + 1U) / 2U;          /* 向上取整 */
        if (n < 64U) {
            n = 64U;                    /* 段2 最小值: 64*2 = 128 ticks */
        }
        return 0x80U | (n - 64U);
    }

    /* 段3: DTG[7:5] = 110, 步长 = 8 ticks, 范围 256~504 ticks */
    if (ticks <= 504U) {
        n = (ticks + 7U) / 8U;          /* 向上取整 */
        if (n < 32U) {
            n = 32U;                    /* 段3 最小值: 32*8 = 256 ticks */
        }
        return 0xC0U | (n - 32U);
    }

    /* 段4: DTG[7:5] = 111, 步长 = 16 ticks, 范围 512~992 ticks */
    n = (ticks + 15U) / 16U;            /* 向上取整 */
    if (n < 32U) {
        n = 32U;                        /* 段4 最小值: 32*16 = 512 ticks */
    } else if (n > 63U) {
        n = 63U;                        /* 段4 最大值: 63*16 = 1008 ticks */
    }
    return 0xE0U | (n - 32U);
}

/**
 * @brief   初始化 PWM 硬件
 */
int hw_pwm_init(hw_pwm_instance_t *pwm, const hw_pwm_config_t *config)
{
    uint32_t deadtime_ticks;
    uint32_t dtg;
    uint32_t center_ticks;
    TIM_HandleTypeDef *htim;

    if (pwm == NULL || config == NULL || config->htim == NULL) {
        return -1;
    }

    if (config->timer_clk_hz == 0U || config->period_ticks == 0U) {
        return -1;
    }

    pwm->config = *config;
    pwm->enabled = 0;

    htim = pwm->config.htim;

    /* 设置 ARR */
    __HAL_TIM_SET_AUTORELOAD(htim, pwm->config.period_ticks);

    /* CH1~CH3 初始占空比 50% */
    center_ticks = (pwm->config.period_ticks + 1U) / 2U;
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, center_ticks);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, center_ticks);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, center_ticks);

    /* CH4 用于触发 ADC 注入转换 */
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4, pwm->config.adc_trigger_ticks);

    /* 计算并设置死区时间 */
    deadtime_ticks = us_to_timer_ticks(pwm->config.dead_time_us,
                                       pwm->config.timer_clk_hz);
    dtg = calc_deadtime_reg_from_ticks(deadtime_ticks);

    /* 只修改 BDTR 的 DTG 字段，不影响 Break、MOE 等配置 */
    MODIFY_REG(htim->Instance->BDTR, TIM_BDTR_DTG, dtg & TIM_BDTR_DTG);  

    /* 产生更新事件，加载 ARR 和 CCR 到影子寄存器 */
    htim->Instance->EGR = TIM_EGR_UG;

    /* 清除 UG 可能产生的更新标志 */
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);

    /* SD 初始化保持低电平，关闭驱动器 */
    HAL_GPIO_WritePin(SD_GPIO_PORT, SD_GPIO_PIN, GPIO_PIN_RESET);

    return 0;
}

/**
 * @brief   设置 PWM 占空比
 */
void hw_pwm_set_duty(hw_pwm_instance_t *pwm, const pwm_output_t *output)
{
    if (pwm == NULL || output == NULL) {
        return;
    }

    /* 保存 PWM 值 */
    pwm->pwm = *output;

    /* 设置 CCR */
    __HAL_TIM_SET_COMPARE(pwm->config.htim, TIM_CHANNEL_1, output->cmp_a);
    __HAL_TIM_SET_COMPARE(pwm->config.htim, TIM_CHANNEL_2, output->cmp_b);
    __HAL_TIM_SET_COMPARE(pwm->config.htim, TIM_CHANNEL_3, output->cmp_c);
}

/**
 * @brief   使能 PWM 输出
 */
void hw_pwm_enable(hw_pwm_instance_t *pwm)
{
    if (pwm == NULL || pwm->enabled) {
        return;
    }

    /* 使能 TIM1 输出 */
    HAL_TIM_PWM_Start(pwm->config.htim, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(pwm->config.htim, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(pwm->config.htim, TIM_CHANNEL_3);

    /* 使能互补输出 */
    HAL_TIMEx_PWMN_Start(pwm->config.htim, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(pwm->config.htim, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(pwm->config.htim, TIM_CHANNEL_3);

    /* 使能 CH4，用于触发 ADC 注入转换 */
    HAL_TIM_PWM_Start(pwm->config.htim, TIM_CHANNEL_4);

    /* 使能驱动器 (SD = 高) */
    HAL_GPIO_WritePin(SD_GPIO_PORT, SD_GPIO_PIN, GPIO_PIN_SET);

    pwm->enabled = 1;
}

/**
 * @brief   禁用 PWM 输出
 */
void hw_pwm_disable(hw_pwm_instance_t *pwm)
{
    if (pwm == NULL || !pwm->enabled) {
        return;
    }

    /* 禁用驱动器 (SD = 低) */
    HAL_GPIO_WritePin(SD_GPIO_PORT, SD_GPIO_PIN, GPIO_PIN_RESET);

    /* 禁用 TIM1 输出 */
    HAL_TIM_PWM_Stop(pwm->config.htim, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(pwm->config.htim, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(pwm->config.htim, TIM_CHANNEL_3);

    HAL_TIMEx_PWMN_Stop(pwm->config.htim, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(pwm->config.htim, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(pwm->config.htim, TIM_CHANNEL_3);

    /* 禁用 CH4，停止触发 ADC */
    HAL_TIM_PWM_Stop(pwm->config.htim, TIM_CHANNEL_4);

    pwm->enabled = 0;
}

/**
 * @brief   设置 SD 引脚状态
 */
void hw_pwm_set_sd(uint8_t enable)
{
    HAL_GPIO_WritePin(SD_GPIO_PORT, SD_GPIO_PIN,
                      enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief   获取 PWM 状态
 */
uint8_t hw_pwm_is_enabled(const hw_pwm_instance_t *pwm)
{
    return (pwm != NULL) ? pwm->enabled : 0;
}

/**
 * @brief   设置死区时间 (可在运行时调用)
 */
void hw_pwm_set_deadtime(hw_pwm_instance_t *pwm, float dead_time_us)
{
    uint32_t deadtime_ticks;
    uint32_t dtg;

    if (pwm == NULL) {
        return;
    }

    pwm->config.dead_time_us = dead_time_us;

    deadtime_ticks = us_to_timer_ticks(dead_time_us,
                                       pwm->config.timer_clk_hz);
    dtg = calc_deadtime_reg_from_ticks(deadtime_ticks);

    pwm->config.htim->Instance->BDTR &= ~TIM_BDTR_DTG;
    pwm->config.htim->Instance->BDTR |= dtg;
}
