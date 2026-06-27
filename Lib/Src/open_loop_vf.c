/**
 * @file open_loop_vf.c
 * @author blacksheep (blacksheep.208h@gmail.com)
 * @brief 开环 V/f 控制
 * @version 0.1
 * @date 2026-06-04
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

void open_loop_vf_init(open_loop_vf_t *vf, float target_freq, float accel, float vf_ratio, float v_max, float v_min)
{
    vf->freq     = 0.0f;
    vf->target_freq = target_freq;  // 只支持 0 或 freq_max
    vf->accel    = accel;
    vf->vf_ratio = vf_ratio;
    vf->v_max    = v_max;
    vf->v_min    = v_min;           // 最小电压限幅，用于低频保证启动
    vf->theta_e  = 0.0f;
    vf->v_out    = 0.0f;
}

void open_loop_vf_step(open_loop_vf_t *vf, float dt)
{
    /* 频率爬坡 */
    if (vf->freq < vf->target_freq) {
        /* 启动：频率从 0 升到 target_freq */
        vf->freq += vf->accel * dt;
        if (vf->freq > vf->target_freq)
            vf->freq = vf->target_freq;
    } else if (vf->freq > vf->target_freq) {
        /* 停止：频率从 target_freq 降到 0 */
        vf->freq -= vf->accel * dt;
        if (vf->freq < vf->target_freq)
            vf->freq = vf->target_freq;
    }

    /* 电压 = V/f 比 × 频率，限幅 */
    float v_temp = vf->vf_ratio * vf->freq;
    v_temp = fmaxf(v_temp, vf->v_min);
    v_temp = fminf(v_temp, vf->v_max);
    vf->v_out = v_temp;

    /* 电角度累加: θ += 2π × f × dt */
    /* TODO: 当前 angle wrap 只减/加一次 2π，高频或大 dt 下可能不够鲁棒。
     * 实际使用中（16kHz PWM，freq < 1000Hz）单步增量远小于 2π，暂时不会出问题。
     * 后续上真实 MCU 时，考虑改用定点数 Q16（uint16_t）表示角度，利用无符号整数自然溢出归一化。
     * 或者改用 while 循环处理大角度步进。 */
    vf->theta_e += 2.0f * PI * vf->freq * dt;
    if (vf->theta_e >= 2.0f * PI)
        vf->theta_e -= 2.0f * PI;
    if (vf->theta_e < 0.0f)
        vf->theta_e += 2.0f * PI;
}
