/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_OVERLAY_H
#define VIDEO_OVERLAY_H

typedef enum {
  OFF,
  INTERNAL,
  EXTERNAL,
  AUTOMATIC
} syncMode_t;

typedef enum {
    SYNC_STATE_SEARCH,
    SYNC_STATE_FOUND,
    SYNC_STATE_EXTERNAL,
} syncState_t;

typedef enum {
  OSD_INIT,
  OSD_OFF,  
  OSD_MSP,
  OSD_MENU,
  OSD_EXIT_MENU
} osdState_e;

typedef enum {
    MODE_UNKNOWN,
    MODE_PAL,
    MODE_NTSC
} videoMode_t;

typedef struct {
    uint32_t  compInput;
    uint32_t  opampInput;
    uint8_t   gain;
} videoInput_t;

typedef struct {
    float     phase;
    uint16_t  luminance;
} colorMap_t;

extern osdState_e osdState;

void set_video_input(uint8_t input);
void video_overlay_init(void);
void video_sync_loop(void);
void setSyncMode(syncMode_t mode);
void set_video_input(uint8_t input);

#endif //VIDEO_OVERLAY_H
