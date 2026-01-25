/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef RTC6705_H
#define RTC6705_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  RTC6705_LOW      = 0x01,  // PA5G_BS = 000 PA5G_PW=00 PD_Q5G=1
  RTC6705_PA_3dBm  = 0x38,  // PA5G_BS = 111 PA5G_PW=00 PD_Q5G=0
  RTC6705_PA_7dBm  = 0x3A,  // PA5G_BS = 111 PA5G_PW=01 PD_Q5G=0
  RTC6705_PA_11dBm = 0x3C,  // PA5G_BS = 111 PA5G_PW=10 PD_Q5G=0
  RTC6705_PA_13dBm = 0x3E   // PA5G_BS = 111 PA5G_PW=11 PD_Q5G=0
} rtc6705_power_t;

bool rtc6705_init(void);
void rtc6705_allow_power_writes(bool allow);
void rtc6705_set_power(rtc6705_power_t level);
uint32_t rtc6705_set_frequency(uint32_t freq_mhz);

#endif //RTC6705_H
