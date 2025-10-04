/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdbool.h>
#include <string.h>
#include "canvas_char.h"
#include "main.h"
#if defined(HIGH_RAM)
#include "video_graphics.h"
#endif

CCMRAM_BSS char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
CCMRAM_DATA uint8_t active_buffer = 0;
CCMRAM_DATA uint8_t paint_buffer = 1;

EXEC_RAM void canvas_char_flush_map(void)
{
    memset(canvas_char_map, ' ', sizeof(canvas_char_map));
}

EXEC_RAM void canvas_char_clean(void)
{
    paint_buffer = 1 - active_buffer;
    memset(canvas_char_map[paint_buffer], ' ', sizeof(canvas_char_map[0]));
}

EXEC_RAM void canvas_char_write(uint8_t x, uint8_t y, const char *data, const uint16_t len)
{
    if (y >= ROW_SIZE) y = 0;
    if (x >= COLUMN_SIZE) x = 0;

    for (uint16_t i = 0; i < len; i++) {
        const uint8_t col = (x + i) % COLUMN_SIZE;
        canvas_char_map[paint_buffer][y][col] = data[i];
    }
}

EXEC_RAM void canvas_char_draw_complete(void)
{
    active_buffer = paint_buffer; // Switch active buffer
#if defined(HIGH_RAM)
    video_render_canvas_from_map(); // Render canvas map to video frame buffer
#endif
    
}

EXEC_RAM void canvas_print(uint8_t x, uint8_t y, const char *str) {
  if (x >= COLUMN_SIZE) return;
  if (y >= ROW_SIZE) return;

  while (*str && x < COLUMN_SIZE) {
    canvas_char_map[paint_buffer][y][x++] = *str++;
  }
}