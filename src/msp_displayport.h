/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef MSP_DISPLAYPORT_H
#define MSP_DISPLAYPORT_H
#include <stdint.h>
#include <stdbool.h>

#define MSP_PACALTABLE            0x4800
#define MSP_SET_PACALTABLE        0x4801
#define MSP_PACALIBRATION         0x4802
#define MSP_SET_PACALIBRATION     0x4803

typedef enum
{
  MSP_OWNER_UART = 0x00,
  MSP_OWNER_USB = 0x01,
  MSP_OWNER_MAX = 0xFF
} msp_owner_t;

typedef struct {
  bool armed : 1;
  bool cameraControl : 1;
} fc_status_t;

extern fc_status_t fcStatus;

void msp_displayport_init(void);
void msp_loop_process(void);

#endif //MSP_DISPLAYPORT_H
