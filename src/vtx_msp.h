/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VTX_MSP_H
#define VTX_MSP_H
#include <stdint.h>
#include <stdbool.h>

/* Optional helpers to query current state (for OSD, logs, etc.) */
typedef struct {
  uint8_t band;        // 1..5 (A/B/E/F/R), 0 if using frequency
  uint8_t channel;     // 1..8
  uint16_t frequency;  // MHz; if nonzero, overrides band/channel on SET
  uint8_t power;       // power index (0..N-1)
  uint8_t pitmode;     // 0/1
  uint8_t vtx_table_available;
  uint8_t configSet;
} vtx_config_t;

/* ---- VTX bands table: letter + 8-char name + 8 channel freqs (MHz) ---- */
#define VTX_CHANNEL_COUNT    8
#define VTX_CH_LABEL_COUNT   8
#define VTX_IS_FACTORY_BAND  1

typedef struct {
    char letter;                            /* 'A','B','E','F','R' */
    uint8_t band_name[VTX_CH_LABEL_COUNT];  /* shown in BF “Name”, exactly 8 bytes */
    uint16_t freq[VTX_CHANNEL_COUNT];       /* ch1..ch8, MHz */
} vtx_band_t;

const vtx_config_t* vtx_get_config(void);
const char* vtx_get_band_name(uint8_t band);
uint8_t vtx_get_band_count(void);
uint16_t vtx_get_power_mw(void);
uint16_t vtx_get_frequency(uint8_t band, uint8_t channel);
void vtx_set_pitmode(uint8_t pitmode);
void vtx_set_band_channel(int8_t band, uint8_t channel);
void vtx_set_power(int8_t power);

bool vtx_msp_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);
void vtx_msp_request_config(uint8_t owner);

void vtx_msp_clear_table_and_set_defaults(uint8_t owner);
void vtx_msp_push_power_table(uint8_t owner);
void vtx_msp_push_band_table(uint8_t owner);
void vtx_msp_push_calibration_table(uint8_t owner);
void vtx_msp_eeprom_write(uint8_t owner);

#endif //VTX_MSP_H
