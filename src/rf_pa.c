/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "rf_pa.h"
#include "main.h"
#include <stdbool.h>
#include "rtc6705.h"
#include  "vtx_msp.h"


powerTable_t powerTable[] = { {0,  {' ', ' ', '0'}, RTC6705_PA_3dBm,  { 5650, 5700, 5750, 5800, 5850, 5900, 5950 },     //cal frequency
                                                                      { 0,    0,    0,    0,    0,    0,    0    }},
                              {1,  {' ', ' ', '1'}, RTC6705_PA_3dBm,  { 1931, 1951, 1972, 1995, 2019, 2043, 2071 },
                                                                      { 172,  142,  115,  109,  112,  110,  117  }},
                              {10, {' ', '1', '0'}, RTC6705_PA_7dBm,  { 1976, 1997, 2018, 2040, 2064, 2090, 2121 }, 
                                                                      { 1174, 1026, 856,  783,  767,  771,  824  }}, 
                              {25, {' ', '2', '5'}, RTC6705_PA_7dBm,  { 2016, 2033, 2054, 2075, 2100, 2128, 2163 }, 
                                                                      { 2306, 1954, 1708, 1542, 1500, 1512, 1612 }}, 
                              {50, {' ', '5', '0'}, RTC6705_PA_11dBm, { 2050, 2064, 2083, 2103, 2129, 2159, 2198 }, 
                                                                      { 3274, 3206, 2804, 2514, 2432, 2469, 2595 }},
                            };


#define NUM_PWR (sizeof(powerTable)/sizeof(powerTable[0]) - 1)

static uint16_t g_vref_mv = 0;
float rf_detector_target = 0;
double rf_detector = 0;
 
uint8_t rf_pa_power_count(void)
{
    return NUM_PWR;
}

static inline void dac_ch2_write_mv(uint16_t mv)
{
    uint32_t dac_raw = DAC12BIT_FROM_MV(mv);
    #ifdef PA_LIMIT
      if (dac_raw > PA_LIMIT) {
        dac_raw > PA_LIMIT;
      } 
    #endif
    if (dac_raw > 4095u) dac_raw = 4095u;
    LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_2, dac_raw);
    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_2);

    g_vref_mv = mv;
}

void rf_pa_init(void)
{
    g_vref_mv = 0;
    /* Enable DAC1 ch2 if not already enabled by user init */
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);
    rf_pa_enable(false); // keep PA off at boot
}

void rf_pa_enable(bool on)
{
    dac_ch2_write_mv(on ? g_vref_mv : 0u);
}

void rf_pa_set_vref_mv(uint16_t mv)
{
    dac_ch2_write_mv(mv);
    TRACE_INFO("SET PA mv %i\n",mv);
}

uint16_t rf_pa_get_vref_mv(void)
{
    return g_vref_mv;
}

uint16_t rf_pa_read_vdet_mv(void)
{
    return adc_read_mv(ADC_CH_PA_VDET);
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

uint16_t rf_pa_set_power_level(rf_pa_power_t level)
{
    uint16_t mv;

    if (!level || level >= RF_PA_PWR_COUNT) {
      mv = 0;
      rf_detector_target = 0;
      rf_detector = 0;
    } else {
      mv = get_calibration_mV(level);
      rf_detector_target = get_detector_target(level);
      rf_detector = rf_detector_target;
    }

    rf_pa_set_vref_mv(mv);
    return mv;
}

void rf_pa_set_calibration(uint16_t mv)
{
    rf_detector_target = 0;  
    rf_pa_set_vref_mv(mv);
}

void rf_pa_loop(void)
{
    static uint32_t last_tick = 0;
    static uint32_t last_detector_change = 0;

    if ((HAL_GetTick() - last_tick) >= 1) {
      rf_detector = rf_detector * 0.999 + rf_pa_read_vdet_mv() * 0.001;
      last_tick = HAL_GetTick();
    }

    if (rf_detector_target && !vtx_get_config()->pitmode) {
      if ((HAL_GetTick() - last_detector_change) >= 1000)  {

        if (rf_detector < rf_detector_target * 0.985) {
          rf_pa_set_vref_mv(g_vref_mv + 1);
          last_detector_change = HAL_GetTick();
          TRACE_INFO("detector change %i \n", g_vref_mv);
        } else if (rf_detector > rf_detector_target * 1.02) {
          rf_pa_set_vref_mv(g_vref_mv - 1);
          last_detector_change = HAL_GetTick();
          TRACE_INFO("detector change %i \n", g_vref_mv);
        }
      }
    }

}
