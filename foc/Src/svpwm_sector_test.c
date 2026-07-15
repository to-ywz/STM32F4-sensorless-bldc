/**
 * @file    svpwm_sector_test.c
 * @brief   SVPWM 六扇区手动测试模块。
 */

#include "svpwm_sector_test.h"

static const float g_sector_vectors[6][2] = {
    { 0.8660254f,  0.5f },
    { 0.0f,         1.0f },
    {-0.8660254f,  0.5f },
    {-0.8660254f, -0.5f },
    { 0.0f,        -1.0f },
    { 0.8660254f, -0.5f },
};

int svpwm_sector_test_init(svpwm_sector_test_t *test,
                           svpwm_output_t *svpwm)
{
    if (test == NULL || svpwm == NULL) {
        return -1;
    }

    test->svpwm = svpwm;
    test->requested_sector = 0U;
    test->active = 0U;
    test->current_sector = 0U;
    test->pwm = svpwm->pwm;

    return 0;
}

void svpwm_sector_test_set_sector(svpwm_sector_test_t *test,
                                  uint8_t sector)
{
    if (test == NULL) {
        return;
    }

    if (sector >= 1U && sector <= 6U) {
        svpwm_sector_test_start(test, sector);
    } else if (sector == 0U) {
        svpwm_sector_test_stop(test);
    }
}

void svpwm_sector_test_start(svpwm_sector_test_t *test,
                             uint8_t sector)
{
    if (test == NULL || sector < 1U || sector > 6U) {
        return;
    }

    test->requested_sector = sector;
    test->active = 1U;
}

void svpwm_sector_test_stop(svpwm_sector_test_t *test)
{
    if (test == NULL) {
        return;
    }

    test->requested_sector = 0U;
    test->active = 0U;
    test->current_sector = 0U;
}

uint8_t svpwm_sector_test_is_active(const svpwm_sector_test_t *test)
{
    return (test != NULL) ? test->active : 0U;
}

int svpwm_sector_test_step(svpwm_sector_test_t *test)
{
    uint8_t sector;

    if (test == NULL || test->svpwm == NULL || test->active == 0U) {
        return 0;
    }

    sector = test->requested_sector;
    if (sector < 1U || sector > 6U) {
        return 0;
    }

    svpwm_update(test->svpwm,
                 g_sector_vectors[sector - 1U][0],
                 g_sector_vectors[sector - 1U][1]);
    test->pwm = test->svpwm->pwm;
    test->current_sector = test->svpwm->sector;
    test->requested_sector = 0U;

    /* active 保持为 1，用于持续保持当前测试扇区，直到 sector 0。 */

    return 1;
}
