/**
 * @file open_loop_vf.h
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief 开环 V/f 控制
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef OPEN_LOOP_VF_H
#define OPEN_LOOP_VF_H

/**
 * @brief 开环 V/f 控制状态
 *
 * 成员:
 *   freq        - 当前频率 (Hz)
 *   target_freq - 目标频率 (Hz)，只支持 0 或 freq_max
 *   accel       - 加速度 (Hz/s)
 *   vf_ratio    - V/f 比 (V/Hz)
 *   v_max       - 最大电压限幅 (V)
 *   v_min       - 最小电压限幅 (V)
 *   theta_e     - 电角度 (rad)
 *   v_out       - 输出电压幅值 (V)
 */
typedef struct {
    float freq;
    float target_freq;
    float accel;
    float vf_ratio;
    float v_max;
    float v_min;
    float theta_e;
    float v_out;
} open_loop_vf_t;

/**
 * @brief 初始化 V/f 参数
 *
 * @param vf        : V/f 实例指针
 * @param target_freq : 目标频率 (Hz)，只支持 0 或 freq_max
 * @param accel     : 加速度 (Hz/s)
 * @param vf_ratio  : V/f 比 (V/Hz)
 * @param v_max     : 最大电压限幅 (V)
 * @param v_min     : 最小电压限幅 (V)
 */
void open_loop_vf_init(open_loop_vf_t *vf, float target_freq, float accel, float vf_ratio, float v_max, float v_min);

/**
 * @brief V/f 单步更新
 *
 * 内部累加频率和角度，输出电压幅值和电角度。
 * 供外部调用 InvPark(vd=v_out, vq=0, theta_e) 和 SVPWM。
 *
 * @param vf : V/f 实例指针
 * @param dt : 时间步长 (s)
 */
void open_loop_vf_step(open_loop_vf_t *vf, float dt);

#endif
