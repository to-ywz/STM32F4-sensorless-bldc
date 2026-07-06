/**
 * @file    comm_scope.h
 * @brief   调试示波输出模块。
 * @version 0.1.0
 * @date    2026-07-06
 */

#ifndef COMM_SCOPE_H
#define COMM_SCOPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_vofa.h"
#include <stdint.h>

#define VOFA_MODE_NORMAL              1U
#define VOFA_MODE_SCOPE               2U

#define VOFA_NORMAL_MAX_CHANNELS      20U
#define VOFA_SCOPE_MAX_CHANNELS       3U

typedef struct {
    comm_vofa_t *vofa;
    uint8_t      mode;
    uint32_t     normal_period_ms;
    uint32_t     last_tick_ms;
} comm_scope_t;

typedef struct {
    comm_vofa_t *vofa;
    uint8_t      mode;
    uint32_t     normal_period_ms;
} comm_scope_config_t;

/**
 * @brief  初始化调试示波输出对象。
 * @param  scope  调试示波对象。
 * @param  config 调试示波配置。
 * @return 0 表示成功，负数表示失败。
 */
int comm_scope_init(comm_scope_t *scope, const comm_scope_config_t *config);

/**
 * @brief  获取当前输出模式。
 * @param  scope 调试示波对象。
 * @return 输出模式。
 */
uint8_t comm_scope_get_mode(const comm_scope_t *scope);

/**
 * @brief  普通监控输出任务。
 * @param  scope   调试示波对象。
 * @param  now_ms  当前系统时间，单位 ms。
 * @param  values  float 通道数组。
 * @param  count   通道数量，最大 VOFA_NORMAL_MAX_CHANNELS。
 * @return 0 表示成功或未到发送周期，负数表示失败。
 */
int comm_scope_poll_normal(comm_scope_t *scope,
                           uint32_t now_ms,
                           const float *values,
                           uint8_t count);

/**
 * @brief  示波输出。
 * @param  scope  调试示波对象。
 * @param  values float 通道数组。
 * @param  count  通道数量，最大 VOFA_SCOPE_MAX_CHANNELS。
 * @return 0 表示成功，负数表示失败。
 */
int comm_scope_send_scope(comm_scope_t *scope,
                          const float *values,
                          uint8_t count);

#ifdef __cplusplus
}
#endif

#endif /* COMM_SCOPE_H */
