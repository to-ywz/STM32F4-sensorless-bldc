/**
 * @file foc_math.h
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief FOC 数学函数 (Clarke / Park / InvPark / PI / 矢量限幅)
 * @version 0.1
 * @date 2026-05-26
 *
 * @par 版本记录
 *  - 0.1 初始版本
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef FOC_MATH_H
#define FOC_MATH_H

#include "pid.h"

/**
 * @brief Clarke 变换 (两相静止 αβ 坐标系)
 *
 * 公式: iα = ia, iβ = (ia + 2*ib) / √3
 * 三相平衡约束: ia + ib + ic = 0，只需 ia, ib
 *
 * @param ia      : A 相电流
 * @param ib      : B 相电流
 * @param i_alpha : 输出 α 轴电流
 * @param i_beta  : 输出 β 轴电流
 */
void foc_clarke(float ia, float ib, float* i_alpha, float* i_beta);

/**
 * @brief Park 变换 (两相旋转 dq 坐标系)
 *
 * 公式: id = iα*cosθ + iβ*sinθ, iq = -iα*sinθ + iβ*cosθ
 * 角度内部自动归一化到 [0, 2π)
 *
 * @param i_alpha : α 轴电流
 * @param i_beta  : β 轴电流
 * @param theta   : 电角度 (rad)
 * @param i_d     : 输出 d 轴电流
 * @param i_q     : 输出 q 轴电流
 */
void foc_park(float i_alpha, float i_beta, float theta, float* i_d, float* i_q);

/**
 * @brief 逆 Park 变换
 *
 * 公式: vα = vd*cosθ - vq*sinθ, vβ = vd*sinθ + vq*cosθ
 * 角度内部自动归一化到 [0, 2π)
 *
 * @param v_d     : d 轴电压
 * @param v_q     : q 轴电压
 * @param theta   : 电角度 (rad)
 * @param v_alpha : 输出 α 轴电压
 * @param v_beta  : 输出 β 轴电压
 */
void foc_inv_park(float v_d, float v_q, float theta, float* v_alpha, float* v_beta);

/**
 * @brief 初始化 FOC 电流环 PI (对称限幅, 无微分)
 *
 * @param pid     : PID 实例指针
 * @param kp      : 比例增益
 * @param ki      : 积分增益
 * @param v_max   : 电压输出限幅 (±v_max)
 */
void foc_pi_init(pid_t *pid, float kp, float ki, float v_max);

/**
 * @brief 电流环 PI 更新 (封装 pid_compute)
 *
 * @param pid    : PID 实例指针
 * @param ref    : 电流参考值 (id_ref / iq_ref)
 * @param actual : 实际电流 (id / iq)
 * @param dt     : 控制周期 (s)
 * @return float : 电压输出 (vd / vq)
 */
float foc_pi_update(pid_t *pid, float ref, float actual, float dt);

/**
 * @brief 电压矢量等比限幅
 *
 * |v| = sqrt(vd² + vq²)，超过 v_max 时等比缩放，保持方向不变。
 *
 * @param v_d   : d 轴电压指针 (输入输出)
 * @param v_q   : q 轴电压指针 (输入输出)
 * @param v_max : 最大电压矢量幅值 (V)
 */
void foc_vector_limit(float *v_d, float *v_q, float v_max);

#endif
