/**
 * @file    motor_fault.c
 * @brief   电机控制故障锁存与现场保存。
 * @version 0.1.0
 * @date    2026-07-20
 */

#include "motor_fault.h"
#include <stddef.h>

void motor_fault_init(motor_fault_t *fault)
{
    if (fault == NULL) {
        return;
    }

    fault->latched = 0U;
    fault->code = MOTOR_FAULT_NONE;
    fault->context = (motor_fault_context_t){0};
}

void motor_fault_raise(motor_fault_t *fault,
                       motor_fault_code_t code,
                       const motor_fault_context_t *context)
{
    if (fault == NULL || code == MOTOR_FAULT_NONE || fault->latched != 0U) {
        return;
    }

    if (context != NULL) {
        fault->context = *context;
    } else {
        fault->context = (motor_fault_context_t){0};
    }

    fault->code = code;
    fault->latched = 1U;
}

uint8_t motor_fault_is_latched(const motor_fault_t *fault)
{
    return (fault != NULL) ? fault->latched : 0U;
}

motor_fault_code_t motor_fault_get_code(const motor_fault_t *fault)
{
    return (fault != NULL) ? fault->code : MOTOR_FAULT_NONE;
}

int motor_fault_get_context(const motor_fault_t *fault,
                            motor_fault_context_t *context)
{
    if (fault == NULL || context == NULL || fault->latched == 0U) {
        return -1;
    }

    *context = fault->context;
    return 0;
}

void motor_fault_clear(motor_fault_t *fault)
{
    if (fault == NULL) {
        return;
    }

    fault->latched = 0U;
    fault->code = MOTOR_FAULT_NONE;
    fault->context = (motor_fault_context_t){0};
}
