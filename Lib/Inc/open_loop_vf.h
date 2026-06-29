/**
 * @file open_loop_vf.h
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief 开环 V/f 控制 (带状态机)
 * @version 0.2
 * @date 2026-06-29
 *
 * @par 版本记录
 *  - 0.1 初始版本，无状态机
 *  - 0.2 增加 STOP/ALIGN/RAMP/RUN 状态机，明确启停接口
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef OPEN_LOOP_VF_H
#define OPEN_LOOP_VF_H

#include <stdint.h>

/**
 * @brief V/F 控制状态枚举
 */
typedef enum {
    OPEN_LOOP_VF_STATE_STOP  = 0,   /**< 停止: v_out=0, 不输出电压矢量 */
    OPEN_LOOP_VF_STATE_ALIGN = 1,   /**< 转子预定位: 固定角度、固定电压 */
    OPEN_LOOP_VF_STATE_RAMP  = 2,   /**< 频率爬坡: freq 从 0 增长到 target_freq */
    OPEN_LOOP_VF_STATE_RUN   = 3    /**< 稳态运行: 维持 target_freq */
} open_loop_vf_state_t;

/**
 * @brief 开环 V/f 控制结构体
 *
 * 成员:
 *   state          - 当前状态
 *   freq           - 当前频率 (Hz)
 *   target_freq    - 目标频率 (Hz)
 *   accel          - 加速度 (Hz/s)
 *   vf_ratio       - V/f 比 (V/Hz)
 *   v_max          - 最大电压限幅 (V)
 *   v_min          - 低频补偿电压 (V)，仅在 RAMP/RUN 阶段使用
 *   align_voltage  - 预定位电压 (V)，独立于 v_min
 *   align_time     - 预定位持续时间 (s)
 *   align_elapsed  - 预定位已用时间 (s)
 *   theta_e        - 电角度 (rad)
 *   v_out          - 输出电压幅值 (V)
 *   enable         - 启停命令: 1=启动, 0=停止
 */
typedef struct {
    open_loop_vf_state_t state;
    float freq;
    float target_freq;
    float accel;
    float vf_ratio;
    float v_max;
    float v_min;
    float align_voltage;
    float align_time;
    float align_elapsed;
    float theta_e;
    float v_out;
    uint8_t enable;
} open_loop_vf_t;

/**
 * @brief 初始化 V/f 参数
 *
 * 初始化后状态为 STOP，v_out=0，enable=0。
 * 需要调用 open_loop_vf_start() 才会启动。
 *
 * @param vf           : V/f 实例指针
 * @param target_freq  : 目标频率 (Hz)
 * @param accel        : 加速度 (Hz/s)
 * @param vf_ratio     : V/f 比 (V/Hz)
 * @param v_max        : 最大电压限幅 (V)
 * @param v_min        : 低频补偿电压 (V)
 */
void open_loop_vf_init(open_loop_vf_t *vf, float target_freq, float accel,
                       float vf_ratio, float v_max, float v_min);

/**
 * @brief 配置预定位参数
 *
 * 必须在 open_loop_vf_start() 之前调用，否则使用默认值 (0)。
 *
 * @param vf            : V/f 实例指针
 * @param align_voltage : 预定位电压 (V)，应保守设置
 * @param align_time    : 预定位持续时间 (s)
 */
void open_loop_vf_set_align(open_loop_vf_t *vf, float align_voltage, float align_time);

/**
 * @brief 启动 V/F 控制
 *
 * 从 STOP 进入 ALIGN (如有配置) 或直接进入 RAMP。
 * 调用后 enable=1，状态机开始工作。
 *
 * @param vf : V/f 实例指针
 */
void open_loop_vf_start(open_loop_vf_t *vf);

/**
 * @brief 停止 V/F 控制
 *
 * 立即进入 STOP，v_out=0，freq=0。
 * 不做减速处理 (后续可扩展减速逻辑)。
 *
 * @param vf : V/f 实例指针
 */
void open_loop_vf_stop(open_loop_vf_t *vf);

/**
 * @brief V/f 单步更新
 *
 * 根据当前状态执行:
 *   STOP  : v_out=0, theta_e 保持
 *   ALIGN : 输出 align_voltage，累加时间，超时后进入 RAMP
 *   RAMP  : 频率爬坡，电压 = max(v_min, vf_ratio * freq)
 *   RUN   : 维持 target_freq，电压 = max(v_min, vf_ratio * freq)
 *
 * 供外部调用 InvPark(vd=v_out, vq=0, theta_e) 和 SVPWM。
 *
 * @param vf : V/f 实例指针
 * @param dt : 时间步长 (s)
 */
void open_loop_vf_step(open_loop_vf_t *vf, float dt);

/**
 * @brief 获取当前状态
 * @param vf : V/f 实例指针
 * @return 状态枚举值
 */
open_loop_vf_state_t open_loop_vf_get_state(const open_loop_vf_t *vf);

#endif
