/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

#if defined(USE_VTX)

#include "rf_pa.h"
#include <stdbool.h>
#include <string.h>
#include "rtc6705.h"
#include "vtx_msp.h"
#include "flash.h"

#ifndef POWER_TABLE 
#define POWER_TABLE         POWER_TABLE_DEFAULT
#endif

static uint16_t g_vref_mv = 0;
float rf_detector_target = 0;
double rf_detector = 0;
static float pa_control_i = 0;
static float pa_control_last_deviation = 0;
powerTable_t powerTable[] = POWER_TABLE;

uint8_t rf_pa_power_count(void)
{
    return (sizeof(powerTable)/sizeof(powerTable[0]) - 1);
}

static inline void dac_ch2_write_mv(uint16_t mv)
{
    #ifdef PA_LIMIT
      if (mv > PA_LIMIT) {
        mv = PA_LIMIT;
      } 
    #endif
    uint32_t dac_raw = DAC12BIT_FROM_MV(mv);
    
    if (dac_raw > 4095u) dac_raw = 4095u;
    LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_2, dac_raw);
    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_2);

    g_vref_mv = mv;
}

void rf_pa_enable(bool on)
{
    if(on) {
      dac_ch2_write_mv(g_vref_mv);
      #ifdef PA_ON_GPIO_Port
      LL_GPIO_SetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
      #endif
    } else {
      #ifdef PA_ON_GPIO_Port
      LL_GPIO_ResetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
      #endif
      dac_ch2_write_mv(0u);
    }
    
}

void rf_pa_set_vref_mv(uint16_t mv)
{
    dac_ch2_write_mv(mv);
    //TRACE_DEBUG("SET PA mv %i\n",mv);
}

uint16_t rf_pa_get_vref_mv(void)
{
    return g_vref_mv;
}

