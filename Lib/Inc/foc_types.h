/**
 * @file    foc_types.h
 * @brief   FOC 库基础类型定义
 * @version 0.1.0
 * @date    2026-06-27
 */

#ifndef __FOC_TYPES_H
#define __FOC_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 基础数学类型 */
typedef int32_t  q15_t;      /* Q15 定点数 (1.15) */
typedef int32_t  q31_t;      /* Q31 定点数 (1.31) */
typedef float    f32_t;      /* 32位浮点 */

/* 二维向量 (alpha-beta 坐标系) */
typedef struct {
    f32_t alpha;
    f32_t beta;
} ab_t;

/* 二维向量 (d-q 坐标系) */
typedef struct {
    f32_t d;
    f32_t q;
} dq_t;

/* 三相值 */
typedef struct {
    f32_t a;
    f32_t b;
    f32_t c;
} phase3_t;

/* PWM 占空比 (比较值) */
typedef struct {
    uint32_t cmp_a;     /* A 相比较值 (CCR1) */
    uint32_t cmp_b;     /* B 相比较值 (CCR2) */
    uint32_t cmp_c;     /* C 相比较值 (CCR3) */
} svpwm_pwm_t;

/* 扇区编号 (1-6) */
typedef enum {
    SECTOR_1 = 1,
    SECTOR_2,
    SECTOR_3,
    SECTOR_4,
    SECTOR_5,
    SECTOR_6,
} svpwm_sector_t;

/* 坐标变换角度 */
typedef struct {
    f32_t sin_val;
    f32_t cos_val;
} sincos_t;

/* 电机参数 */
typedef struct {
    uint32_t pole_pairs;        /* 极对数 */
    f32_t   rs;                 /* 相电阻 (Ohm) */
    f32_t   ls;                 /* 相电感 (H) */
    f32_t   flux_linkage;       /* 磁链 (Wb) */
    f32_t   rated_current;      /* 额定电流 (A) */
    f32_t   max_speed;          /* 最大转速 (rad/s) */
} motor_params_t;

/* 控制器状态 */
typedef enum {
    CTRL_IDLE = 0,              /* 空闲 */
    CTRL_RUNNING,               /* 运行中 */
    CTRL_FAULT,                 /* 故障 */
    CTRL_STOPPING,              /* 停止中 */
} ctrl_state_t;

/* 故障标志 */
typedef enum {
    FAULT_NONE          = 0x0000,
    FAULT_OVERCURRENT   = 0x0001,   /* 过流 */
    FAULT_OVERVOLTAGE   = 0x0002,   /* 过压 */
    FAULT_UNDERVOLTAGE  = 0x0004,   /* 欠压 */
    FAULT_OVERTEMP      = 0x0008,   /* 过温 */
    FAULT_STALL         = 0x0010,   /* 堵转 */
    FAULT_HARDWARE      = 0x0020,   /* 硬件故障 */
} fault_flag_t;

#ifdef __cplusplus
}
#endif

#endif /* __FOC_TYPES_H */
