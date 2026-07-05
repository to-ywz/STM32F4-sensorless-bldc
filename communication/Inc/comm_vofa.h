/**
 * @file    comm_vofa.h
 * @brief   VOFA+ JustFloat 帧输出。
 * @version 0.1.0
 * @date    2026-07-05
 */

#ifndef COMM_VOFA_H
#define COMM_VOFA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_uart.h"
#include <stdint.h>

#define COMM_VOFA_MAX_CHANNELS  20U

typedef struct {
    comm_uart_t *uart;
    uint8_t      tx_buf[COMM_VOFA_MAX_CHANNELS * sizeof(float) + 4U];
} comm_vofa_t;

/**
 * @brief 初始化 VOFA+ JustFloat 输出对象。
 * @param vofa VOFA 对象。
 * @param uart 通用 UART 通信对象。
 */
void comm_vofa_init(comm_vofa_t *vofa, comm_uart_t *uart);

/**
 * @brief  发送 VOFA+ JustFloat 帧。
 * @param  vofa   VOFA 对象。
 * @param  values float 通道数组。
 * @param  count  float 通道数量，最大 COMM_VOFA_MAX_CHANNELS。
 * @return 0 表示成功，负数表示忙或失败。
 */
int comm_vofa_send_justfloat(comm_vofa_t *vofa,
                             const float *values,
                             uint8_t count);

#ifdef __cplusplus
}
#endif

#endif /* COMM_VOFA_H */
