/**
 * @file pid.h
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief PID 控制器，支持积分限幅 (anti-windup) 和输出限幅
 * @version 0.2
 * @date 2026-05-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef __PID_H__
#define __PID_H__

/**
 * @brief PID 控制器结构体
 *
 * 成员:
 *   kp         - 比例增益
 *   ki         - 积分增益
 *   kd         - 微分增益
 *   integral   - 积分累加器
 *   prev_error - 上一次误差，用于微分计算
 *   out_min    - 输出下限
 *   out_max    - 输出上限
 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float out_min;
    float out_max;
} pid_t;

/**
 * @brief 初始化 PID 控制器
 *
 * @param pid     : PID 实例指针
 * @param kp      : 比例增益
 * @param ki      : 积分增益
 * @param kd      : 微分增益
 * @param out_min : 输出下限
 * @param out_max : 输出上限
 */
void pid_init(pid_t *pid, float kp, float ki, float kd, float out_min, float out_max);

/**
 * @brief 计算 PID 输出
 *
 * 采用 conditional integration anti-windup: 输出饱和时不累积积分，
 * 避免积分器溢出导致的大超调。
 *
 * 公式: output = Kp*e + Ki*∫e + Kd*(de/dt)
 *
 * @param pid        : PID 实例指针
 * @param setpoint   : 目标值
 * @param measurement: 实际测量值
 * @param dt         : 控制周期 (s)
 * @return float     : 控制输出，已限幅到 [out_min, out_max]
 */
float pid_compute(pid_t *pid, float setpoint, float measurement, float dt);

#endif
