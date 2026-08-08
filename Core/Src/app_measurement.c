/**
 * @file    app_measurement.c
 * @brief   相电压与母线电压测量应用模块。
 */

#include "app_measurement.h"
#include <math.h>
#include <stddef.h>

int app_measurement_init(app_measurement_t *measurement,
                         const app_measurement_config_t *config)
{
    if (measurement == NULL || config == NULL ||
        config->adc_vref <= 0.0f || config->adc_max_count <= 0.0f ||
        config->vrefint_cal_raw == 0U || config->vrefint_cal_v <= 0.0f ||
        config->current_shunt_ohm <= 0.0f ||
        config->current_amplifier_gain <= 0.0f ||
        config->software_overcurrent_limit_a <= 0.0f ||
        config->software_overcurrent_trip_count == 0U ||
        config->bemf_scale <= 0.0f || config->vbus_scale <= 0.0f) {
        return -1;
    }

    // TODO：这里赋值太多了，是否考虑将 直接 = 0，然后只赋值 config，减少代码量和出错概率。
    measurement->config = *config;
    measurement->sequence = 0U;
    measurement->current_u_raw = 0U;
    measurement->current_v_raw = 0U;
    measurement->current_w_raw = 0U;
    measurement->current_sector_applied = 0U;
    measurement->current_calibration_target = 0U;
    measurement->current_calibration_count = 0U;
    measurement->current_calibration_sum_u = 0U;
    measurement->current_calibration_sum_v = 0U;
    measurement->current_calibration_sum_w = 0U;
    measurement->current_offset_u_raw = config->current_offset_v /
                                        config->adc_vref *
                                        config->adc_max_count;
    measurement->current_offset_v_raw = measurement->current_offset_u_raw;
    measurement->current_offset_w_raw = measurement->current_offset_u_raw;
    measurement->overcurrent_enabled = 0U;
    measurement->overcurrent_fault = 0U;
    measurement->overcurrent_trip_count = 0U;
    measurement->bemf_u_raw = 0U;
    measurement->bemf_v_raw = 0U;
    measurement->bemf_w_raw = 0U;
    measurement->vbus_raw = 0U;
    measurement->vrefint_raw = config->vrefint_cal_raw;

    return 0;
}

void app_measurement_set_overcurrent_enabled(
    app_measurement_t *measurement,
    uint8_t enabled)
{
    if (measurement == NULL) {
        return;
    }

    measurement->overcurrent_enabled = enabled ? 1U : 0U;
    measurement->overcurrent_trip_count = 0U;
}

uint8_t app_measurement_is_overcurrent_fault(
    const app_measurement_t *measurement)
{
    if (measurement == NULL) {
        return 0U;
    }

    return measurement->overcurrent_fault;
}

uint8_t app_measurement_check_overcurrent(
    app_measurement_t *measurement,
    uint8_t sector_applied,
    uint16_t current_u_raw,
    uint16_t current_v_raw,
    uint16_t current_w_raw)
{
    float vdda_v;
    float current_u_a;
    float current_v_a;
    float current_w_a;
    float current_peak_a;
    current_three_shunt_result_t current_result;

    if (measurement == NULL ||
        measurement->overcurrent_enabled == 0U ||
        measurement->overcurrent_fault != 0U ||
        measurement->current_calibration_target != 0U) {
        return 0U;
    }

    if (measurement->vrefint_raw == 0U) {
        vdda_v = measurement->config.adc_vref;
    } else {
        vdda_v = (float)measurement->config.vrefint_cal_raw *
                 measurement->config.vrefint_cal_v /
                 (float)measurement->vrefint_raw;
    }

    current_u_a = ((float)current_u_raw -
                   measurement->current_offset_u_raw) * vdda_v /
                  (measurement->config.adc_max_count *
                   measurement->config.current_shunt_ohm *
                   measurement->config.current_amplifier_gain);
    current_v_a = ((float)current_v_raw -
                   measurement->current_offset_v_raw) * vdda_v /
                  (measurement->config.adc_max_count *
                   measurement->config.current_shunt_ohm *
                   measurement->config.current_amplifier_gain);
    current_w_a = ((float)current_w_raw -
                   measurement->current_offset_w_raw) * vdda_v /
                  (measurement->config.adc_max_count *
                   measurement->config.current_shunt_ohm *
                   measurement->config.current_amplifier_gain);

    /* 保护判断必须使用与 VOFA 相同的扇区重构结果。 */
    if (current_three_shunt_process(sector_applied,
                                    current_u_a,
                                    current_v_a,
                                    current_w_a,
                                    &current_result) == 0) {
        current_u_a = current_result.current_u_a;
        current_v_a = current_result.current_v_a;
        current_w_a = current_result.current_w_a;
    }

    current_peak_a = fmaxf(fabsf(current_u_a), fabsf(current_v_a));
    current_peak_a = fmaxf(current_peak_a, fabsf(current_w_a));

    if (current_peak_a >= measurement->config.software_overcurrent_limit_a) {
        if (measurement->overcurrent_trip_count < UINT16_MAX) {
            measurement->overcurrent_trip_count++;
        }

        if (measurement->overcurrent_trip_count >=
            measurement->config.software_overcurrent_trip_count) {
            measurement->overcurrent_fault = 1U;
            measurement->overcurrent_enabled = 0U;
            return 1U;
        }
    } else {
        measurement->overcurrent_trip_count = 0U;
    }

    return 0U;
}

