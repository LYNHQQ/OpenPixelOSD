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
uint8_t boxIdIdx[3] = {0};

uint16_t debug0;
uint16_t debug1;
uint16_t debug2;
uint16_t debug3;

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

extern uint16_t triggerLine;
extern videoMode_t videoMode;

#ifdef USE_COLOR
extern uint32_t phase_val[2][10];
extern float phaseOffset;
void set_color_phase(videoMode_t mode);
extern uint16_t colorDelay;
#endif


__weak void cameraControl1_changed(bool value) {
  UNUSED(value);
}

__weak void cameraControl2_changed(bool value) {
  UNUSED(value);
}

__weak void cameraControl3_changed(bool value) {
  UNUSED(value);
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

        if (boxIdIdx[0] && (fc.status.cameraControl1 != ((status>>boxIdIdx[0]) & 0x01))) {
          fc.status.cameraControl1 = ((status>>boxIdIdx[0]) & 0x01);
          cameraControl1_changed(fc.status.cameraControl1);
          TRACE_INFO("CAMERA_CONTROL_1 %i\n", fc.status.cameraControl1);
        }   
        if (boxIdIdx[1] && (fc.status.cameraControl2 != ((status>>boxIdIdx[1]) & 0x01))) {
          fc.status.cameraControl2 = ((status>>boxIdIdx[1]) & 0x01);
          cameraControl2_changed(fc.status.cameraControl2);
          TRACE_INFO("CAMERA_CONTROL_2 %i\n", fc.status.cameraControl2);
        }
        if (boxIdIdx[2] && (fc.status.cameraControl3 != ((status>>boxIdIdx[2]) & 0x01))) {
          fc.status.cameraControl3 = ((status>>boxIdIdx[2]) & 0x01);
          cameraControl3_changed(fc.status.cameraControl3);
          TRACE_INFO("CAMERA_CONTROL_3 %i\n", fc.status.cameraControl3);
        }
        break;

    case MSP_BOXIDS:
        for (uint16_t i = 0; i < data_size; i++) {
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_1) {
            boxIdIdx[0] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_1 %02x\n", i)
          }
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_2) {
            boxIdIdx[1] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_2 %02x\n", i)
          }
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_3) {
            boxIdIdx[2] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_3 %02x\n", i)
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
        debug2 = payload[4] + (uint16_t)(payload[5]<<8);
        debug3 = payload[6] + (uint16_t)(payload[7]<<8);
        TRACE_INFO("target_debug %04x %04x %04x %04x\n", debug0, debug1, debug2, debug3);
        switch (debug1) {
        case 0:
#if defined(USE_VTX)
            rf_pa_set_vref_mv(debug0);
#endif
            break;
        case 1:
            
            break;
        case 2:
            #ifdef USE_COLOR
            if (debug0) {
              LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_A, debug0);
              LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, (debug0 + debug2) % 1227);
            } else {
              LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_A, debug2);
              LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, debug3);
            }
            #endif
            break;
        case 3:
            #ifdef TRIGGER_LINE
            triggerLine = debug0;
            #endif
            break;
        case 4:
            {
              //char c = debug0;
              //canvas_char_write(debug2, debug3, (const char*)&c, 1, 0);
            }
            break;
        case 5:
        case 6:
            LL_TIM_OC_SetCompareCH2(TIM1, debug0);
            break;
        case 7:
        case 8:
        case 9:
            {
              #ifdef USE_COLOR
                colorDelay = debug0;
                LL_TIM_OC_SetCompareCH1(TIM1, TIM1_AUTORELOAD - (colorDelay % TIM1_AUTORELOAD));

                phaseOffset = debug2 / 10.0f;
                set_color_phase(videoMode);
              #endif
            }
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
