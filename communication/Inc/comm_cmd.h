/**
 * @file    comm_cmd.h
 * @brief   串口文本命令解析模块。
 * @version 0.1.0
 * @date    2026-07-05
 */

#ifndef COMM_CMD_H
#define COMM_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "comm_uart.h"
#include <stdint.h>

#define COMM_CMD_LINE_SIZE  48U

typedef void (*comm_cmd_void_cb_t)(void *user);
typedef void (*comm_cmd_freq_cb_t)(void *user, float freq_hz);
typedef void (*comm_cmd_sector_cb_t)(void *user, uint8_t sector);

typedef struct {
    comm_uart_t        *uart;
    void               *user;
    comm_cmd_void_cb_t  start;
    comm_cmd_void_cb_t  stop;
    comm_cmd_freq_cb_t  set_freq;
    comm_cmd_sector_cb_t set_sector;
    float               freq_min_hz;
    float               freq_max_hz;
} comm_cmd_config_t;

typedef struct {
    comm_uart_t        *uart;
    void               *user;
    comm_cmd_void_cb_t  start;
    comm_cmd_void_cb_t  stop;
    comm_cmd_freq_cb_t  set_freq;
    comm_cmd_sector_cb_t set_sector;
    float               freq_min_hz;
    float               freq_max_hz;
    char                line[COMM_CMD_LINE_SIZE];
    uint16_t            line_len;
    int                 last_status;
} comm_cmd_t;

/**
 * @brief  初始化文本命令解析器。
 * @param  cmd    命令解析器对象。
 * @param  config 命令解析器配置。
 * @return 0 表示成功，负数表示失败。
 */
int comm_cmd_init(comm_cmd_t *cmd, const comm_cmd_config_t *config);

/**
 * @brief  轮询接收缓冲区并解析完整命令行。
 * @param  cmd 命令解析器对象。
 * @return 最近一次命令状态，0 表示成功或暂无命令，负数表示解析失败。
 */
int comm_cmd_poll(comm_cmd_t *cmd);

/**
 * @brief  获取最近一次命令状态。
 * @param  cmd 命令解析器对象。
 * @return 0 表示成功或暂无错误，负数表示最近一次解析错误。
 */
int comm_cmd_get_last_status(const comm_cmd_t *cmd);

#ifdef __cplusplus
}
#endif

#endif /* COMM_CMD_H */
