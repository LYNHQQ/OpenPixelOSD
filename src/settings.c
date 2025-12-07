/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "flash.h"
#include "main.h"
#include "video_overlay.h"
#include "vtx_msp.h"

#include <stdbool.h>
#include <string.h>

#define BLOCK_SETTINGS      31

typedef struct {
  uint8_t idx;
  uint8_t displayport_enabled;
  uint8_t band;
  uint8_t channel;
  uint8_t power;
  uint8_t spare;
  uint16_t frequency;
} setting_t;

void settings_load(void) {
  setting_t setting;
  vtx_config_t *vtx_config = (vtx_config_t*)vtx_get_config();

  setting.idx = BLOCK_SETTINGS;
  if(eeprom_read((flashBlock_t*) &setting)) {
    displayport_enabled = setting.displayport_enabled;
    vtx_config->band = setting.band;
    vtx_config->channel = setting.channel;
    vtx_config->power = setting.power;
    vtx_config->frequency = setting.frequency;
  }

}

void settings_save(void) {
  setting_t setting;
  vtx_config_t *vtx_config = (vtx_config_t*)vtx_get_config();

  setting.idx = BLOCK_SETTINGS;
  setting.displayport_enabled = displayport_enabled;
  setting.band = vtx_config->band;
  setting.channel = vtx_config->channel;
  setting.power = vtx_config->power;
  setting.frequency = vtx_config->frequency;

  eeprom_write((flashBlock_t*) &setting);
  eeprom_save();
}

