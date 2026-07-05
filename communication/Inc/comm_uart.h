/**
 * @file    comm_uart.h
 * @brief   通用 UART 通信抽象层。
 * @version 0.2.0
 * @date    2026-07-05
 */

#ifndef COMM_UART_H
#define COMM_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct comm_uart;

typedef void (*comm_uart_rx_cb_t)(const uint8_t *data, uint16_t len);
typedef int (*comm_uart_start_rx_fn_t)(struct comm_uart *comm);
typedef int (*comm_uart_send_fn_t)(struct comm_uart *comm,
                                   const uint8_t *data,
                                   uint16_t len);
typedef uint16_t (*comm_uart_read_fn_t)(struct comm_uart *comm,
                                        uint8_t *data,
                                        uint16_t len);
typedef uint8_t (*comm_uart_tx_busy_fn_t)(const struct comm_uart *comm);

typedef struct {
    comm_uart_start_rx_fn_t start_rx;
    comm_uart_send_fn_t     send;
    comm_uart_read_fn_t     read;
    comm_uart_tx_busy_fn_t  is_tx_busy;
} comm_uart_ops_t;

typedef struct comm_uart {
    void                    *drv;
    const comm_uart_ops_t   *ops;
    comm_uart_rx_cb_t        rx_cb;
} comm_uart_t;

/*
 * TODO: 后续收敛为 opaque handle + 用户静态 storage。
 * 当前阶段暂不使用动态内存，先保留显式对象，便于调试通信链路。
 */

/**
 * @brief  初始化通用 UART 通信对象。
 * @param  comm 通信对象。
 * @param  drv  底层驱动私有对象，由适配层解释。
 * @param  ops  底层驱动操作表。
 * @return 0 表示成功，负数表示失败。
 */
int comm_uart_init(comm_uart_t *comm, void *drv, const comm_uart_ops_t *ops);

/**
 * @brief 设置接收回调。
 * @param comm 通信对象。
 * @param cb   接收回调，通常在串口接收事件上下文调用。
 */
void comm_uart_set_rx_callback(comm_uart_t *comm, comm_uart_rx_cb_t cb);

/**
 * @brief  启动接收。
 * @param  comm 通信对象。
 * @return 0 表示成功，负数表示失败。
 */
int comm_uart_start_rx(comm_uart_t *comm);

/**
 * @brief  发送字节流。
 * @param  comm 通信对象。
 * @param  data 待发送数据。
 * @param  len  数据长度。
 * @return 0 表示成功，负数表示忙或失败。
 */
int comm_uart_send(comm_uart_t *comm, const uint8_t *data, uint16_t len);

/**
 * @brief  从接收缓冲区读取字节流。
 * @param  comm 通信对象。
 * @param  data 输出缓冲区。
 * @param  len  期望读取长度。
 * @return 实际读取长度。
 */
uint16_t comm_uart_read(comm_uart_t *comm, uint8_t *data, uint16_t len);

/**
 * @brief  查询发送是否忙。
 * @param  comm 通信对象。
 * @return 1 表示忙，0 表示空闲。
 */
uint8_t comm_uart_is_tx_busy(const comm_uart_t *comm);

/**
 * @brief 通知上层收到一帧数据。
 * @param comm 通信对象。
 * @param data 接收数据。
 * @param len  接收长度。
 */
void comm_uart_on_rx(comm_uart_t *comm, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* COMM_UART_H */
