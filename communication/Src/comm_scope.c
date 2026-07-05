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
        config->low_rate_period_ms == 0U) {
        return -1;
    }

    if (config->mode != SCOPE_MODE_REALTIME_LOW_RATE &&
        config->mode != SCOPE_MODE_HIGH_RATE_3CH) {
        return -2;
    }

    scope->vofa               = config->vofa;
    scope->mode               = config->mode;
    scope->low_rate_period_ms = config->low_rate_period_ms;
    scope->last_tick_ms       = 0U;

    return 0;
}

uint8_t comm_scope_get_mode(const comm_scope_t *scope)
{
    if (scope == NULL) {
        return 0U;
    }

    return scope->mode;
}

int comm_scope_poll_low_rate(comm_scope_t *scope,
                             uint32_t now_ms,
                             const float *values,
                             uint8_t count)
{
    if (scope == NULL || values == NULL || count == 0U ||
        count > SCOPE_MAX_CHANNELS || scope->vofa == NULL) {
        return -1;
    }

    if (scope->mode != SCOPE_MODE_REALTIME_LOW_RATE) {
        return 0;
    }

    if ((uint32_t)(now_ms - scope->last_tick_ms) <
        scope->low_rate_period_ms) {
        return 0;
    }
    scope->last_tick_ms = now_ms;

    return comm_vofa_send_justfloat(scope->vofa, values, count);
}

int comm_scope_send_high_rate_3ch(comm_scope_t *scope, const float *values)
{
    if (scope == NULL || values == NULL || scope->vofa == NULL) {
        return -1;
    }

    if (scope->mode != SCOPE_MODE_HIGH_RATE_3CH) {
        return 0;
    }

    return comm_vofa_send_justfloat(scope->vofa,
                                    values,
                                    SCOPE_HIGH_RATE_CHANNELS);
}
