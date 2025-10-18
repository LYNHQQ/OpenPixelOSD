/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "video_overlay.h"
#include "system.h"
#include "main.h"
#include "video_gen.h"
#include "logo/logo.h"
#include "canvas_char.h"
#include "video_graphics.h"
#if defined(LOW_RAM)
#include "fonts/font_bf_default.h"
#endif

#define TIM2_TICK_MS        (1e6f / 170000000)

// OPAMP1 multiplexer constants
#define OPAMP_CONST_IO0     0x108000E1U  // Positive Input IO0 (e.g., PA1) -
#define OPAMP_CONST_IO1     ((VIDEO_TOTAL_GAIN / VIDEO2_INPUT_GAIN) == 1 ? 0x108000E5U : 0x108000E5U)   // Positive Input IO1 (e.g., PA3) - video generator input
#define OPAMP_CONST_IO2     ((VIDEO_TOTAL_GAIN / VIDEO1_INPUT_GAIN) == 1 ? 0x108000E9U : 0x108000C9U)  // Positive Input IO2 (e.g., PA7) - video input form camera

#define OPAMP_CONST_DAC     0x108000EDU  // Positive Input DAC1_OUT1 (internal DAC)

#define OFFSET_Y            (16)
#define LINE_BUF_SZ         (PEXELS_PER_LINE+1)

#define OFFSET_SYNC         300
#define OFFSET_TRANSPARENT  100
#define OFFSET_WHITE        700
#define OFFSET_GREY         250

#define DAC_BLACK           DAC12BIT_FROM_MV(550)
#define DAC_WHITE           (DAC_BLACK + DAC12BIT_FROM_MV(OFFSET_WHITE))
#define DAC_GRAY            (DAC_BLACK + DAC12BIT_FROM_MV(OFFSET_GREY))
#define DAC_SYNC            (DAC_BLACK - DAC12BIT_FROM_MV(OFFSET_SYNC))

#define MAX_RENDER_LINE     (303) // for PAL

#define LOGO_OFFSET_X       (90)
#define LOGO_OFFSET_Y       (25)

uint32_t opa_val[8] = { OPAMP_CONST_DAC, OPAMP_CONST_DAC, OPAMP_CONST_DAC, OPAMP_CONST_DAC, 
                        OPAMP_CONST_IO0, OPAMP_CONST_IO0, OPAMP_CONST_IO0, OPAMP_CONST_IO0  };

uint16_t video_level[9] = { DAC_SYNC, DAC_BLACK, DAC_BLACK, DAC_WHITE, DAC_GRAY,
                                      DAC_BLACK, DAC_GRAY, DAC_WHITE, DAC_GRAY};

uint16_t* video_levels = &video_level[1];
uint32_t* opa_vals = &opa_val[0];

uint16_t sync_quality = 0;
uint16_t sync_noise = 0;
uint8_t frame_counter = 0; 
uint16_t sync_voltage = SYNC_START_MV;

static uint16_t dac_buff[2][LINE_BUF_SZ];   // DAC double buffer for draw pixel (12-bit CH1)  DMA HALF_WORLD/WORLD
static uint32_t opamp_buff[2][LINE_BUF_SZ]; // double buffer for OPAMP1 multiplexer (32-bit)  DMA WORLD/WORLD
CCMRAM_BSS static bool buf_idx = 0; // current buffer index for double buffering
CCMRAM_DATA static uint32_t video_source = VIDOE_OPAMP_IMPUT;
extern volatile bool video_gen_enabled;
extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
CCMRAM_DATA bool show_logo = true;
CCMRAM_DATA bool show_test_pattern = false;
CCMRAM_BSS bool new_field = false;

#if defined(HIGH_RAM)
extern uint8_t active_video_buffer;
extern uint8_t video_frame_buffer[2][VIDEO_HEIGHT][VIDEO_BYTES_PER_LINE];
#endif

EXEC_RAM static void set_black_level(uint32_t new_level)
{
  if (new_level > DAC12BIT_FROM_MV(OFFSET_SYNC))
    video_level[0] = (new_level - DAC12BIT_FROM_MV(OFFSET_SYNC)) * VIDEO_TOTAL_GAIN;
  else
    video_level[0] = 0;

  video_level[1] = new_level * VIDEO_TOTAL_GAIN;
  video_level[2] = (new_level + DAC12BIT_FROM_MV(OFFSET_TRANSPARENT)) * VIDEO_TOTAL_GAIN;
  video_level[3] = (new_level + DAC12BIT_FROM_MV(OFFSET_WHITE)) * VIDEO_TOTAL_GAIN;
  video_level[4] = (new_level + DAC12BIT_FROM_MV(OFFSET_GREY)) * VIDEO_TOTAL_GAIN;

#ifdef ALPHA_CHANNEL
  video_level[5] = new_level;
  video_level[6] = new_level + DAC12BIT_FROM_MV(OFFSET_TRANSPARENT);
  video_level[7] = new_level + DAC12BIT_FROM_MV(OFFSET_WHITE);
  video_level[8] = new_level + DAC12BIT_FROM_MV(OFFSET_GREY);

  LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, new_level + DAC12BIT_FROM_MV(300));
  LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_1);
