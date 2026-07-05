/**
 * @file    comm_ringbuf_test.c
 * @brief   comm_ringbuf PC 端单元测试。
 * @date    2026-07-05
 *
 * 编译: gcc -std=c99 -Wall -I communication\Inc -o Tests\comm_ringbuf_test.exe Tests\comm_ringbuf_test.c communication\Src\comm_ringbuf.c
 * 返回 0 = 全部通过, 非 0 = 失败数
 */

#include "../communication/Inc/comm_ringbuf.h"
#include <stdint.h>

static int g_fail;

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { g_fail++; return; } \
} while (0)

static void test_init(void)
{
    comm_ringbuf_t rb;
    uint8_t buf[4];

    ASSERT_EQ(comm_ringbuf_init(&rb, buf, sizeof(buf)), 0);
    ASSERT_EQ(comm_ringbuf_available(&rb), 0U);
    ASSERT_EQ(comm_ringbuf_free(&rb), 3U);
}

static void test_full_and_empty(void)
{
    comm_ringbuf_t rb;
    uint8_t buf[4];
    uint8_t in[4] = {1U, 2U, 3U, 4U};
    uint8_t out[4] = {0U};

    ASSERT_EQ(comm_ringbuf_init(&rb, buf, sizeof(buf)), 0);
    ASSERT_EQ(comm_ringbuf_write(&rb, in, sizeof(in)), 3U);
    ASSERT_EQ(comm_ringbuf_free(&rb), 0U);
    ASSERT_EQ(comm_ringbuf_available(&rb), 3U);

    ASSERT_EQ(comm_ringbuf_read(&rb, out, sizeof(out)), 3U);
    ASSERT_EQ(out[0], 1U);
    ASSERT_EQ(out[1], 2U);
    ASSERT_EQ(out[2], 3U);
    ASSERT_EQ(comm_ringbuf_available(&rb), 0U);
}

static void test_wrap(void)
{
    comm_ringbuf_t rb;
    uint8_t buf[5];
    uint8_t in1[3] = {1U, 2U, 3U};
    uint8_t in2[3] = {4U, 5U, 6U};
    uint8_t out[4] = {0U};

    ASSERT_EQ(comm_ringbuf_init(&rb, buf, sizeof(buf)), 0);
    ASSERT_EQ(comm_ringbuf_write(&rb, in1, sizeof(in1)), 3U);
    ASSERT_EQ(comm_ringbuf_read(&rb, out, 2U), 2U);
    ASSERT_EQ(out[0], 1U);
    ASSERT_EQ(out[1], 2U);
    ASSERT_EQ(comm_ringbuf_write(&rb, in2, sizeof(in2)), 3U);
    ASSERT_EQ(comm_ringbuf_available(&rb), 4U);

    ASSERT_EQ(comm_ringbuf_read(&rb, out, sizeof(out)), 4U);
    ASSERT_EQ(out[0], 3U);
    ASSERT_EQ(out[1], 4U);
    ASSERT_EQ(out[2], 5U);
    ASSERT_EQ(out[3], 6U);
}

int main(void)
{
    test_init();
    test_full_and_empty();
    test_wrap();

    return g_fail;
}
