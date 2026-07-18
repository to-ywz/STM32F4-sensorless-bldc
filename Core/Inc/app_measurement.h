/**
 * @file    app_measurement.h
 * @brief   相电压与母线电压测量应用模块。
 */

#ifndef APP_MEASUREMENT_H
#define APP_MEASUREMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "foc_current_reconstruct.h"

/**
 * @brief ADC、电流和电压测量换算配置。
 */
typedef struct {
    float adc_vref;
    float adc_max_count;
    uint16_t vrefint_cal_raw;
    float vrefint_cal_v;
    float current_offset_v;
    float current_shunt_ohm;
    float current_amplifier_gain;
    float software_overcurrent_limit_a;
    uint16_t software_overcurrent_trip_count;
    float bemf_offset_v;
    float bemf_scale;
    float vbus_offset_v;
    float vbus_scale;
} app_measurement_config_t;

/**
 * @brief 一次相电压与母线电压测量结果。
 */
typedef struct {
    uint16_t current_u_raw;
    uint16_t current_v_raw;
    uint16_t current_w_raw;
    uint16_t bemf_u_raw;
    uint16_t bemf_v_raw;
    uint16_t bemf_w_raw;
    uint16_t vbus_raw;
    float bemf_u_v;
    float bemf_v_v;
    float bemf_w_v;
    float vbus_v;
    float current_u_a;
    float current_v_a;
    float current_w_a;
    float current_u_adc_v;
    float current_v_adc_v;
    float current_w_adc_v;
    float current_sum_error_a;
    uint8_t current_sector;
    uint8_t current_reconstructed;
    float bemf_u_adc_v;
    float bemf_v_adc_v;
    float bemf_w_adc_v;
    float vbus_adc_v;
    float vdda_v;
    uint16_t vrefint_raw;
} app_measurement_sample_t;

/**
 * @brief 相电压与母线电压测量对象。
 *
 * 原始值由 ADC 中断更新，主循环通过 getter 读取。
 */
typedef struct {
    app_measurement_config_t config;
    volatile uint32_t sequence;
    volatile uint16_t current_u_raw;
    volatile uint16_t current_v_raw;
    volatile uint16_t current_w_raw;
    volatile uint8_t current_sector;
    volatile uint32_t current_calibration_target;
    volatile uint32_t current_calibration_count;
    volatile uint32_t current_calibration_sum_u;
    volatile uint32_t current_calibration_sum_v;
    volatile uint32_t current_calibration_sum_w;
    float current_offset_u_raw;
    float current_offset_v_raw;
    float current_offset_w_raw;
    volatile uint8_t overcurrent_enabled;
    volatile uint8_t overcurrent_fault;
    volatile uint16_t overcurrent_trip_count;
    volatile uint16_t bemf_u_raw;
    volatile uint16_t bemf_v_raw;
    volatile uint16_t bemf_w_raw;
    volatile uint16_t vbus_raw;
    volatile uint16_t vrefint_raw;
} app_measurement_t;

/**
 * @brief 初始化测量对象。
 */
int app_measurement_init(app_measurement_t *measurement,
                         const app_measurement_config_t *config);

/**
 * @brief 更新一次 ADC3 注入序列结果。
 */
void app_measurement_update(app_measurement_t *measurement,
                            uint16_t bemf_u_raw,
                            uint16_t bemf_v_raw,
                            uint16_t bemf_w_raw,
                            uint16_t vbus_raw);

/**
 * @brief 更新一次 ADC1 三相电流原始采样值。
 */
void app_measurement_update_current(app_measurement_t *measurement,
                                    uint8_t current_sector,
                                    uint16_t current_u_raw,
                                    uint16_t current_v_raw,
                                    uint16_t current_w_raw);

/**
 * @brief 设置软件过流保护是否开始监控。
 *
 * 保护只应在三相零点校准完成且 PWM 已经准备好后启用。
 */
void app_measurement_set_overcurrent_enabled(
    app_measurement_t *measurement,
    uint8_t enabled);

/**
 * @brief 使用本次 ADC 原始值执行软件过流判断。
 *
 * @return 1 表示本次新触发过流故障，0 表示未触发。
 */
uint8_t app_measurement_check_overcurrent(
    app_measurement_t *measurement,
    uint8_t current_sector,
    uint16_t current_u_raw,
    uint16_t current_v_raw,
    uint16_t current_w_raw);

/**
 * @brief 查询软件过流故障锁存状态。
 */
uint8_t app_measurement_is_overcurrent_fault(
    const app_measurement_t *measurement);

/**
 * @brief 开始三相电流零点采样。
 */
void app_measurement_start_current_zero_calibration(
    app_measurement_t *measurement,
    uint32_t sample_count);

/**
 * @brief 查询三相电流零点采样是否完成。
 */
uint8_t app_measurement_is_current_zero_calibration_done(
    const app_measurement_t *measurement);

/**
 * @brief 应用最近一次三相电流零点采样结果。
 */
int app_measurement_apply_current_zero_calibration(
    app_measurement_t *measurement);

/**
 * @brief 更新一次 VREFINT 原始采样值。
 */
void app_measurement_update_vrefint(app_measurement_t *measurement,
                                    uint16_t vrefint_raw);

/**
 * @brief 获取最近一次测量结果并完成电压换算。
 */
int app_measurement_get(const app_measurement_t *measurement,
                        app_measurement_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* APP_MEASUREMENT_H */