#endif
}

EXEC_RAM static void init_buffers()
{
    for (uint32_t j = 0; j < LINE_BUF_SZ; j++) {
        dac_buff[0][j] = DAC_GRAY;
        opamp_buff[0][j] = video_source;
        dac_buff[1][j] = DAC_GRAY;
        opamp_buff[1][j] = video_source;
    }
}

static void show_version(void)
{
    char str[COLUMN_SIZE];
    sprintf(str, "FW: %s", FW_VERSION);
    canvas_char_write(8, 9, str, strlen(str));
    sprintf(str, "MCU: %s", MCU_TYPE);
    canvas_char_write(8, 10, str, strlen(str));
    canvas_char_draw_complete();
}

void video_overlay_init(void)
{
    init_buffers();
    canvas_char_flush_map();

#if defined(HIGH_RAM)
    video_graphics_init();
#endif

    DAC1_Init(); // DAC1_CH1 for video detection
    DAC3_Init(); // DAC3_CH1 for render line
    OPAMP1_Init(); // OPAMP1 as multiplexer for video source selection
    TIM1_Init(); // TIM1 for video line generation
    TIM2_Init(); // TIM2 detect HSYNC VSYNC video input
    TIM15_Init(); // TIM15 delay for video line generation start
    TIM17_Init(); // TIM17 for video generator
    COMP2_Init(); // COMP2 for video sync detection
    //COMP3_Init(); // COMP3 for video sync detection
    
#if defined(ALPHA_CHANNEL) && defined(STM32G474xx)
    OPAMP6_Init();
    LL_OPAMP_Enable(OPAMP6);
#endif

    LL_OPAMP_Enable(OPAMP1);
    LL_OPAMP_Enable(OPAMP3);
    

    LL_COMP_Enable(COMP2);
    LL_COMP_Enable(COMP3);

    LL_TIM_EnableIT_CC2(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);

    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);

    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);

    LL_TIM_EnableIT_UPDATE(TIM4);
    LL_TIM_EnableCounter(TIM4);

    LL_TIM_EnableIT_UPDATE(TIM17);

    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, (DAC12BIT_FROM_MV(SYNC_START_MV)) * VIDEO_INPUT_GAIN);

    show_version();
    video_gen_enabled = true;
    video_gen_stop();

}

void scan_sync_voltage (int16_t step) {

    sync_voltage += step;

    if(sync_voltage > SYNC_SCAN_MAX_MV) {
      sync_voltage = SYNC_SCAN_MIN_MV;
    } else if(sync_voltage < SYNC_SCAN_MIN_MV) {
      sync_voltage = SYNC_SCAN_MIN_MV;
    }

    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_voltage) * VIDEO_INPUT_GAIN);
}


#if defined(LOW_RAM)
EXEC_RAM static void squash_canvas_raw_pixel_buff(char c, uint32_t glyph_row, uint32_t x_off)
{
    const uint8_t * const glyph = &font_data[(uint8_t)c * FONT_STRIDE];
    const uint32_t row_offset = glyph_row * BYTES_PER_ROW;

    for(uint32_t col = 0; col < FONT_WIDTH; col++) {
        uint32_t bitpos     = col * FONT_BPP;
        uint32_t byte_index = row_offset + (bitpos >> 3);
        uint32_t bit_offset = bitpos & 0x7;

        uint8_t raw_byte = glyph[byte_index];
        uint8_t pixel = (raw_byte >> (6 - bit_offset)) & 0x03;

        dac_buff[buf_idx][x_off + col] = video_levels[pixel];
        opamp_buff[buf_idx][x_off + col] = opa_vals[pixel];;

    }
    opamp_buff[buf_idx][x_off + FONT_WIDTH] = video_source;
}
#endif

void render_overlay_logo_line(uint16_t line)
{
    if (line >= MAX_RENDER_LINE) return;

    // Logo line considering vertical offset
    if (line < LOGO_OFFSET_Y || (line - LOGO_OFFSET_Y) >= LOGO_HEIGHT) {
        // Line outside the logo area — do nothing, keep existing buffer contents
        return;
    }

    uint16_t logo_row = line - LOGO_OFFSET_Y;
    const uint8_t *logo_line_ptr = &logo_data[logo_row * LOGO_ROW_BYTES];

    uint32_t buf_idx_local = LOGO_OFFSET_X;
    for (uint32_t i = 0; i < LOGO_WIDTH>>2; i++) {
        uint8_t byte = logo_line_ptr[i];
        uint8_t pixel;
    
        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 6) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 4) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 2) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = byte & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];
    }
}

