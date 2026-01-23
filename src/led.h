#pragma once

#include <stdint.h>

#define RGB_LED_COUNT         5

#define RGB(R,G,B)            (B+(R<<8)+(G<<16))

#define LED_STATE             64
#define TP1                   65
#define TP2                   66

void led_init(void);
void led_set(uint8_t idx, uint32_t value);
void led_toggle(uint8_t idx);
void RGB_led_send(void);