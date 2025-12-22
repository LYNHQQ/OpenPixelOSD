/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include <string.h>

#include "msp_fc.h"
#include "main.h"
#include "settings.h"
#include "msp.h"
#include "rf_pa.h"
#include "video_overlay.h"

fc_t fc;
uint8_t boxIdIdx = 0;

uint16_t debug0;
uint16_t debug1;

uint8_t mspStickpos(void) {
  uint8_t result = 0;
  for (uint8_t i = 0; i < 4; i++) {
    result >>= 2;
    if      (fc.rcChannel[i] > 500   && fc.rcChannel[i] < 1250  ) result |= 0x40;
    else if (fc.rcChannel[i] >= 1250 && fc.rcChannel[i] <= 1750 ) result |= 0x00;
    else if (fc.rcChannel[i] > 1750  && fc.rcChannel[i] < 2500  ) result |= 0x80;
    else result |= 0xc0;
  }
  return result;
}

bool msp_fc_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    uint32_t status;
    UNUSED(owner);

    switch(msp_cmd) {
    case MSP_STATUS:
        memcpy(&status,&payload[6],4);

        if ( !fc.status.armed && (status & 0x01)) {
            TRACE_INFO("FC ARMED\n");
            fc.status.armed = 1;
        } else if ( fc.status.armed && !(status & 0x01)) {
            TRACE_INFO("FC DISARMED\n");
            fc.status.armed = 0;
        }

        if (boxIdIdx) {
          if(!fc.status.cameraControl && (status & 1<<boxIdIdx)) {
            fc.status.cameraControl = 1;
            #if (VIDEO1_INPUT_ENABLED == true && VIDEO2_INPUT_ENABLED == true)
            TRACE_INFO("CAM SWITCH on\n");
            if (settings.camswitchEnabled)
              set_video_input(1);
            #endif
          } else if (fc.status.cameraControl && !(status & 1<<boxIdIdx)) {
            fc.status.cameraControl = 0;
            #if (VIDEO1_INPUT_ENABLED == true && VIDEO2_INPUT_ENABLED == true)
            TRACE_INFO("CAM SWITCH off\n");
            if (settings.camswitchEnabled)
              set_video_input(0);
            #endif
          }
        }
        break;

    case MSP_BOXIDS:
        for (uint16_t i = 0; i < data_size; i++) {
          if (BOXID_CAM_SWITCH && (payload[i] == BOXID_CAM_SWITCH)) {
            boxIdIdx = i;
            TRACE_INFO("BOXID IDX CAM SWITCH %02x\n", i)
          }
        }
        break;

    case MSP_RC:
        memcpy(fc.rcChannel, (uint16_t*)payload, sizeof(fc.rcChannel));
        fc.stickPos = mspStickpos();
        break;

    case MSP_DEBUG:
        debug0 = payload[0] + (uint16_t)(payload[1]<<8);
        debug1 = (uint16_t)payload[2];
        TRACE_INFO("target_debug %04x %04x\n", debug0, debug1);
        switch (debug1) {
        case 0:
#if defined(BUILD_VARIANT_VTX)
            rf_pa_set_vref_mv(debug0);
#endif
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        default:
            break;
        }
        break;
    default:
        return false;
        break;
    }
    return true;
}
