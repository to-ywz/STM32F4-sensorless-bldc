/**
 * @file    svpwm_sector_test.h
 * @brief   SVPWM 六扇区手动测试模块。
 */

#ifndef SVPWM_SECTOR_TEST_H
#define SVPWM_SECTOR_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "svpwm.h"
#include <stdint.h>

typedef struct {
    svpwm_output_t *svpwm;
    volatile uint8_t requested_sector;
    volatile uint8_t active;
    uint8_t current_sector;
    pwm_output_t pwm;
} svpwm_sector_test_t;

int svpwm_sector_test_init(svpwm_sector_test_t *test,
                           svpwm_output_t *svpwm);

void svpwm_sector_test_set_sector(svpwm_sector_test_t *test,
                                  uint8_t sector);

void svpwm_sector_test_start(svpwm_sector_test_t *test,
                             uint8_t sector);

void svpwm_sector_test_stop(svpwm_sector_test_t *test);

uint8_t svpwm_sector_test_is_active(const svpwm_sector_test_t *test);

int svpwm_sector_test_step(svpwm_sector_test_t *test);

#ifdef __cplusplus
}
#endif

#endif /* SVPWM_SECTOR_TEST_H */
