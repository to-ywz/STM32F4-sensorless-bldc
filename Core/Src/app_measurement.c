/**
 * @file    app_measurement.c
 * @brief   相电压与母线电压测量应用模块。
 */

#include "app_measurement.h"
#include <stddef.h>

int app_measurement_init(app_measurement_t *measurement,
                         const app_measurement_config_t *config)
{
    if (measurement == NULL || config == NULL ||
        config->adc_vref <= 0.0f || config->adc_max_count <= 0.0f ||
        config->vrefint_cal_raw == 0U || config->vrefint_cal_v <= 0.0f ||
        config->bemf_scale <= 0.0f || config->vbus_scale <= 0.0f) {
        return -1;
    }

    measurement->config = *config;
    measurement->sequence = 0U;
    measurement->bemf_u_raw = 0U;
    measurement->bemf_v_raw = 0U;
    measurement->bemf_w_raw = 0U;
    measurement->vbus_raw = 0U;
    measurement->vrefint_raw = config->vrefint_cal_raw;

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

    if (measurement == NULL || sample == NULL) {
        return -1;
    }

    for (;;) {
        sequence_start = measurement->sequence;
        if ((sequence_start & 1U) != 0U) {
            continue;
        }

        sample->bemf_u_raw = measurement->bemf_u_raw;
        sample->bemf_v_raw = measurement->bemf_v_raw;
        sample->bemf_w_raw = measurement->bemf_w_raw;
        sample->vbus_raw = measurement->vbus_raw;
        sample->vrefint_raw = measurement->vrefint_raw;
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
