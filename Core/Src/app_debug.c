/**
 * @file    app_debug.c
 * @brief   应用层调试输出与命令绑定。
 * @version 0.1.0
 * @date    2026-07-06
 */

#include "app_debug.h"
#include <stddef.h>

/**
 * @brief 填充 VOFA 普通监控通道。
 *
 * 这是应用层模板。后续要改普通监控变量时，优先改这里；
 * 或者把外部准备好的数组复制到 values 中。
 */
static uint8_t app_debug_fill_vofa_normal(app_debug_t *debug,
                                          float *values,
                                          uint8_t max_count)
{
    uint8_t i;

    app_measurement_sample_t measurement;

    if (debug == NULL || debug->vf == NULL || debug->svpwm == NULL ||
        debug->cmd == NULL || debug->measurement == NULL || values == NULL ||
        max_count < VOFA_NORMAL_MAX_CHANNELS) {
        return 0U;
    }

    if (app_measurement_get(debug->measurement, &measurement) < 0) {
        return 0U;
    }

    for (i = 0U; i < max_count; i++) {
        values[i] = 0.0f;
    }

    values[0] = measurement.current_u_a;
    values[1] = measurement.current_v_a;
    values[2] = measurement.current_w_a;
    values[3] = measurement.current_u_adc_v;
    values[4] = measurement.current_v_adc_v;
    values[5] = measurement.current_w_adc_v;
    values[6] = measurement.bemf_u_v;
    values[7] = measurement.bemf_v_v;
    values[8] = measurement.bemf_w_v;
    values[9] = measurement.vbus_v;
    values[10] = measurement.bemf_u_adc_v;
    values[11] = measurement.bemf_v_adc_v;
    values[12] = measurement.bemf_w_adc_v;
    values[13] = measurement.vbus_adc_v;
    values[14] = measurement.vdda_v;
    values[15] = (float)measurement.vrefint_raw;
    values[16] = debug->vf->freq;
    values[17] = (float)debug->svpwm->pwm.cmp_a;
    values[18] = (float)debug->svpwm->pwm.cmp_b;
    values[19] = (float)debug->svpwm->pwm.cmp_c;

    return VOFA_NORMAL_MAX_CHANNELS;
}

/**
 * @brief 填充 VOFA Scope 通道。
 *
 * 这是应用层模板。后续要改为三相电流、Ud/Uq/theta 或其他
 * PWM 同步变量时，优先改这里。
 */
static uint8_t app_debug_fill_vofa_scope_pwm(float *values,
                                             uint8_t max_count,
                                             const pwm_output_t *pwm)
{
    if (values == NULL || pwm == NULL ||
        max_count < VOFA_SCOPE_MAX_CHANNELS) {
        return 0U;
    }

    values[0] = (float)pwm->cmp_a;
    values[1] = (float)pwm->cmp_b;
    values[2] = (float)pwm->cmp_c;

    return VOFA_SCOPE_MAX_CHANNELS;
}

int app_debug_init(app_debug_t *debug, const app_debug_config_t *config)
{
    if (debug == NULL || config == NULL || config->vf == NULL ||
        config->svpwm == NULL || config->cmd == NULL ||
        config->scope == NULL || config->measurement == NULL ||
        config->fault == NULL) {
        return -1;
    }

    debug->vf    = config->vf;
    debug->svpwm = config->svpwm;
    debug->cmd   = config->cmd;
    debug->scope = config->scope;
    debug->measurement = config->measurement;
    debug->fault = config->fault;

    return 0;
}

void app_debug_poll(app_debug_t *debug, uint32_t now_ms)
{
    float values[VOFA_NORMAL_MAX_CHANNELS];
    uint8_t count;

    if (debug == NULL || debug->cmd == NULL || debug->scope == NULL) {
        return;
    }

    (void)comm_cmd_poll(debug->cmd);

    count = app_debug_fill_vofa_normal(debug,
                                       values,
                                       VOFA_NORMAL_MAX_CHANNELS);
    if (count > 0U) {
        (void)comm_scope_poll_normal(debug->scope,
                                     now_ms,
                                     values,
                                     count);
    }
}

void app_debug_on_pwm_update(app_debug_t *debug, const pwm_output_t *pwm)
{
    float values[VOFA_SCOPE_MAX_CHANNELS];
    uint8_t count;

    if (debug == NULL || debug->scope == NULL || pwm == NULL) {
        return;
    }

    count = app_debug_fill_vofa_scope_pwm(values,
                                          VOFA_SCOPE_MAX_CHANNELS,
                                          pwm);
    if (count > 0U) {
        (void)comm_scope_send_scope(debug->scope, values, count);
    }
}

void app_debug_cmd_start(void *user)
{
    app_debug_t *debug = (app_debug_t *)user;

    if (debug == NULL || debug->vf == NULL) {
        return;
    }

    /* 故障锁存后仍接收串口命令，但异常状态下 start 不生效。 */
    if (app_measurement_is_overcurrent_fault(debug->measurement) != 0U ||
        motor_fault_is_latched(debug->fault) != 0U) {
        return;
    }

    open_loop_vf_start(debug->vf);
}

void app_debug_cmd_stop(void *user)
{
    app_debug_t *debug = (app_debug_t *)user;

    if (debug == NULL || debug->vf == NULL) {
        return;
    }

    open_loop_vf_stop(debug->vf);
}

void app_debug_cmd_set_freq(void *user, float freq_hz)
{
    app_debug_t *debug = (app_debug_t *)user;

    if (debug == NULL || debug->vf == NULL) {
        return;
    }

    open_loop_vf_set_target_freq(debug->vf, freq_hz);
}
