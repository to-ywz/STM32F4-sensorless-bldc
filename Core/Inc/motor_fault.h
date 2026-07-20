/**
 * @file    motor_fault.h
 * @brief   电机控制故障锁存与现场保存。
 * @version 0.1.0
 * @date    2026-07-20
 */

#ifndef MOTOR_FAULT_H
#define MOTOR_FAULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    MOTOR_FAULT_NONE       = 0U,
    MOTOR_FAULT_MODULATION = 1U,
    MOTOR_FAULT_OVERCURRENT = 2U
} motor_fault_code_t;

typedef struct {
    uint8_t  svpwm_fault;
    uint8_t  sector;
    float    v_alpha;
    float    v_beta;
    uint32_t cmp_a;
    uint32_t cmp_b;
    uint32_t cmp_c;
    uint32_t tick_ms;
} motor_fault_context_t;

typedef struct {
    volatile uint8_t          latched;
    volatile motor_fault_code_t code;
    motor_fault_context_t     context;
} motor_fault_t;

void motor_fault_init(motor_fault_t *fault);

/**
 * @brief 锁存首次故障并保存现场，后续故障不覆盖首次现场。
 */
void motor_fault_raise(motor_fault_t *fault,
                       motor_fault_code_t code,
                       const motor_fault_context_t *context);

uint8_t motor_fault_is_latched(const motor_fault_t *fault);
motor_fault_code_t motor_fault_get_code(const motor_fault_t *fault);
int motor_fault_get_context(const motor_fault_t *fault,
                            motor_fault_context_t *context);

/**
 * @brief 清除故障锁存。
 * @note 仅允许在明确的人工复位流程中调用，禁止自动调用。
 */
void motor_fault_clear(motor_fault_t *fault);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_FAULT_H */
