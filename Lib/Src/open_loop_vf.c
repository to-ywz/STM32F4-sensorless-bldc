/**
 * @file open_loop_vf.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief 开环 V/f 控制 (带状态机)
 * @version 0.2
 * @date 2026-06-29
 *
 * @par 版本记录
 *  - 0.1 初始版本，无状态机
 *  - 0.2 增加 STOP/ALIGN/RAMP/RUN 状态机
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <math.h>
#include "open_loop_vf.h"

#ifndef M_PI
    #define PI 3.14159265358979323846f
#else
    #define PI M_PI
#endif

void open_loop_vf_init(open_loop_vf_t *vf, float target_freq, float accel,
                       float vf_ratio, float v_max, float v_min)
{
    vf->state         = OPEN_LOOP_VF_STATE_STOP;
    vf->freq          = 0.0f;
    vf->target_freq   = target_freq;
    vf->accel         = accel;
    vf->vf_ratio      = vf_ratio;
    vf->v_max         = v_max;
    vf->v_min         = v_min;
    vf->align_voltage = 0.0f;
    vf->align_time    = 0.0f;
    vf->align_elapsed = 0.0f;
    vf->theta_e       = 0.0f;
    vf->v_out         = 0.0f;
    vf->enable        = 0;
}

void open_loop_vf_set_align(open_loop_vf_t *vf, float align_voltage, float align_time)
{
    if (vf == NULL) {
        return;
    }
    vf->align_voltage = align_voltage;
    vf->align_time    = align_time;
}

void open_loop_vf_start(open_loop_vf_t *vf)
{
    if (vf == NULL) {
        return;
    }

    vf->enable = 1;

    /* 有预定位配置时先进入 ALIGN，否则直接 RAMP */
    if (vf->align_time > 0.0f && vf->align_voltage > 0.0f) {
        vf->state         = OPEN_LOOP_VF_STATE_ALIGN;
        vf->align_elapsed = 0.0f;
        vf->theta_e       = 0.0f;     /* 预定位固定角度 */
        vf->v_out         = 0.0f;
    } else {
        vf->state = OPEN_LOOP_VF_STATE_RAMP;
        vf->freq  = 0.0f;
        vf->v_out = 0.0f;
    }
}

void open_loop_vf_stop(open_loop_vf_t *vf)
{
    if (vf == NULL) {
        return;
    }

    vf->enable = 0;
    vf->state  = OPEN_LOOP_VF_STATE_STOP;
    vf->freq   = 0.0f;
    vf->v_out  = 0.0f;
    /* theta_e 保持当前值，不做复位 */
}

void open_loop_vf_step(open_loop_vf_t *vf, float dt)
{
    if (vf == NULL) {
        return;
    }

    /* 外部关闭 enable 时强制回到 STOP */
    if (!vf->enable && vf->state != OPEN_LOOP_VF_STATE_STOP) {
        open_loop_vf_stop(vf);
        return;
    }

    switch (vf->state) {

    /* ---- STOP ---- */
    case OPEN_LOOP_VF_STATE_STOP:
        vf->v_out = 0.0f;
        vf->freq  = 0.0f;
        /* theta_e 保持，不输出电压矢量 */
        break;

    /* ---- ALIGN: 固定角度、固定电压 ---- */
    case OPEN_LOOP_VF_STATE_ALIGN:
        vf->v_out = vf->align_voltage;
        vf->freq  = 0.0f;
        /* theta_e 保持 0 (或 init 时设定的值) */
        vf->align_elapsed += dt;
        if (vf->align_elapsed >= vf->align_time) {
            vf->state = OPEN_LOOP_VF_STATE_RAMP;
            vf->freq  = 0.0f;
        }
        break;

    /* ---- RAMP: 频率爬坡 ---- */
    case OPEN_LOOP_VF_STATE_RAMP:
        vf->freq += vf->accel * dt;
        if (vf->freq >= vf->target_freq) {
            vf->freq  = vf->target_freq;
            vf->state = OPEN_LOOP_VF_STATE_RUN;
        }
        /* 低频补偿: v_out = max(v_min, vf_ratio * freq)
           v_min 是低频补偿电压，保证低频时有足够电压建立磁场 */
        {
            float v_temp = vf->vf_ratio * vf->freq;
            v_temp = fmaxf(v_temp, vf->v_min);
            v_temp = fminf(v_temp, vf->v_max);
            vf->v_out = v_temp;
        }
        /* 电角度累加 */
        vf->theta_e += 2.0f * PI * vf->freq * dt;
        if (vf->theta_e >= 2.0f * PI)
            vf->theta_e -= 2.0f * PI;
        if (vf->theta_e < 0.0f)
            vf->theta_e += 2.0f * PI;
        break;

    /* ---- RUN: 稳态运行 ---- */
    case OPEN_LOOP_VF_STATE_RUN:
        /* 频率保持 target_freq */
        vf->freq = vf->target_freq;
        {
            float v_temp = vf->vf_ratio * vf->freq;
            v_temp = fmaxf(v_temp, vf->v_min);
            v_temp = fminf(v_temp, vf->v_max);
            vf->v_out = v_temp;
        }
        vf->theta_e += 2.0f * PI * vf->freq * dt;
        if (vf->theta_e >= 2.0f * PI)
            vf->theta_e -= 2.0f * PI;
        if (vf->theta_e < 0.0f)
            vf->theta_e += 2.0f * PI;
        break;

    default:
        /* 异常状态，回到 STOP */
        open_loop_vf_stop(vf);
        break;
    }

    /* 最终安全检查 */
    if (!isfinite(vf->v_out))  vf->v_out = 0.0f;
    if (!isfinite(vf->freq))   vf->freq  = 0.0f;
    if (!isfinite(vf->theta_e)) vf->theta_e = 0.0f;
}

open_loop_vf_state_t open_loop_vf_get_state(const open_loop_vf_t *vf)
{
    if (vf == NULL) {
        return OPEN_LOOP_VF_STATE_STOP;
    }
    return vf->state;
}
