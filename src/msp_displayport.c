/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include "msp_displayport.h"
#include "canvas_char.h"
#include "fonts/update_font.h"
#include "main.h"
#include "msp.h"
#include "uart.h"
#include "usb.h"
#include "rf_pa.h"

#if defined(BUILD_VARIANT_VTX)
#include "vtx_msp.h"
#define MSP_REQUEST_LOOP_INTERVAL 100u
#endif

typedef enum {
    MSP_DISPLAYPORT_KEEPALIVE,
    MSP_DISPLAYPORT_RELEASE,
    MSP_DISPLAYPORT_CLEAR,
    MSP_DISPLAYPORT_DRAW_STRING,
    MSP_DISPLAYPORT_DRAW_SCREEN,
    MSP_DISPLAYPORT_SET_OPTIONS,
    MSP_DISPLAYPORT_DRAW_SYSTEM
} msp_displayport_cmd_t;

extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
extern bool show_logo;

extern uint8_t fcArmed;
extern uint16_t rcChannel[4];
extern uint8_t stickPos;

CCMRAM_BSS static msp_port_t msp_uart = {0};
CCMRAM_BSS static msp_port_t msp_usb = {0};
EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);

uint16_t debug0;
uint16_t debug1;


void msp_displayport_init(void)
{
    uart1_init();
    uart1_dma_rx_start();
    msp_uart.callback = msp_callback;
    msp_uart.owner = MSP_OWNER_UART;

    msp_usb.callback = msp_callback;
    msp_usb.owner = MSP_OWNER_USB;
}

uint8_t mspStickpos(void) {
  uint8_t result = 0;
  for (uint8_t i = 0; i < 4; i++) {
    result >>= 2;
    if      (rcChannel[i] > 500   && rcChannel[i] < 1250  ) result |= 0x40;
    else if (rcChannel[i] >= 1250 && rcChannel[i] <= 1750 ) result |= 0x00;
    else if (rcChannel[i] > 1750  && rcChannel[i] < 2500  ) result |= 0x80;
    else result |= 0xc0;
  }
  return result;
}

EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    switch(msp_version) {
    case MSP_V1: {
        switch(msp_cmd) {
        case MSP_DISPLAYPORT: {
            msp_displayport_cmd_t sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            {
                static bool displayport_initialized = false;
                if (!displayport_initialized) {
                    #if defined(BUILD_VARIANT_VTX)
                    vtx_msp_request_config(owner);
                    #endif
                    displayport_initialized = true;
                    show_logo = false;
                    // Send canvas size to FC
                    uint8_t data[2] = {COLUMN_SIZE, ROW_SIZE};
                    uint8_t tx_buff[64];
                    uint16_t len = construct_msp_command_v1(tx_buff, MSP_SET_OSD_CANVAS, data, 2, MSP_OUTBOUND);
                    switch(owner) {
                    case MSP_OWNER_UART:
                        uart1_tx_dma(tx_buff, len);
                        break;
                    case MSP_OWNER_USB:
                        usb_uart_write_bytes((const char *)tx_buff, len);
                        break;
                    default:
                        break;
                    }
                }
            }
                break;
            case MSP_DISPLAYPORT_RELEASE: // 1 -> Close DisplayPort
                show_logo = true;
                break;
            case MSP_DISPLAYPORT_CLEAR: // 2 -> Clear Screen
                canvas_char_clean();
                break;
            case MSP_DISPLAYPORT_DRAW_STRING:  // 3 -> Draw String
            {
                if (data_size < 5) break;
                uint8_t row = payload[1];
                uint8_t col = payload[2];
                if (row >= ROW_SIZE || col >= COLUMN_SIZE) break;
                uint8_t len = data_size - 4;
                memcpy(&canvas_char_map[active_buffer][row][col], (const char *)&payload[4], len);
            }
                break;
            case MSP_DISPLAYPORT_DRAW_SCREEN: // 4 -> Draw Screen
                canvas_char_draw_complete();
                break;
            case MSP_DISPLAYPORT_SET_OPTIONS: // 5 -> Set Options (HDZero/iNav)
                break;
            default:
                break;
            }
        }
            break;

        case  MSP_OSD_CHAR_WRITE: {
            update_font_symbol_write(payload[0], &payload[1], data_size - 1);
        }
            break;

        case MSP_VTX_CONFIG:
        case MSP_SET_VTX_CONFIG:
        case MSP_VTXTABLE_BAND:
        case MSP_VTXTABLE_POWERLEVEL: {
#if defined(BUILD_VARIANT_VTX)
            vtx_msp_handle_msp(owner, msp_cmd, data_size, payload);
            const vtx_config_t *vtx_config = vtx_get_config();
            if (!vtx_config->vtx_table_available) {
                TRACE_INFO("Set Table defaults\n");
                vtx_msp_clear_table_and_set_defaults(owner);
                
            }
#endif
        }
            break;
        case MSP_STATUS:
          if ( !fcArmed && (payload[6] & 0x01)) {
              TRACE_INFO("FC ARMED\n");
              fcArmed = 1;
          } else if ( fcArmed && !(payload[6] & 0x01)) {
              TRACE_INFO("FC DISARMED\n");
              fcArmed = 0;
          }
          break;
        case MSP_RC:
            memcpy(rcChannel, (uint16_t*)payload, sizeof(rcChannel));
            stickPos = mspStickpos();
            break;
        case MSP_DEBUG:
            debug0 = payload[0] + (uint16_t)(payload[1]<<8);
            debug1 = (uint16_t)payload[2];
            TRACE_INFO("target_debug %04x %04x\n", debug0, debug1);
            switch (debug1) {
              case 0:
                rf_pa_set_vref_mv(debug0);
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
                vtx_msp_clear_table_and_set_defaults(MSP_OWNER_UART);
                break;

              default:
                break;
            }
            break;
        default:
            printf("MSP command not parsed %d:0x%02X\r\n",msp_cmd, msp_cmd);
            break;
        }
        break;
        
      }
      break;
    case MSP_V2_OVER_V1:
        break;
    case MSP_V2_NATIVE:
        switch(msp_cmd) {
          case MSP_PACALTABLE:
          case MSP_SET_PACALTABLE:
          case MSP_PACALIBRATION:
          case MSP_SET_PACALIBRATION:
#if defined(BUILD_VARIANT_VTX)
            vtx_msp_handle_msp(owner, msp_cmd, data_size, payload);
#endif
            break;
          default:
            printf("MSP V2 command not parsed %d:0x%02X\r\n",msp_cmd, msp_cmd);
            break;
        }
        break;
    default:
        break;
#if 0 // debug msp via usb-cdc
    for(int i = 0; i < data_size; i++) {
        printf("0x%02x ", payload[i]);
    }
    printf("\r\n");
#endif

    }
}

EXEC_RAM void msp_loop_process(void)
{
    uint8_t byte;
    while (uart_rx_ring_get(&byte)) {
        msp_process_received_data(&msp_uart, byte);
    }
    while (usb_uart_read_byte(&byte)) {
        msp_process_received_data(&msp_usb, byte);
    }

#if defined(BUILD_VARIANT_VTX)
    static uint32_t last_tick = 0;
    static bool resp = true;
    static uint8_t c = 0;
    if ((HAL_GetTick() - last_tick) >= (MSP_REQUEST_LOOP_INTERVAL + (1 - fcArmed) * 5) && resp) {
        last_tick = HAL_GetTick();
        switch(c) {
          case 0:
          case 2:
            vtx_msp_send_command(MSP_OWNER_UART, MSP_RC);
            c++;
            break;
          case 1:
            vtx_msp_send_command(MSP_OWNER_UART, MSP_STATUS);
            c = c + (1 - fcArmed);
            break;
          case 3:
            if (!vtx_get_config()->configSet) vtx_msp_request_config(MSP_OWNER_UART);
            c = 0;
            break;
        }
        
    }
#endif
}