void render_test_pattern_line(uint16_t line)
{
    if (line >= MAX_RENDER_LINE) return;

    // Logo line considering vertical offset
    if (line < LOGO_OFFSET_Y || (line - LOGO_OFFSET_Y) >= LOGO_HEIGHT) {
        // Line outside the logo area — do nothing, keep existing buffer contents
        return;
    }
    uint8_t pattern = 0;

    for (uint16_t buf_index = 0; buf_index < LINE_BUF_SZ - 32; buf_index += 8) {

        px_t px = pattern++ & 0x03;

        // If pixel is transparent — skip (keep existing buffer value)
        if (px == PX_TRANSPARENT) {
            continue;
        }

        // Overlay — overwrite only if pixel is not transparent
        for (uint8_t x = 0; x < 8; x++) {
          dac_buff[buf_idx][buf_index + x] = video_levels[px + 1];
          opamp_buff[buf_idx][buf_index+ x] = opa_vals[px];
        }
        
    }
}

#if defined(LOW_RAM)
EXEC_RAM static void render_line(uint16_t line)
{
    CCMRAM_BSS static uint32_t draw_line = 0;
    uint32_t map_row = 0;
    uint32_t glyph_row = 0;
    uint32_t line_parity = 0;
    uint32_t i = 0;
    char c = 0;

    // Offset current draw line by Y_OFFSET
    if (line < OFFSET_Y) return;
    draw_line = line - OFFSET_Y;

    // Calculate which character row and which row in the glyph
    map_row    = draw_line / FONT_HEIGHT;  // 0..ROW_SIZE-1
    glyph_row  = draw_line % FONT_HEIGHT;  // 0..FONT_HEIGHT-1

    line_parity = draw_line & 1;

    if (map_row >= ROW_SIZE) {
        // Out of screen — just transparent
        for (i = 0; i < LINE_BUF_SZ; i++) {
            dac_buff[line_parity][i] = video_level[1];
            opamp_buff[line_parity][i] = video_source;
        }
        return;
    }

    // Render each character of the map
    for (i = 0; i < COLUMN_SIZE; i++) {
        c = canvas_char_map[active_buffer][map_row][i]; // draw previous char map buffer
        squash_canvas_raw_pixel_buff(c, glyph_row, i * FONT_WIDTH);
    }

    opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
}
#endif

#if defined(HIGH_RAM)

EXEC_RAM void render_video_line(uint16_t line)
{
    CCMRAM_BSS static uint32_t draw_line = 0;

    draw_line = line - 1;

    uint8_t *line_ptr = video_frame_buffer[!active_video_buffer][draw_line];

    if (draw_line >= VIDEO_HEIGHT) {
      for (uint16_t i = 0; i < LINE_BUF_SZ; i++ ) {
        dac_buff[buf_idx][i] = video_levels[1];
        opamp_buff[buf_idx][i] = video_source;
      }
    } else {
      uint32_t buf_idx_local = 0;
      for (uint32_t i = 0; i < VIDEO_BYTES_PER_LINE; i++) {
          uint8_t byte = line_ptr[i];
          uint8_t pixel;
      
          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (byte >> 6) & 0x3;
          dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
          opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (byte >> 4) & 0x3;
          dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
          opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (byte >> 2) & 0x3;
          dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
          opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = byte & 0x3;
          dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
          opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];
      }
      dac_buff[buf_idx][LINE_BUF_SZ-1] = video_levels[1];
      opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
    }
}
#endif

EXEC_RAM static void push_line_to_dma(uint16_t line)
{
    buf_idx = (line & 1);
    // Stop TIM1 and both DMA channels
    LL_TIM_DisableCounter(TIM1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_1);

    // Configure length and addresses
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&opamp_buff[!buf_idx][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, LINE_BUF_SZ);
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_CHANNEL_1, (uint32_t)&dac_buff[!buf_idx][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_1, LINE_BUF_SZ);

    // Enable DMA request for DAC
    LL_DAC_EnableDMAReq(DAC3, LL_DAC_CHANNEL_1);

    // Enable both DMA channels
    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_1); // first start DAC channel
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    LL_TIM_EnableDMAReq_UPDATE(TIM1);


#if defined(HIGH_RAM)
    render_video_line(line); // video frame buffer
#else
    render_line(line); // char canvas map
#endif
    if (show_test_pattern) {
      render_test_pattern_line(line);
    } else if (show_logo) {
      render_overlay_logo_line(line);  
    }
}