void app_measurement_update_current(app_measurement_t *measurement,
                                    uint8_t sector_applied,
                                    uint16_t current_u_raw,
                                    uint16_t current_v_raw,
                                    uint16_t current_w_raw)
{
    if (measurement == NULL) {
        return;
    }

    measurement->sequence++;
    measurement->current_u_raw = current_u_raw;
    measurement->current_v_raw = current_v_raw;
    measurement->current_w_raw = current_w_raw;
    measurement->current_sector_applied = sector_applied;

    if (measurement->current_calibration_count <
        measurement->current_calibration_target) {
        measurement->current_calibration_sum_u += current_u_raw;
        measurement->current_calibration_sum_v += current_v_raw;
        measurement->current_calibration_sum_w += current_w_raw;
        measurement->current_calibration_count++;
    }
    measurement->sequence++;
}

void app_measurement_start_current_zero_calibration(
    app_measurement_t *measurement,
    uint32_t sample_count)
{
    if (measurement == NULL || sample_count == 0U) {
        return;
    }

    measurement->current_calibration_sum_u = 0U;
    measurement->current_calibration_sum_v = 0U;
    measurement->current_calibration_sum_w = 0U;
    measurement->current_calibration_count = 0U;
    measurement->current_calibration_target = sample_count;
}

uint8_t app_measurement_is_current_zero_calibration_done(
    const app_measurement_t *measurement)
{
    if (measurement == NULL || measurement->current_calibration_target == 0U) {
        return 0U;
    }

    return (measurement->current_calibration_count >=
            measurement->current_calibration_target) ? 1U : 0U;
}

int app_measurement_apply_current_zero_calibration(
    app_measurement_t *measurement)
{
    uint32_t count;

    if (measurement == NULL ||
        !app_measurement_is_current_zero_calibration_done(measurement)) {
        return -1;
    }

    count = measurement->current_calibration_count;
    measurement->current_offset_u_raw =
        (float)measurement->current_calibration_sum_u / (float)count;
    measurement->current_offset_v_raw =
        (float)measurement->current_calibration_sum_v / (float)count;
    measurement->current_offset_w_raw =
        (float)measurement->current_calibration_sum_w / (float)count;
    measurement->current_calibration_target = 0U;

    return 0;
}

void app_measurement_update(app_measurement_t *measurement,
                            uint16_t bemf_u_raw,
                            uint16_t bemf_v_raw,
                            uint16_t bemf_w_raw,
                            uint16_t vbus_raw)
{
    if (measurement == NULL) {
        return;
    }

    /* 奇数表示更新中，偶数表示一组采样已经完整写入。 */
    measurement->sequence++;
    measurement->bemf_u_raw = bemf_u_raw;
    measurement->bemf_v_raw = bemf_v_raw;
    measurement->bemf_w_raw = bemf_w_raw;
    measurement->vbus_raw = vbus_raw;
    measurement->sequence++;
}

void app_measurement_update_vrefint(app_measurement_t *measurement,
                                    uint16_t vrefint_raw)
{
    if (measurement == NULL || vrefint_raw == 0U) {
        return;
    }

    measurement->sequence++;
    measurement->vrefint_raw = vrefint_raw;
    measurement->sequence++;
}

