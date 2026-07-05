/**
 * @file    comm_vofa.c
 * @brief   VOFA+ JustFloat 帧输出。
 * @version 0.1.0
 * @date    2026-07-05
 */

#include "comm_vofa.h"
#include <string.h>
#include <stddef.h>

#define COMM_VOFA_TAIL0  0x00U
#define COMM_VOFA_TAIL1  0x00U
#define COMM_VOFA_TAIL2  0x80U
#define COMM_VOFA_TAIL3  0x7FU

void comm_vofa_init(comm_vofa_t *vofa, comm_uart_t *uart)
{
    if (vofa == NULL) {
        return;
    }

    vofa->uart = uart;
}

int comm_vofa_send_justfloat(comm_vofa_t *vofa,
                             const float *values,
                             uint8_t count)
{
    uint16_t payload_len;
    uint16_t frame_len;
    uint8_t *tail;

    if (vofa == NULL || vofa->uart == NULL || values == NULL || count == 0U) {
        return -1;
    }

    if (count > COMM_VOFA_MAX_CHANNELS) {
        return -2;
    }

    payload_len = (uint16_t)count * (uint16_t)sizeof(float);
    frame_len = payload_len + 4U;

    memcpy(vofa->tx_buf, values, payload_len);

    tail = &vofa->tx_buf[payload_len];
    tail[0] = COMM_VOFA_TAIL0;
    tail[1] = COMM_VOFA_TAIL1;
    tail[2] = COMM_VOFA_TAIL2;
    tail[3] = COMM_VOFA_TAIL3;

    return comm_uart_send(vofa->uart, vofa->tx_buf, frame_len);
}
