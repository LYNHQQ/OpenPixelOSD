/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "msp.h"
#include "msp_fc.h"
#include "system.h"
#include "usb.h"
#include "video_gen.h"
#include "video_overlay.h"
#include "flash.h"
#if defined(BUILD_VARIANT_VTX)
#include "rtc6705.h"
#include "rf_pa.h"
#include "vtx_msp.h"
#include "settingsMenu.h"
#include "settings.h"
#endif
#include <stdio.h>
#ifdef TRACE_LEVEL
#include "dbgu.h"
#endif

#if defined(HIGH_RAM)
#include "video_graphics.h"
extern bool new_field;
#endif

#define LED_BLINK_INTERVAL 100 // milliseconds
#define DEBUG_LOOP_INTERVAL 5000 // milliseconds
#define LOGO_TIMEOUT_MS 4000 // 4 seconds

void led_blink(void);
void logo_timeout_check(void);

extern volatile uint16_t sync_voltage;
extern uint16_t sync_voltage_low;
extern uint16_t sync_levels[];
extern double rf_detector;

void debug_print_loop(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= DEBUG_LOOP_INTERVAL) {
        last_tick = HAL_GetTick();
        TRACE_CMD(uint16_t rf_detect_int = rf_detector);
        TRACE_INFO("sync V:%i bl: %i sync low:%i adc_PA:%i adc_5V:%i\n",
          sync_voltage, 
          (uint16_t)DAC12BIT_TO_MV(sync_levels[1] / VIDEO_TOTAL_GAIN), 
          sync_voltage_low,
          rf_detect_int,
          adc_read_mv(ADC_CH_RESERVED) * 2); // Loop debug printf here
    }
}

int main (void)
{
    HAL_Init();
    SystemClock_Config();
    #ifdef USE_SWO
    SWO_Init();
    TRACE_INFO_WP("\n");
    TRACE_INFO("Getting new Started Project --\n");
    TRACE_INFO("Compiled: %s %s --\n", __DATE__, __TIME__);
    #endif
    gpio_init();
    usb_init();
    dma_init();
    adc_init();
    flash_init();
#if defined(BUILD_VARIANT_VTX)
    settings_load();
#endif
    video_overlay_init();

#if defined(HIGH_RAM)
    video_graphics_init();
    video_draw_text_system_font(FONT_SYSTEM_WIDTH * 2, VIDEO_HEIGHT - FONT_SYSTEM_HEIGHT, "WAITING MSP...");
    video_graphics_draw_complete();
#endif
    msp_init();

#if defined(BUILD_VARIANT_VTX)
    rf_pa_init();
    if(rtc6705_init()) {
        printf("rtc6705 detected\r\n");
        vtx_set_pitmode(1);
        vtx_set_power(vtx_get_config()->power);
    }
#endif

    while (1)
    {
        msp_loop_process();
        led_blink();
        debug_print_loop();
        logo_timeout_check();
        video_sync_loop();
#if defined(BUILD_VARIANT_VTX)
        msp_menu();
        rf_pa_loop();
#endif

#if 0 // TODO: remove later
// For test only - 3D cube animation
#if defined(HIGH_RAM)
        if (new_field == false) {
            video_draw_3d_cube_animation();
        }
#endif
#endif

    }
}

void led_blink(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= LED_BLINK_INTERVAL) {
      LED_STATE_GPIO_Port->ODR ^= LED_STATE_Pin;
      last_tick = HAL_GetTick();
    }
}

void logo_timeout_check(void)
{
    static uint32_t boot_time = 0;
    static bool timeout_checked = false;
    extern bool show_logo;

    // Initialize boot time on first call
    if (boot_time == 0) {
        boot_time = HAL_GetTick();
    }

    // Check if LOGO_TIMEOUT_MS has elapsed, clear logo and version string if so
    if (!timeout_checked && (HAL_GetTick() - boot_time) >= LOGO_TIMEOUT_MS) {
        show_logo = false;
        // Clear the canvas to remove version string
        //canvas_char_clean();
        //canvas_char_draw_complete();
        timeout_checked = true;
    }
}