int app_measurement_get(const app_measurement_t *measurement,
                        app_measurement_sample_t *sample)
{
    uint32_t sequence_start;
    uint32_t sequence_end;
    uint16_t vrefint_raw;
    float vdda_v;
    current_three_shunt_result_t current_result;

    if (measurement == NULL || sample == NULL) {
        return -1;
    }

    for (;;) { // 数据一致性保障
        sequence_start = measurement->sequence;
        if ((sequence_start & 1U) != 0U) {
            continue;
        }

        sample->bemf_u_raw = measurement->bemf_u_raw;
        sample->bemf_v_raw = measurement->bemf_v_raw;
        sample->bemf_w_raw = measurement->bemf_w_raw;
        sample->vbus_raw = measurement->vbus_raw;
        sample->vrefint_raw = measurement->vrefint_raw;
        sample->current_u_raw = measurement->current_u_raw;
        sample->current_v_raw = measurement->current_v_raw;
        sample->current_w_raw = measurement->current_w_raw;
        sample->current_sector_applied = measurement->current_sector_applied;
        sequence_end = measurement->sequence;

        if (sequence_start == sequence_end &&
            (sequence_end & 1U) == 0U) {
            break;
        }
    }

    vrefint_raw = sample->vrefint_raw;
    if (vrefint_raw == 0U) {
        vdda_v = measurement->config.adc_vref;
    } else {
        vdda_v = (float)measurement->config.vrefint_cal_raw *
                 measurement->config.vrefint_cal_v /
                 (float)vrefint_raw;
    }
    sample->vdda_v = vdda_v;

    sample->current_u_adc_v = (float)sample->current_u_raw *
                              vdda_v / measurement->config.adc_max_count;
    sample->current_v_adc_v = (float)sample->current_v_raw *
                              vdda_v / measurement->config.adc_max_count;
    sample->current_w_adc_v = (float)sample->current_w_raw *
                              vdda_v / measurement->config.adc_max_count;

    sample->current_u_a = ((float)sample->current_u_raw -
                           measurement->current_offset_u_raw) * vdda_v /
                          (measurement->config.adc_max_count *
                           measurement->config.current_shunt_ohm *
                           measurement->config.current_amplifier_gain);
    sample->current_v_a = ((float)sample->current_v_raw -
                           measurement->current_offset_v_raw) * vdda_v /
                          (measurement->config.adc_max_count *
                           measurement->config.current_shunt_ohm *
                           measurement->config.current_amplifier_gain);
    sample->current_w_a = ((float)sample->current_w_raw -
                           measurement->current_offset_w_raw) * vdda_v /
                          (measurement->config.adc_max_count *
                           measurement->config.current_shunt_ohm *
                           measurement->config.current_amplifier_gain);

    if (current_three_shunt_process(sample->current_sector_applied,
                                    sample->current_u_a,
                                    sample->current_v_a,
                                    sample->current_w_a,
                                    &current_result) == 0) {
        sample->current_u_a = current_result.current_u_a;
        sample->current_v_a = current_result.current_v_a;
        sample->current_w_a = current_result.current_w_a;
        sample->current_sum_error_a = current_result.sum_error_a;
        sample->current_reconstructed = current_result.reconstructed;
    } else {
        sample->current_sum_error_a = sample->current_u_a +
                                      sample->current_v_a +
                                      sample->current_w_a;
        sample->current_reconstructed = 0U;
    }

    sample->bemf_u_adc_v = (float)sample->bemf_u_raw *
                           vdda_v / measurement->config.adc_max_count;
    sample->bemf_v_adc_v = (float)sample->bemf_v_raw *
                           vdda_v / measurement->config.adc_max_count;
    sample->bemf_w_adc_v = (float)sample->bemf_w_raw *
                           vdda_v / measurement->config.adc_max_count;
    sample->vbus_adc_v = (float)sample->vbus_raw *
                        vdda_v / measurement->config.adc_max_count;

    sample->bemf_u_v = (sample->bemf_u_adc_v -
                        measurement->config.bemf_offset_v) *
                       measurement->config.bemf_scale;
    sample->bemf_v_v = (sample->bemf_v_adc_v -
                        measurement->config.bemf_offset_v) *
                       measurement->config.bemf_scale;
    sample->bemf_w_v = (sample->bemf_w_adc_v -
                        measurement->config.bemf_offset_v) *
                       measurement->config.bemf_scale;
    sample->vbus_v = (sample->vbus_adc_v -
                      measurement->config.vbus_offset_v) *
                     measurement->config.vbus_scale;

    return 0;
}
