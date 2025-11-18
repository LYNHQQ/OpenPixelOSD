/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_OVERLAY_H
#define VIDEO_OVERLAY_H

typedef enum {
  AUTOMATIC,
  INTERNAL,
  EXTERNAL,
  OFF
} syncMode_t;

void video_overlay_init(void);
void video_sync_loop(void);
void setSyncMode(syncMode_t mode);

#endif //VIDEO_OVERLAY_H
