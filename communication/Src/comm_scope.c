/**
 * @file    comm_scope.c
 * @brief   调试示波输出模块。
 * @version 0.1.0
 * @date    2026-07-06
 */

#include "comm_scope.h"
#include <stddef.h>

int comm_scope_init(comm_scope_t *scope, const comm_scope_config_t *config)
{
    if (scope == NULL || config == NULL || config->vofa == NULL ||
        config->normal_period_ms == 0U) {
        return -1;
    }

    if (config->mode != VOFA_MODE_NORMAL &&
        config->mode != VOFA_MODE_SCOPE) {
        return -2;
    }

    scope->vofa             = config->vofa;
    scope->mode             = config->mode;
    scope->normal_period_ms = config->normal_period_ms;
    scope->last_tick_ms     = 0U;

    return 0;
}

uint8_t comm_scope_get_mode(const comm_scope_t *scope)
{
    if (scope == NULL) {
        return 0U;
    }

    return scope->mode;
}

int comm_scope_poll_normal(comm_scope_t *scope,
                           uint32_t now_ms,
                           const float *values,
                           uint8_t count)
{
    if (scope == NULL || values == NULL || count == 0U ||
        count > VOFA_NORMAL_MAX_CHANNELS || scope->vofa == NULL) {
        return -1;
    }

    if (scope->mode != VOFA_MODE_NORMAL) {
        return 0;
    }

    if ((uint32_t)(now_ms - scope->last_tick_ms) <
        scope->normal_period_ms) {
        return 0;
    }
    scope->last_tick_ms = now_ms;

    return comm_vofa_send_justfloat(scope->vofa, values, count);
}

int comm_scope_send_scope(comm_scope_t *scope,
                          const float *values,
                          uint8_t count)
{
    if (scope == NULL || values == NULL || count == 0U ||
        count > VOFA_SCOPE_MAX_CHANNELS || scope->vofa == NULL) {
        return -1;
    }

    if (scope->mode != VOFA_MODE_SCOPE) {
        return 0;
    }

    return comm_vofa_send_justfloat(scope->vofa, values, count);
}