EXEC_RAM static inline void pars_video_signal(uint32_t tim_tick)
{
    CCMRAM_BSS static uint16_t video_line = 0;
    CCMRAM_BSS static uint16_t quality = 0;

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if(time_ns > 59.f && time_ns < 65.f) {
        video_line++;
        #ifdef TRIGGER_LINE
        if (video_line == TRIGGER_LINE) {
          LL_GPIO_SetOutputPin(LED_STATE_GPIO_Port, LED_STATE_Pin);
        }
        #endif
        if (video_line <= MAX_RENDER_LINE) {
            push_line_to_dma(video_line);
        }
        if (new_field == false) {
            new_field = true;
        }
        if(time_ns < 63.5f) {
          quality++;
        }
    } else if (time_ns > 29.0f && time_ns < 33.0f) {
        if ( video_line > 10) {
          if(video_line > 300) {
            sync_quality = quality;
            sync_noise = 0;
            frame_counter++;
          } else {
            sync_quality = quality + (305 - video_line);
          }
          quality = 0;
        }
        video_line = 0;
        if (new_field == true) {
            new_field = false;
        }
    } else {
        sync_noise++;
        // Do nothing, wait for next sync
    }
    #ifdef TRIGGER_LINE
      LL_GPIO_ResetOutputPin(LED_STATE_GPIO_Port, LED_STATE_Pin);
    #endif
}

EXEC_RAM static inline bool check_resync(uint32_t tim_tick)
{
    CCMRAM_BSS static uint8_t resync = 100;

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if(time_ns > 63.5f && time_ns < 64.5f) {
        resync--;
    } else {
        resync = 100;
    }
    if(resync) {
      return false;
    }
    
    resync = 100;
    return true;
}

// Called from COMP2 & TIM2 event (video input)
EXEC_RAM void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC2(TIM2)) {
        if (video_gen_enabled == true) {
          if(!check_resync(TIM2->CCR2)) {
            LL_TIM_ClearFlag_CC2(TIM2);
            return;
          }
        }
        TIM4->CNT = 0;
        video_source = VIDOE_OPAMP_IMPUT;
        opa_val[1] = video_source;
        opa_val[5] = VIDOE_OPAMP_IMPUT;
        OPAMP1->CSR = video_source;
        LL_TIM_DisableCounter(TIM8);
        if (video_gen_enabled == true) {
            video_gen_stop();
        } else {
            pars_video_signal(TIM2->CCR2);
            set_black_level(LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_1) / VIDEO_TOTAL_GAIN);
        }
        LL_TIM_ClearFlag_CC2(TIM2);
    }
}

// Called form TIM17 event (video gen)
EXEC_RAM void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
    static uint32_t ARR_prev = 0;
    if (LL_TIM_IsActiveFlag_UPDATE(TIM17)) {
      if(DMA1_Channel5->CNDTR == 1) {
        ARR_prev += TIM17->ARR;
      } else {
        TIM4->CNT = 0;
        video_source = OPAMP_CONST_DAC;
        opa_val[1] = video_source;
        opa_val[5] = OPAMP_CONST_DAC;
        OPAMP1->CSR = video_source;
        if (video_gen_enabled == false) {
            set_black_level(DAC_BLACK);
            video_gen_start();
        } else {
            pars_video_signal(ARR_prev);
        }
        ARR_prev = TIM17->ARR;
      }
        LL_TIM_ClearFlag_UPDATE(TIM17);
    }
}


void video_sync_loop(void) {
  static uint32_t last_tick = 0;
  static uint8_t sync_lost = 50;
  static uint8_t last_frame;

  if ((HAL_GetTick() - last_tick) >= 20) {
    last_tick = HAL_GetTick();
    if(sync_quality > SYNC_QUALITY_THRESHOLD || video_gen_enabled == true) {
      scan_sync_voltage(SYNC_SCAN_INC_MV);
    } else if (sync_noise > SYNC_NOISE_THRESHOLD) {
      sync_noise = 0;
      scan_sync_voltage(SYNC_SCAN_INC_MV);
    }
    else if(DAC12BIT_TO_MV(video_level[1] / VIDEO_TOTAL_GAIN) - sync_voltage < SYNC_TO_BLACK_MIN_MV ) {
      scan_sync_voltage(-1);
    }

    if (video_gen_enabled == false) {
      if(last_frame == frame_counter) {
        if(!--sync_lost) {
          video_source = OPAMP_CONST_DAC;
          OPAMP1->CSR = video_source;
          set_black_level(DAC_BLACK);
          video_gen_start();
          sync_lost = SYNC_LOST_FRAMES_THRESHOLD;
        }
      } else {
        sync_lost = SYNC_LOST_FRAMES_THRESHOLD;
      }
      last_frame = frame_counter;
    }
  }


}
