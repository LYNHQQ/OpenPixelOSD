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

void video_overlay_init(void);
void video_sync_loop(void);
void setSyncMode(syncMode_t mode);

#endif //VIDEO_OVERLAY_H
