/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef RF_PA_H
#define RF_PA_H
#include <stdint.h>
#include <stdbool.h>

#define POWER_LABEL_LENGTH      3
#define RF_PA_PWR_OFF           0

typedef struct {
    uint16_t mW;
    uint8_t label[3];
    uint8_t rtcPA;
    uint16_t calibration[7];
    uint16_t detector[7];
} powerTable_t;

extern powerTable_t powerTable[];

uint8_t rf_pa_power_count(void);
void rf_pa_write_eeprom(uint8_t idx);
void rf_pa_init(void);
void rf_pa_enable(bool on);
uint16_t rf_pa_read_vdet_mv(void);
uint16_t rf_pa_get_vref_mv(void);
void rf_pa_set_vref_mv(uint16_t mv);
void rf_pa_set_calibration(uint16_t mv);
uint16_t rf_pa_set_power_level(uint8_t level);
void rf_pa_loop(void);

#endif //RF_PA_H
