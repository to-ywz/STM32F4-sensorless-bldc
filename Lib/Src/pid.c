/**
 * @file pid.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief PID 控制器实现
 * @version 0.2
 * @date 2026-05-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "pid.h"

void pid_init(pid_t *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
}

float pid_compute(pid_t *pid, float setpoint, float measurement, float dt)
{
    float error = setpoint - measurement;

    float p = pid->kp * error;
    float d = pid->kd * (error - pid->prev_error) / dt;
    pid->prev_error = error;

    /* 试算: 先加入本次积分再判断是否饱和 */
    float tentative_integral = pid->integral + error * dt;
    float tentative_output = p + pid->ki * tentative_integral + d;

    /* conditional integration anti-windup */
    if (tentative_output > pid->out_max || tentative_output < pid->out_min) {
        /* 饱和: 不累积积分，保持原 integral 不变 */
        float output = p + pid->ki * pid->integral + d;
        if (output > pid->out_max) return pid->out_max;
        if (output < pid->out_min) return pid->out_min;
        return output;
    }

    /* 未饱和: 正常累积 */
    pid->integral = tentative_integral;
    return tentative_output;
}

// EOF
