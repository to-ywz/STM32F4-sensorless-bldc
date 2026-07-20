/**
 * @file    app_debug.h
 * @brief   应用层调试输出与命令绑定。
 * @version 0.1.0
 * @date    2026-07-06
 */

#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_cmd.h"
#include "comm_scope.h"
#include "app_measurement.h"
#include "motor_fault.h"
#include "open_loop_vf.h"
#include "svpwm.h"
#include <stdint.h>

typedef struct {
    open_loop_vf_t  *vf;
    svpwm_output_t  *svpwm;
    comm_cmd_t      *cmd;
    comm_scope_t    *scope;
    app_measurement_t *measurement;
    motor_fault_t   *fault;
} app_debug_t;

typedef struct {
    open_loop_vf_t  *vf;
    svpwm_output_t  *svpwm;
    comm_cmd_t      *cmd;
    comm_scope_t    *scope;
    app_measurement_t *measurement;
    motor_fault_t   *fault;
} app_debug_config_t;

/**
 * @brief  初始化应用层调试对象。
 * @param  debug  应用层调试对象。
 * @param  config 应用层调试配置。
 * @return 0 表示成功，负数表示失败。
 */
int app_debug_init(app_debug_t *debug, const app_debug_config_t *config);

/**
 * @brief 应用层调试主循环任务。
 * @param debug  应用层调试对象。
 * @param now_ms 当前系统时间，单位 ms。
 */
void app_debug_poll(app_debug_t *debug, uint32_t now_ms);

/**
 * @brief PWM 更新后的示波采样点。
 * @param debug 应用层调试对象。
 * @param pwm   PWM 输出对象。
 */
void app_debug_on_pwm_update(app_debug_t *debug, const pwm_output_t *pwm);

/**
 * @brief 串口命令：启动控制。
 * @param user 应用层调试对象。
 */
void app_debug_cmd_start(void *user);

/**
 * @brief 串口命令：停止控制。
 * @param user 应用层调试对象。
 */
void app_debug_cmd_stop(void *user);

/**
 * @brief 串口命令：设置目标频率。
 * @param user    应用层调试对象。
 * @param freq_hz 目标频率，单位 Hz。
 */
void app_debug_cmd_set_freq(void *user, float freq_hz);

#ifdef __cplusplus
}
#endif

#endif /* APP_DEBUG_H */
