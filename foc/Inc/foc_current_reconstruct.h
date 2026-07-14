/**
 * @file    foc_current_reconstruct.h
 * @brief   三电阻电流按扇区选择与第三相重构。
 */

#ifndef FOC_CURRENT_RECONSTRUCT_H
#define FOC_CURRENT_RECONSTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    FOC_CURRENT_PHASE_U = 0U,
    FOC_CURRENT_PHASE_V = 1U,
    FOC_CURRENT_PHASE_W = 2U,
} foc_current_phase_t;

typedef struct {
    foc_current_phase_t sample_phase_a;
    foc_current_phase_t sample_phase_b;
    foc_current_phase_t reconstruct_phase;
} foc_current_sample_plan_t;

typedef struct {
    float current_u_a;
    float current_v_a;
    float current_w_a;
    float sum_error_a;
    uint8_t sector;
    uint8_t reconstructed;
} foc_current_result_t;

/**
 * @brief 获取指定扇区的两相采样计划。
 * @param sector 扇区编号，范围 1~6。
 * @return 采样计划；无效扇区返回 NULL。
 */
const foc_current_sample_plan_t *foc_current_get_sample_plan(uint8_t sector);

/**
 * @brief 按扇区使用两相电流重构第三相。
 *
 * 重构公式为：Iu + Iv + Iw = 0。
 * 输入参数仍包含三相，是为了兼容当前 ADC 三路 Rank；函数只使用
 * 当前扇区计划中的两相，第三相输入值会被忽略。
 *
 * @return 0 表示已完成重构，负数表示扇区无效。
 */
int foc_current_reconstruct(uint8_t sector,
                            float current_u_a,
                            float current_v_a,
                            float current_w_a,
                            foc_current_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* FOC_CURRENT_RECONSTRUCT_H */
