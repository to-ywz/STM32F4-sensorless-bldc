/**
 * @file    comm_uart_stm32.c
 * @brief   STM32 HAL UART DMA 适配层。
 * @version 0.2.0
 * @date    2026-07-05
 */

#include "comm_uart_stm32.h"
#include <stddef.h>

static comm_uart_stm32_t *g_uart_stm32;

static int comm_uart_stm32_kick_tx(comm_uart_stm32_t *drv)
{
    HAL_StatusTypeDef status;
    uint16_t len;

    if (drv == NULL || drv->huart == NULL || drv->tx_dma_buf == NULL ||
        drv->tx_dma_size == 0U || drv->tx_busy) {
        return -1;
    }

    len = comm_ringbuf_read(&drv->tx_ring, drv->tx_dma_buf, drv->tx_dma_size);
    if (len == 0U) {
        return 0;
    }

    drv->tx_busy = 1U;
    drv->tx_dma_len = len;

    status = HAL_UART_Transmit_DMA(drv->huart, drv->tx_dma_buf, len);
    if (status != HAL_OK) {
        drv->tx_busy = 0U;
        drv->tx_dma_len = 0U;
        return -2;
    }

    return 0;
}

static int comm_uart_stm32_start_rx(comm_uart_t *comm)
{
    HAL_StatusTypeDef status;
    comm_uart_stm32_t *drv;

    if (comm == NULL || comm->drv == NULL) {
        return -1;
    }

    drv = (comm_uart_stm32_t *)comm->drv;
    if (drv->huart == NULL || drv->rx_dma_buf == NULL ||
        drv->rx_dma_size == 0U) {
        return -1;
    }

    status = HAL_UARTEx_ReceiveToIdle_DMA(drv->huart,
                                          drv->rx_dma_buf,
                                          drv->rx_dma_size);
    if (status != HAL_OK) {
        return -2;
    }

    /*
     * 命令帧只需要 IDLE 事件。关闭半传输中断，避免解析层收到
     * 没有业务意义的半包通知。
     */
    if (drv->huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(drv->huart->hdmarx, DMA_IT_HT);
    }

    return 0;
}

static int comm_uart_stm32_send(comm_uart_t *comm,
                                const uint8_t *data,
                                uint16_t len)
{
    comm_uart_stm32_t *drv;
    uint16_t written;

    if (comm == NULL || comm->drv == NULL || data == NULL || len == 0U) {
        return -1;
    }

    drv = (comm_uart_stm32_t *)comm->drv;

    if (comm_ringbuf_free(&drv->tx_ring) < len) {
        return -2;
    }

    written = comm_ringbuf_write(&drv->tx_ring, data, len);
    if (written != len) {
        return -3;
    }

    (void)comm_uart_stm32_kick_tx(drv);

    return 0;
}

static uint16_t comm_uart_stm32_read(comm_uart_t *comm,
                                     uint8_t *data,
                                     uint16_t len)
{
    comm_uart_stm32_t *drv;

    if (comm == NULL || comm->drv == NULL || data == NULL || len == 0U) {
        return 0U;
    }

    drv = (comm_uart_stm32_t *)comm->drv;
    return comm_ringbuf_read(&drv->rx_ring, data, len);
}

static uint8_t comm_uart_stm32_is_tx_busy(const comm_uart_t *comm)
{
    const comm_uart_stm32_t *drv;

    if (comm == NULL || comm->drv == NULL) {
        return 0U;
    }

    drv = (const comm_uart_stm32_t *)comm->drv;
    return drv->tx_busy ? 1U : 0U;
}

static const comm_uart_ops_t g_uart_stm32_ops = {
    comm_uart_stm32_start_rx,
    comm_uart_stm32_send,
    comm_uart_stm32_read,
    comm_uart_stm32_is_tx_busy,
};

int comm_uart_stm32_init(comm_uart_t *comm,
                         comm_uart_stm32_t *drv,
                         const comm_uart_stm32_config_t *config)
{
    int ret;

    if (comm == NULL || drv == NULL || config == NULL ||
        config->huart == NULL ||
        config->rx_dma_buf == NULL || config->rx_dma_size == 0U ||
        config->rx_ring_buf == NULL || config->rx_ring_size < 2U ||
        config->tx_ring_buf == NULL || config->tx_ring_size < 2U ||
        config->tx_dma_buf == NULL || config->tx_dma_size == 0U) {
        return -1;
    }

    drv->huart       = config->huart;
    drv->comm        = comm;
    drv->rx_dma_buf  = config->rx_dma_buf;
    drv->rx_dma_size = config->rx_dma_size;
    drv->tx_dma_buf  = config->tx_dma_buf;
    drv->tx_dma_size = config->tx_dma_size;
    drv->tx_dma_len  = 0U;
    drv->tx_busy     = 0U;

    ret = comm_ringbuf_init(&drv->rx_ring,
                            config->rx_ring_buf,
                            config->rx_ring_size);
    if (ret < 0) {
        return ret;
    }

    ret = comm_ringbuf_init(&drv->tx_ring,
                            config->tx_ring_buf,
                            config->tx_ring_size);
    if (ret < 0) {
        return ret;
    }

    ret = comm_uart_init(comm, drv, &g_uart_stm32_ops);
    if (ret < 0) {
        return ret;
    }

    g_uart_stm32 = drv;

    return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (g_uart_stm32 == NULL || huart != g_uart_stm32->huart) {
        return;
    }

    g_uart_stm32->tx_busy = 0U;
    g_uart_stm32->tx_dma_len = 0U;
    (void)comm_uart_stm32_kick_tx(g_uart_stm32);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (g_uart_stm32 == NULL || huart != g_uart_stm32->huart) {
        return;
    }

    g_uart_stm32->tx_busy = 0U;
    g_uart_stm32->tx_dma_len = 0U;
    (void)comm_uart_start_rx(g_uart_stm32->comm);
    (void)comm_uart_stm32_kick_tx(g_uart_stm32);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    uint16_t written;

    if (g_uart_stm32 == NULL || huart != g_uart_stm32->huart) {
        return;
    }

    if (size > 0U && size <= g_uart_stm32->rx_dma_size) {
        written = comm_ringbuf_write(&g_uart_stm32->rx_ring,
                                     g_uart_stm32->rx_dma_buf,
                                     size);
        if (written > 0U) {
            comm_uart_on_rx(g_uart_stm32->comm,
                            g_uart_stm32->rx_dma_buf,
                            written);
        }
    }

    (void)comm_uart_start_rx(g_uart_stm32->comm);
}
