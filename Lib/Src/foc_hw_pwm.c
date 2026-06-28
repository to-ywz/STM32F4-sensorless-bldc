/**
 * @file    foc_hw_pwm.c
 * @brief   PWM 硬件接口层实现
 * @version 0.1.0
 * @date    2026-06-27
 */

#include "foc_hw_pwm.h"
#include "main.h"

/* SD 引脚定义，使用 CubeMX 生成的宏 */
#define SD_GPIO_PORT    DRIVER_SD_GPIO_Port
#define SD_GPIO_PIN     DRIVER_SD_Pin

/* 定时器时钟频率 (假设 168MHz, 需要根据实际配置调整) */
#define TIM1_CLK_MHZ    168

/**
 * @brief   计算死区寄存器值
 * @param   dead_time_us    死区时间 (us)
 * @return  DTG 寄存器值
 */
static uint32_t calc_deadtime_reg(float dead_time_us)
{
    /* STM32F4 高级定时器死区计算 */
    /* Tdtg = 1 / (TIM1_CLK / 1) = 1/168 us = 5.95ns (DTG < 128) */
    /* Tdtg = 2 / (TIM1_CLK / 1) = 2/168 us = 11.9ns (128 <= DTG < 192) */
    /* Tdtg = 8 / (TIM1_CLK / 1) = 8/168 us = 47.6ns (DTG >= 192) */

    uint32_t dtg;
    float tdtg_ns;

    if (dead_time_us < 0.0f) {
        dead_time_us = 0.0f;
    }

    /* 转换为 ns */
    float dead_time_ns = dead_time_us * 1000.0f;

    if (dead_time_ns < 128.0f * (1000.0f / TIM1_CLK_MHZ)) {
        /* DTG[6:0] < 128, Tdtg = Tdts */
        tdtg_ns = 1000.0f / TIM1_CLK_MHZ;
        dtg = (uint32_t)(dead_time_ns / tdtg_ns);
    } else if (dead_time_ns < (64.0f + 64.0f) * (2000.0f / TIM1_CLK_MHZ)) {
        /* 128 <= DTG[5:0] < 192, Tdtg = 2 * Tdts */
        tdtg_ns = 2000.0f / TIM1_CLK_MHZ;
        dtg = 128 + (uint32_t)((dead_time_ns - 128.0f * tdtg_ns) / tdtg_ns);
    } else {
        /* DTG[6:0] >= 192, Tdtg = 8 * Tdts */
        tdtg_ns = 8000.0f / TIM1_CLK_MHZ;
        dtg = 192 + (uint32_t)((dead_time_ns - 64.0f * tdtg_ns) / tdtg_ns);
    }

    /* 限制最大值 */
    if (dtg > 255) {
        dtg = 255;
    }

    return dtg;
}

/**
 * @brief   初始化 PWM 硬件
 */
int hw_pwm_init(hw_pwm_instance_t *pwm, const hw_pwm_config_t *config)
{
    if (pwm == NULL || config == NULL || config->htim == NULL) {
        return -1;
    }

    pwm->config = *config;
    pwm->enabled = 0;

    /* 配置定时器 */
    TIM_HandleTypeDef *htim = pwm->config.htim;

    /* 设置 ARR */
    __HAL_TIM_SET_AUTORELOAD(htim, pwm->config.period);

    /* 设置死区 */
    uint32_t dtg = calc_deadtime_reg(pwm->config.dead_time);
    htim->Instance->BDTR &= ~TIM_BDTR_DTG;
    htim->Instance->BDTR |= dtg;

    /* 设置初始占空比为 0 */
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, 0);

    /* 禁用输出 (SD = 低) */
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
 * @brief   设置死区时间
 */
void hw_pwm_set_deadtime(hw_pwm_instance_t *pwm, float dead_time)
{
    if (pwm == NULL) {
        return;
    }

    pwm->config.dead_time = dead_time;

    uint32_t dtg = calc_deadtime_reg(dead_time);
    pwm->config.htim->Instance->BDTR &= ~TIM_BDTR_DTG;
    pwm->config.htim->Instance->BDTR |= dtg;
}
