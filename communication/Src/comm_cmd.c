/**
 * @file    comm_cmd.c
 * @brief   串口文本命令解析模块。
 * @version 0.1.0
 * @date    2026-07-05
 */

#include "comm_cmd.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define COMM_CMD_OK          0
#define COMM_CMD_ERR_ARG    -1
#define COMM_CMD_ERR_FULL   -2
#define COMM_CMD_ERR_CMD    -3
#define COMM_CMD_ERR_RANGE  -4

static int comm_cmd_is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

static char *comm_cmd_skip_space(char *str)
{
    while (*str != '\0' && comm_cmd_is_space(*str)) {
        str++;
    }

    return str;
}

static int comm_cmd_is_line_end(char ch)
{
    return ch == '\0' || ch == '\r' || ch == '\n';
}

static void comm_cmd_trim_tail(char *str)
{
    char *end;

    if (str == NULL || *str == '\0') {
        return;
    }

    end = str + strlen(str);
    while (end > str && comm_cmd_is_space(*(end - 1))) {
        end--;
    }
    *end = '\0';
}

static int comm_cmd_parse_freq(char *arg, float *freq_hz)
{
    char *end;
    float value;

    if (arg == NULL || freq_hz == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    arg = comm_cmd_skip_space(arg);
    value = strtof(arg, &end);
    if (end == arg) {
        return COMM_CMD_ERR_CMD;
    }

    end = comm_cmd_skip_space(end);
    if (!comm_cmd_is_line_end(*end)) {
        return COMM_CMD_ERR_CMD;
    }

    *freq_hz = value;

    return COMM_CMD_OK;
}

static int comm_cmd_parse_sector(char *arg, uint8_t *sector)
{
    char *end;
    unsigned long value;

    if (arg == NULL || sector == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    arg = comm_cmd_skip_space(arg);
    value = strtoul(arg, &end, 10);
    if (end == arg) {
        return COMM_CMD_ERR_CMD;
    }

    end = comm_cmd_skip_space(end);
    if (!comm_cmd_is_line_end(*end) || value > 6UL) {
        return COMM_CMD_ERR_RANGE;
    }

    *sector = (uint8_t)value;
    return COMM_CMD_OK;
}

static int comm_cmd_exec_line(comm_cmd_t *cmd)
{
    char *line;
    float freq_hz;
    uint8_t sector;

    if (cmd == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    cmd->line[cmd->line_len] = '\0';
    comm_cmd_trim_tail(cmd->line);
    line = comm_cmd_skip_space(cmd->line);

    if (*line == '\0') {
        return COMM_CMD_OK;
    }

    if (strcmp(line, "start") == 0) {
        if (cmd->start != NULL) {
            cmd->start(cmd->user);
        }
        return COMM_CMD_OK;
    }

    if (strcmp(line, "stop") == 0) {
        if (cmd->stop != NULL) {
            cmd->stop(cmd->user);
        }
        return COMM_CMD_OK;
    }

    if (strncmp(line, "freq", 4U) == 0 && comm_cmd_is_space(line[4])) {
        int ret = comm_cmd_parse_freq(&line[4], &freq_hz);
        if (ret < 0) {
            return ret;
        }

        if (freq_hz < cmd->freq_min_hz || freq_hz > cmd->freq_max_hz) {
            return COMM_CMD_ERR_RANGE;
        }

        if (cmd->set_freq != NULL) {
            cmd->set_freq(cmd->user, freq_hz);
        }
        return COMM_CMD_OK;
    }

    if (strncmp(line, "sector", 6U) == 0 && comm_cmd_is_space(line[6])) {
        int ret = comm_cmd_parse_sector(&line[6], &sector);
        if (ret < 0) {
            return ret;
        }

        if (cmd->set_sector != NULL) {
            cmd->set_sector(cmd->user, sector);
        }
        return COMM_CMD_OK;
    }

    return COMM_CMD_ERR_CMD;
}

int comm_cmd_init(comm_cmd_t *cmd, const comm_cmd_config_t *config)
{
    if (cmd == NULL || config == NULL || config->uart == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    cmd->uart        = config->uart;
    cmd->user        = config->user;
    cmd->start       = config->start;
    cmd->stop        = config->stop;
    cmd->set_freq    = config->set_freq;
    cmd->set_sector  = config->set_sector;
    cmd->freq_min_hz = config->freq_min_hz;
    cmd->freq_max_hz = config->freq_max_hz;
    cmd->line_len    = 0U;
    cmd->last_status = COMM_CMD_OK;
    memset(cmd->line, 0, sizeof(cmd->line));

    return COMM_CMD_OK;
}

int comm_cmd_poll(comm_cmd_t *cmd)
{
    uint8_t byte;

    if (cmd == NULL || cmd->uart == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    while (comm_uart_read(cmd->uart, &byte, 1U) == 1U) {
        if (byte == '\r' || byte == '\n') {
            if (cmd->line_len == 0U) {
                continue;
            }

            cmd->last_status = comm_cmd_exec_line(cmd);
            cmd->line_len = 0U;
            return cmd->last_status;
        }

        if (cmd->line_len >= (COMM_CMD_LINE_SIZE - 1U)) {
            cmd->line_len = 0U;
            cmd->last_status = COMM_CMD_ERR_FULL;
            return cmd->last_status;
        }

        cmd->line[cmd->line_len] = (char)byte;
        cmd->line_len++;
    }

    return cmd->last_status;
}

int comm_cmd_get_last_status(const comm_cmd_t *cmd)
{
    if (cmd == NULL) {
        return COMM_CMD_ERR_ARG;
    }

    return cmd->last_status;
}
