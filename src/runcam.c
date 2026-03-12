#include "main.h"
#include "runcam.h"
#include "uart.h"

#if defined(USE_RUNCAM)

#define RCDEVICE_PROTOCOL_HEADER                                    0xCC
// Commands
#define RCDEVICE_PROTOCOL_COMMAND_GET_DEVICE_INFO                   0x00
// camera control
#define RCDEVICE_PROTOCOL_COMMAND_CAMERA_CONTROL                    0x01

#define RCDEVICE_PROTOCOL_CAM_CTRL_SIMULATE_WIFI_BTN                0x00
#define RCDEVICE_PROTOCOL_CAM_CTRL_SIMULATE_POWER_BTN               0x01
#define RCDEVICE_PROTOCOL_CAM_CTRL_CHANGE_MODE                      0x02
#define RCDEVICE_PROTOCOL_CAM_CTRL_START_RECORDING                  0x03
#define RCDEVICE_PROTOCOL_CAM_CTRL_STOP_RECORDING                   0x04


uint8_t crc8_dvb_s2(uint8_t crc, unsigned char a)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc = crc << 1;
        }
    }

    return crc;
}


void runcam_send_packet(uint8_t command, uint8_t *data, int len)
{

    uint8_t buffer[16];
    // prepare pointer
    
    buffer[0] = RCDEVICE_PROTOCOL_HEADER;
    buffer[1] =command;

    for(uint8_t x = 0; x < len; x++) {
      buffer[x + 2] = data[x];
    }

    uint8_t crc = 0;
    for(uint8_t x = 0; x < 2 + len; x++) {
      crc = crc8_dvb_s2(crc, buffer[x]);
    }

    // add crc over (all) data
    buffer[2+len] = crc;

    uart3_tx_dma(buffer, 3 + len);
}

void runcam_recording(bool start) {
  uint8_t p[1];

  if(start) 
    p[0] = RCDEVICE_PROTOCOL_CAM_CTRL_START_RECORDING;
  else
    p[0] = RCDEVICE_PROTOCOL_CAM_CTRL_STOP_RECORDING;

  runcam_send_packet(RCDEVICE_PROTOCOL_COMMAND_CAMERA_CONTROL, p, 1);
}  

void runcam_init(void) {
  uart3_init();
  //uart3_dma_rx_start();

  runcam_recording(0);
}   

void runcam_loop(void) {
#if 0
  uint8_t byte;
  static uint8_t buffer[64];
  static uint8_t c = 0;
  static uint32_t last_rx = 0;

  while (uart3_rx_ring_get(&byte) && (c<64)) {
    last_rx = HAL_GetTick();
    buffer[c++] = byte;
  }

  if(c && ((HAL_GetTick() - last_rx) > 100)) {
    for(uint8_t x = 0; x < c; x++) {
      TRACE_INFO_WP("%02x ", buffer[x]);
    }
    
    TRACE_INFO_WP("\n");
    c = 0;
  }
#endif
}

void cameraControl2_changed(bool value) {
  runcam_recording(value);
}

#endif