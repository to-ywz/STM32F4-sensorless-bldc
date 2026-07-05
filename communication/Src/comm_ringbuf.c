/**
 * @file    comm_ringbuf.c
 * @brief   通用字节环形缓冲区。
 * @version 0.1.0
 * @date    2026-07-05
 */

#include "comm_ringbuf.h"
#include <stddef.h>

static uint16_t comm_ringbuf_next(const comm_ringbuf_t *rb, uint16_t pos)
{
    pos++;
    if (pos >= rb->size) {
        pos = 0U;
    }

    return pos;
}

int comm_ringbuf_init(comm_ringbuf_t *rb, uint8_t *buf, uint16_t size)
{
    if (rb == NULL || buf == NULL || size < 2U) {
        return -1;
    }

    rb->buf  = buf;
    rb->size = size;
    rb->head = 0U;
    rb->tail = 0U;

    return 0;
}

void comm_ringbuf_reset(comm_ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }

    rb->head = 0U;
    rb->tail = 0U;
}

uint16_t comm_ringbuf_available(const comm_ringbuf_t *rb)
{
    uint16_t head;
    uint16_t tail;

    if (rb == NULL || rb->buf == NULL || rb->size == 0U) {
        return 0U;
    }

    head = rb->head;
    tail = rb->tail;

    if (head >= tail) {
        return (uint16_t)(head - tail);
    }

    return (uint16_t)(rb->size - tail + head);
}

uint16_t comm_ringbuf_free(const comm_ringbuf_t *rb)
{
    if (rb == NULL || rb->buf == NULL || rb->size < 2U) {
        return 0U;
    }

    /*
     * 保留一个空字节区分满/空状态，所以最大可用容量为 size - 1。
     */
    return (uint16_t)(rb->size - 1U - comm_ringbuf_available(rb));
}

uint16_t comm_ringbuf_write(comm_ringbuf_t *rb,
                            const uint8_t *data,
                            uint16_t len)
{
    uint16_t written = 0U;

    if (rb == NULL || rb->buf == NULL || data == NULL || len == 0U) {
        return 0U;
    }

    while (written < len && comm_ringbuf_free(rb) > 0U) {
        rb->buf[rb->head] = data[written];
        rb->head = comm_ringbuf_next(rb, rb->head);
        written++;
    }

    return written;
}

uint16_t comm_ringbuf_read(comm_ringbuf_t *rb, uint8_t *data, uint16_t len)
{
    uint16_t read_len = 0U;

    if (rb == NULL || rb->buf == NULL || data == NULL || len == 0U) {
        return 0U;
    }

    while (read_len < len && comm_ringbuf_available(rb) > 0U) {
        data[read_len] = rb->buf[rb->tail];
        rb->tail = comm_ringbuf_next(rb, rb->tail);
        read_len++;
    }

    return read_len;
}