uint16_t rf_pa_read_vdet_raw(void)
{
    return adc_read_raw(ADC_CH_PA_VDET);
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t get_detector_target(uint8_t level)
{
  uint16_t freq = vtx_get_config()->frequency;
  uint8_t i;
  uint8_t calIdx = 5;
  uint16_t retVal;

  if (freq < 5650) freq = 5650;
  if (freq > 5950) freq = 5950;

  TRACE_INFO("PA freq %i\n",freq);

  for (i = 0; i < 6; i++) {
    if (freq < powerTable[0].calibration[i + 1]) {
      calIdx = i;
      break;
    }
  }

  retVal = map(freq,  powerTable[0].calibration[calIdx],   powerTable[0].calibration[calIdx + 1], 
                      powerTable[level].detector[calIdx],  powerTable[level].detector[calIdx + 1]);

  return retVal;
}

uint16_t get_calibration_mV(uint8_t level)
{
  uint16_t freq = vtx_get_config()->frequency;
  uint8_t i;
  uint8_t calIdx = 5;
  uint16_t retVal;

  if (freq < 5650) freq = 5650;
  if (freq > 5950) freq = 5950;

  //freq = 5800;

  for (i = 0; i < 6; i++) {
    if (freq < powerTable[0].calibration[i + 1]) {
      calIdx = i;
      break;
    }
  }

  retVal = map(freq,  powerTable[0].calibration[calIdx],      powerTable[0].calibration[calIdx + 1], 
                      powerTable[level].calibration[calIdx],  powerTable[level].calibration[calIdx + 1]);

  return retVal;
}

void rf_pa_set_power_level(uint8_t level)
{
    uint16_t mv;

    if (!level || level > rf_pa_power_count()) {
      mv = 0;
      rf_detector_target = 0;
      rf_detector = 0;
      pa_control_i = 0;
      pa_control_last_deviation = 0;
      rf_pa_set_vref_mv(mv);
    } else {
      rf_detector_target = get_detector_target(level);
      if(rf_detector_target == 0) {
        mv = get_calibration_mV(level);
        rf_pa_set_vref_mv(mv);
      }
    }
}

void rf_pa_set_calibration(uint16_t mv)
{
    rf_detector_target = 0;  
    rf_pa_set_vref_mv(mv);
}

void rf_pa_read_eeprom(uint8_t idx) {
  flashBlock_t block;
  uint8_t* valBytes;

  valBytes = (uint8_t*)powerTable[idx].calibration;
  block.idx = idx<<2;
  if (eeprom_read(&block)) {
    TRACE_INFO("eeprom block %i read \n", block.idx);
    memcpy(&valBytes[0] ,&block.value, sizeof(block.value));
  }
  block.idx++;
  if (eeprom_read(&block)) {
    TRACE_INFO("eeprom block %i read \n", block.idx);
    memcpy(&valBytes[7] ,&block.value, sizeof(block.value));
  }
  valBytes = (uint8_t*)powerTable[idx].detector;
  block.idx++;
  if (eeprom_read(&block)) {
    TRACE_INFO("eeprom block %i read \n", block.idx);
    memcpy(&valBytes[0] ,&block.value, sizeof(block.value));
  }
  block.idx++;
  if (eeprom_read(&block)) {
    TRACE_INFO("eeprom block %i read \n", block.idx);
    memcpy(&valBytes[7] ,&block.value, sizeof(block.value));
  }
}

void rf_pa_write_eeprom(uint8_t idx) {
  flashBlock_t block;
  uint8_t* valBytes;

  valBytes = (uint8_t*)powerTable[idx].calibration;
  block.idx = idx<<2;
  memcpy(&block.value, &valBytes[0], sizeof(block.value));
  eeprom_write(&block);

  block.idx++;
  memcpy(&block.value, &valBytes[7], sizeof(block.value));
  eeprom_write(&block);

  valBytes = (uint8_t*)powerTable[idx].detector;
  block.idx++;
  memcpy(&block.value, &valBytes[0], sizeof(block.value));
  eeprom_write(&block);

  block.idx++;
  memcpy(&block.value, &valBytes[7], sizeof(block.value));
  eeprom_write(&block);

  eeprom_dump();
}

void rf_pa_init(void)
{
    g_vref_mv = 0;
    /* Enable DAC1 ch2 if not already enabled by user init */
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
    rf_pa_enable(false); // keep PA off at boot

    for (uint8_t idx = 1; idx <= rf_pa_power_count(); idx++) {
      rf_pa_read_eeprom(idx);
    }
}

void rf_pa_loop(void)
{
    static uint32_t last_detector_loop = 0;
    static uint32_t last_control_loop = 0;
    float pa_control_p;
    float pa_control_d;
    float pa_deviation;
    
    if ((HAL_GetTick() - last_detector_loop) >= 1) {
      rf_detector = rf_detector * 0.99 + rf_pa_read_vdet_raw() * 0.01;
      last_detector_loop = HAL_GetTick();
    }

    if (vtx_get_config()->pitmode) {
      pa_control_i = 0;

    } else if (rf_detector_target) {
      if ((HAL_GetTick() - last_control_loop) >= 5)  {
        pa_deviation = rf_detector_target - rf_detector;
        
        pa_control_p = pa_deviation * PA_CONTROL_Kp;
        pa_control_i += pa_deviation * PA_CONTROL_Ki;
        pa_control_d = (pa_control_last_deviation - pa_deviation) * PA_CONTROL_Kd;

        rf_pa_set_vref_mv(MAX(0,PA_CONTROL_OFFSET_MV + pa_control_i + pa_control_p + pa_control_d));
        last_control_loop = HAL_GetTick();
        pa_control_last_deviation  = pa_deviation;
      }
    }

    #if 0
    static uint32_t debug_tick = 0;
    if ((HAL_GetTick() - debug_tick) >= 1000) {
      TRACE_INFO("RF detect %.0f target %.0f  vref %i  i %f\n", rf_detector, rf_detector_target, g_vref_mv, control_i );
      debug_tick = HAL_GetTick();
    }
    #endif
}
#endif