/**
 * @file    comm_uart.c
 * @brief   通用 UART 通信抽象层。
 * @version 0.2.0
 * @date    2026-07-05
 */

#include "comm_uart.h"
#include <stddef.h>

int comm_uart_init(comm_uart_t *comm, void *drv, const comm_uart_ops_t *ops)
{
    if (comm == NULL || drv == NULL || ops == NULL ||
        ops->start_rx == NULL || ops->send == NULL ||
        ops->read == NULL || ops->is_tx_busy == NULL) {
        return -1;
    }

    comm->drv   = drv;
    comm->ops   = ops;
    comm->rx_cb = NULL;

    return 0;
}

void comm_uart_set_rx_callback(comm_uart_t *comm, comm_uart_rx_cb_t cb)
{
    if (comm == NULL) {
        return;
    }

    comm->rx_cb = cb;
}

int comm_uart_start_rx(comm_uart_t *comm)
{
    if (comm == NULL || comm->ops == NULL || comm->ops->start_rx == NULL) {
        return -1;
    }

    return comm->ops->start_rx(comm);
}

int comm_uart_send(comm_uart_t *comm, const uint8_t *data, uint16_t len)
{
    if (comm == NULL || comm->ops == NULL || comm->ops->send == NULL) {
        return -1;
    }

    return comm->ops->send(comm, data, len);
}

uint16_t comm_uart_read(comm_uart_t *comm, uint8_t *data, uint16_t len)
{
    if (comm == NULL || comm->ops == NULL || comm->ops->read == NULL) {
        return 0U;
    }

    return comm->ops->read(comm, data, len);
}

uint8_t comm_uart_is_tx_busy(const comm_uart_t *comm)
{
    if (comm == NULL || comm->ops == NULL || comm->ops->is_tx_busy == NULL) {
        return 0U;
    }

    return comm->ops->is_tx_busy(comm);
}

void comm_uart_on_rx(comm_uart_t *comm, const uint8_t *data, uint16_t len)
{
    if (comm == NULL || data == NULL || len == 0U || comm->rx_cb == NULL) {
        return;
    }

    comm->rx_cb(data, len);
}
