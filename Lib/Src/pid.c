/**
 * @file pid.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief PID 控制器实现
 * @version 0.3
 * @date 2026-06-29
 *
 * @par 版本记录
 *  - 0.1 初始版本
 *  - 0.2 conditional integration anti-windup
 *  - 0.3 增加 dt 异常保护、NaN/Inf 防护
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "pid.h"

/** 最小有效控制周期 (s)，低于此值视为异常 */
#define PID_DT_MIN  1.0e-7f

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

/**
 * @brief 输出限幅
 */
static float pid_clamp(float value, float out_min, float out_max)
{
    if (value > out_max) return out_max;
    if (value < out_min) return out_min;
    return value;
}

float pid_compute(pid_t *pid, float setpoint, float measurement, float dt)
{
    if (pid == NULL) {
        return 0.0f;
    }

    float error = setpoint - measurement;

    /* ---------- P 项 ---------- */
    float p = pid->kp * error;

    /* ---------- D 项 ---------- */
    float d = 0.0f;
    int dt_valid = (dt > PID_DT_MIN) && isfinite(dt);

    if (dt_valid && pid->kd != 0.0f) {
        d = pid->kd * (error - pid->prev_error) / dt;
    }

    /* 无论 dt 是否有效，都更新 prev_error，
       避免下一周期产生人为微分冲击 */
    pid->prev_error = error;

    /* ---------- dt 异常: 跳过积分，只返回 P + 原积分 ---------- */
    if (!dt_valid) {
        float output = p + pid->ki * pid->integral;
        if (!isfinite(output)) {
            output = 0.0f;
        }
        return pid_clamp(output, pid->out_min, pid->out_max);
    }

    /* ---------- 试算: 先加入本次积分再判断是否饱和 ---------- */
    float tentative_integral = pid->integral + error * dt;
    float tentative_output = p + pid->ki * tentative_integral + d;

    /* 输出非有限值时回退 */
    if (!isfinite(tentative_output)) {
        float output = p + pid->ki * pid->integral;
        if (!isfinite(output)) {
            output = 0.0f;
        }
        return pid_clamp(output, pid->out_min, pid->out_max);
    }

    /* conditional integration anti-windup */
    if (tentative_output > pid->out_max || tentative_output < pid->out_min) {
        /* 饱和: 不累积积分，保持原 integral 不变 */
        float output = p + pid->ki * pid->integral + d;
        if (!isfinite(output)) {
            output = 0.0f;
        }
        return pid_clamp(output, pid->out_min, pid->out_max);
    }

    /* 未饱和: 正常累积 */
    pid->integral = tentative_integral;
    return tentative_output;
}

// EOF
