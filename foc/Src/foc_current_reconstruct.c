/**
 * @file    foc_current_reconstruct.c
 * @brief   三电阻电流按扇区选择与第三相重构。
 */

#include "foc_current_reconstruct.h"
#include <stddef.h>

/*
 * 当前已确认的占空比排序下，最大占空比相的低侧有效窗口最短，
 * 因此暂不使用该相，使用另外两相并重构最大占空比相。
 */
static const foc_current_sample_plan_t g_sample_plans[6] = {
    [0] = {FOC_CURRENT_PHASE_V, FOC_CURRENT_PHASE_W,
           FOC_CURRENT_PHASE_U}, /* S1: A > B > C */
    [1] = {FOC_CURRENT_PHASE_U, FOC_CURRENT_PHASE_W,
           FOC_CURRENT_PHASE_V}, /* S2: B > A > C */
    [2] = {FOC_CURRENT_PHASE_U, FOC_CURRENT_PHASE_W,
           FOC_CURRENT_PHASE_V}, /* S3: B > C > A */
    [3] = {FOC_CURRENT_PHASE_U, FOC_CURRENT_PHASE_V,
           FOC_CURRENT_PHASE_W}, /* S4: C > B > A */
    [4] = {FOC_CURRENT_PHASE_U, FOC_CURRENT_PHASE_V,
           FOC_CURRENT_PHASE_W}, /* S5: C > A > B */
    [5] = {FOC_CURRENT_PHASE_V, FOC_CURRENT_PHASE_W,
           FOC_CURRENT_PHASE_U}, /* S6: A > C > B */
};

const foc_current_sample_plan_t *foc_current_get_sample_plan(uint8_t sector)
{
    if (sector < 1U || sector > 6U) {
        return NULL;
    }

    return &g_sample_plans[sector - 1U];
}

static float foc_current_get_value(foc_current_phase_t phase,
                                   float current_u_a,
                                   float current_v_a,
                                   float current_w_a)
{
    switch (phase) {
    case FOC_CURRENT_PHASE_U: return current_u_a;
    case FOC_CURRENT_PHASE_V: return current_v_a;
    case FOC_CURRENT_PHASE_W: return current_w_a;
    default: return 0.0f;
    }
}

static void foc_current_set_value(foc_current_phase_t phase,
                                  float value,
                                  foc_current_result_t *result)
{
    switch (phase) {
    case FOC_CURRENT_PHASE_U: result->current_u_a = value; break;
    case FOC_CURRENT_PHASE_V: result->current_v_a = value; break;
    case FOC_CURRENT_PHASE_W: result->current_w_a = value; break;
    default: break;
    }
}

int foc_current_reconstruct(uint8_t sector,
                            float current_u_a,
                            float current_v_a,
                            float current_w_a,
                            foc_current_result_t *result)
{
    const foc_current_sample_plan_t *plan;
    float sample_a;
    float sample_b;
    float reconstructed;

    if (result == NULL) {
        return -1;
    }

    plan = foc_current_get_sample_plan(sector);
    if (plan == NULL) {
        return -2;
    }

    sample_a = foc_current_get_value(plan->sample_phase_a,
                                     current_u_a, current_v_a, current_w_a);
    sample_b = foc_current_get_value(plan->sample_phase_b,
                                     current_u_a, current_v_a, current_w_a);
    reconstructed = -(sample_a + sample_b);

    result->current_u_a = current_u_a;
    result->current_v_a = current_v_a;
    result->current_w_a = current_w_a;
    foc_current_set_value(plan->sample_phase_a, sample_a, result);
    foc_current_set_value(plan->sample_phase_b, sample_b, result);
    foc_current_set_value(plan->reconstruct_phase, reconstructed, result);
    result->sum_error_a = result->current_u_a +
                          result->current_v_a +
                          result->current_w_a;
    result->sector = sector;
    result->reconstructed = 1U;

    return 0;
}
