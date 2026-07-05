#include "comm_cmd.h"
#include "comm_ringbuf.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    comm_ringbuf_t rx;
    uint8_t        rx_buf[128];
    int            start_count;
    int            stop_count;
    float          freq_hz;
} fake_uart_t;

static int fake_start_rx(comm_uart_t *comm)
{
    (void)comm;
    return 0;
}

static int fake_send(comm_uart_t *comm, const uint8_t *data, uint16_t len)
{
    (void)comm;
    (void)data;
    (void)len;
    return 0;
}

static uint16_t fake_read(comm_uart_t *comm, uint8_t *data, uint16_t len)
{
    fake_uart_t *fake = (fake_uart_t *)comm->drv;
    return comm_ringbuf_read(&fake->rx, data, len);
}

static uint8_t fake_busy(const comm_uart_t *comm)
{
    (void)comm;
    return 0U;
}

static const comm_uart_ops_t fake_ops = {
    fake_start_rx,
    fake_send,
    fake_read,
    fake_busy,
};

static void fake_push(fake_uart_t *fake, const char *text)
{
    (void)comm_ringbuf_write(&fake->rx,
                             (const uint8_t *)text,
                             (uint16_t)strlen(text));
}

static void on_start(void *user)
{
    fake_uart_t *fake = (fake_uart_t *)user;
    fake->start_count++;
}

static void on_stop(void *user)
{
    fake_uart_t *fake = (fake_uart_t *)user;
    fake->stop_count++;
}

static void on_freq(void *user, float freq_hz)
{
    fake_uart_t *fake = (fake_uart_t *)user;
    fake->freq_hz = freq_hz;
}

int main(void)
{
    fake_uart_t fake;
    comm_uart_t uart;
    comm_cmd_t cmd;

    memset(&fake, 0, sizeof(fake));
    assert(comm_ringbuf_init(&fake.rx, fake.rx_buf, sizeof(fake.rx_buf)) == 0);
    assert(comm_uart_init(&uart, &fake, &fake_ops) == 0);

    comm_cmd_config_t config = {
        .uart        = &uart,
        .user        = &fake,
        .start       = on_start,
        .stop        = on_stop,
        .set_freq    = on_freq,
        .freq_min_hz = 0.0f,
        .freq_max_hz = 30.0f,
    };
    assert(comm_cmd_init(&cmd, &config) == 0);

    fake_push(&fake, "start\r\n");
    assert(comm_cmd_poll(&cmd) == 0);
    assert(fake.start_count == 1);

    fake_push(&fake, "freq 12.5\n");
    assert(comm_cmd_poll(&cmd) == 0);
    assert(fake.freq_hz > 12.49f && fake.freq_hz < 12.51f);

    fake_push(&fake, "stop\n");
    assert(comm_cmd_poll(&cmd) == 0);
    assert(fake.stop_count == 1);

    fake_push(&fake, "freq 31\n");
    assert(comm_cmd_poll(&cmd) < 0);

    return 0;
}
