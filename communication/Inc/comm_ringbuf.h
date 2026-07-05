/**
 * @file    comm_ringbuf.h
 * @brief   通用字节环形缓冲区。
 * @version 0.1.0
 * @date    2026-07-05
 */

#ifndef COMM_RINGBUF_H
#define COMM_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint8_t           *buf;
    uint16_t           size;
    volatile uint16_t  head;
    volatile uint16_t  tail;
} comm_ringbuf_t;

/**
 * @brief  初始化环形缓冲区。
 * @param  rb   环形缓冲区对象。
 * @param  buf  外部提供的存储空间。
 * @param  size 存储空间大小，至少为 2。
 * @return 0 表示成功，负数表示失败。
 */
int comm_ringbuf_init(comm_ringbuf_t *rb, uint8_t *buf, uint16_t size);

/**
 * @brief 清空环形缓冲区。
 *
 * @note  本函数会同时修改 head 和 tail。若 ringbuffer 可能被中断、
 *        DMA 回调或主循环并发访问，调用者需要先停止并发访问，
 *        或在外部进入临界区后再调用。
 *
 * @param rb 环形缓冲区对象。
 */
void comm_ringbuf_reset(comm_ringbuf_t *rb);

/**
 * @brief  获取已存储字节数。
 * @param  rb 环形缓冲区对象。
 * @return 已存储字节数。
 */
uint16_t comm_ringbuf_available(const comm_ringbuf_t *rb);

/**
 * @brief  获取剩余可写字节数。
 * @param  rb 环形缓冲区对象。
 * @return 剩余可写字节数。
 */
uint16_t comm_ringbuf_free(const comm_ringbuf_t *rb);

/**
 * @brief  写入字节流。
 * @param  rb   环形缓冲区对象。
 * @param  data 待写入数据。
 * @param  len  待写入长度。
 * @return 实际写入长度。
 */
uint16_t comm_ringbuf_write(comm_ringbuf_t *rb,
                            const uint8_t *data,
                            uint16_t len);

/**
 * @brief  读取字节流。
 * @param  rb   环形缓冲区对象。
 * @param  data 输出缓冲区。
 * @param  len  期望读取长度。
 * @return 实际读取长度。
 */
uint16_t comm_ringbuf_read(comm_ringbuf_t *rb, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* COMM_RINGBUF_H */
