/**
 * @file    comm_uart_stm32.h
 * @brief   STM32 HAL UART DMA 适配层。
 * @version 0.2.0
 * @date    2026-07-05
 */

#ifndef COMM_UART_STM32_H
#define COMM_UART_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_ringbuf.h"
#include "comm_uart.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    UART_HandleTypeDef *huart;
    comm_uart_t        *comm;

    uint8_t            *rx_dma_buf;
    uint16_t            rx_dma_size;
    comm_ringbuf_t      rx_ring;

    comm_ringbuf_t      tx_ring;
    uint8_t            *tx_dma_buf;
    uint16_t            tx_dma_size;
    uint16_t            tx_dma_len;
    volatile uint8_t    tx_busy;
} comm_uart_stm32_t;

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t            *rx_dma_buf;
    uint16_t            rx_dma_size;
    uint8_t            *rx_ring_buf;
    uint16_t            rx_ring_size;
    uint8_t            *tx_ring_buf;
    uint16_t            tx_ring_size;
    uint8_t            *tx_dma_buf;
    uint16_t            tx_dma_size;
} comm_uart_stm32_config_t;

/**
 * @brief  初始化 STM32 UART DMA 适配层。
 * @param  comm   通用通信对象。
 * @param  drv    STM32 UART 适配对象。
 * @param  config STM32 UART 适配配置。
 * @return 0 表示成功，负数表示失败。
 */
int comm_uart_stm32_init(comm_uart_t *comm,
                         comm_uart_stm32_t *drv,
                         const comm_uart_stm32_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* COMM_UART_STM32_H */
