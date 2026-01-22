
#ifndef __MAIN_H
#define __MAIN_H

#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_comp.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_dac.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_opamp.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_hrtim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_adc.h"
#include "trace.h"

#ifndef GIT_TAG
#define GIT_TAG "-.-.-"
#endif /* GIT_TAG */

#ifndef GIT_BRANCH
#define GIT_BRANCH ""
#endif /* GIT_BRANCH */

#ifndef GIT_HASH
#define GIT_HASH ""
#endif /* GIT_HASH */

#define FW_VERSION GIT_TAG
#ifndef MCU_TYPE
#define MCU_TYPE "---------"
#endif /* MCU_TYPE */

#define ROW_SIZE                                16
#define COLUMN_SIZE                             41

#define VISUAL_PICTURE_LINE_NS                  50000
#define LINE_CENTER_NS                          31400

#define NS_TO_TICKS(ns)                         (((ns) * 170UL) / 1000UL)
#define VISUAL_PICTURE_LINE_TICKS_MAX           (NS_TO_TICKS(VISUAL_PICTURE_LINE_NS))
#define PIXELS_PER_LINE                         (COLUMN_SIZE * 12)
#define TIM1_AUTORELOAD                         ((uint32_t)(VISUAL_PICTURE_LINE_TICKS_MAX / PIXELS_PER_LINE) - 1)
#define VISUAL_PICTURE_LINE_TICKS               ((TIM1_AUTORELOAD + 1) * PIXELS_PER_LINE)
#define LINE_START_DELAY                        (NS_TO_TICKS(LINE_CENTER_NS) - (VISUAL_PICTURE_LINE_TICKS) / 2)



#define BLACK_LEVEL_ADC_DELAY_NS                3300
#define LOW_SYNC_ADC_DELAY_NS                   6000
#define COLOR_BURST_SYNC_GATE_CLOSE_NS          2100
#define VISIBLE_LINE_END_NS                     57000

typedef enum {
  PX_BLACK = 0,
  PX_TRANSPARENT,
  PX_WHITE,
  PX_GRAY,
  PX_GREEN,
  PX_RED,
  PX_BLUE,
  PX_YELLOW
} px_t;

// see adc.c - adc_init()
typedef enum {
  ADC_CH_RESERVED = 0, // reserved
  ADC_CH_PA_VDET = 1, // rf pa vdet signal
  ADC_CH_TEMP = 2, // internal temperature sensor
  ADC_CH_VREF_INT  = 3, // internal VREFINT
  ADC_CH_COUNT
} adc_ch_t;


#define OPAMP1_VOUT_VIDEO_OUT_Pin               LL_GPIO_PIN_2
#define OPAMP1_VOUT_VIDEO_OUT_GPIO_Port         GPIOA

#define OPAMP1_VINPIO0_VIDEO2_IN_Pin            LL_GPIO_PIN_3
#define OPAMP1_VINPIO0_VIDEO2_IN_GPIO_Port      GPIOA

#define OPAMP1_VINPIO2_VIDEO1_IN_Pin            LL_GPIO_PIN_7
#define OPAMP1_VINPIO2_VIDEO1_IN_GPIO_Port      GPIOA

//
// Reserved pins for future features
//

// If RGB LED support is added, then TIM8 has required features for driving by DMA.
#define RGBLED_TIM8_CH1_Pin                     LL_GPIO_PIN_15
#define RGBLED_TIM8_CH1_GPIO_Port               GPIOA

// If FRSKY PixelOSD protocol is added, a second UART can be used.
#define FRSKY_PIXEL_OSD_TX_USART3_TX_Pin        LL_GPIO_PIN_10
#define FRSKY_PIXEL_OSD_TX_USART3_TX_GPIO_Port  GPIOC
#define FRSKY_PIXEL_OSD_RX_USART3_RX_Pin        LL_GPIO_PIN_11
#define FRSKY_PIXEL_OSD_RX_USART3_RX_GPIO_Port  GPIOC

// If RF PA VBIAS is expanded, then DAC1_OUT1 can be used to control the VBIAS voltage.
#define RF_VBIAS_DAC1_OUT2_Pin                  LL_GPIO_PIN_5
#define RF_VBIAS_DAC1_OUT2_GPIO_Port            GPIOA


#define EXEC_RAM      __attribute__((section (".ccmram.text"), optimize("Ofast"))) /* exec functions from CCMRAM */
#define CCMRAM_DATA   __attribute__((section (".ccmram.data"))) /* initialized var */
#define CCMRAM_BSS    __attribute__((section (".ccmram.bss"))) /* uninitialized var */

#define DAC12BIT_TO_MV(value)                   (((uint32_t)(value) * 3300) / 4095)
#define DAC12BIT_FROM_MV(mV)                    (((uint32_t)(mV) * 4095) / 3300)

#define DAC8BIT_TO_MV(value)                    (((uint32_t)(value) * 3300) / 255)
#define DAC8BIT_FROM_MV(mV)                     (((uint32_t)(mV) * 255) / 3300)

#define SYNC_START_MV                           300
#define SYNC_SCAN_MIN_MV                        25
#define SYNC_SCAN_MAX_MV                        800
#define SYNC_SCAN_INC_MV                        25

#define SYNC_LOST_FRAMES_THRESHOLD              20

#define BOXID_CAM_SWITCH                        MSP_BOXID_CAMERA_CONTROL_1


#if defined(TARGET_PIXELVTX)
#include "targets\pixelVTX.h"
#elif defined(TARGET_PIXELVTX_COLOR)
#include "targets\pixelVTXcolor.h"
#else
#include "targets\generic.h"
#endif


#if defined(STM32G474xx) && defined(USE_COLOR) && USE_COLOR == 1 
#define IF_USE_COLOR(arg)        arg
#undef  COLUMN_SIZE
#define COLUMN_SIZE              30
#else
#define IF_USE_COLOR(...)        { }
#undef  USE_COLOR
#define USE_COLOR                0
#endif

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

void gpio_init(void);
void adc_init(void);
uint16_t adc_read_raw(adc_ch_t ch);
uint16_t adc_read_mv(adc_ch_t ch);
uint32_t adc_read_vdda_mv(void);
float adc_read_mcu_temp_c(void);
uint16_t adc_read_black_level(void);

void DAC1_Init(void);
void DAC3_Init(void);

void dma_init(void);

void OPAMP1_Init(void);

void TIM1_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);
void TIM7_Init(void);
void TIM15_Init(void);
void TIM17_Init(void);
void HRTIM1_Init(void);

void COMP2_Init(void);
void COMP3_Init(void);

/* Canvas character functions */
EXEC_RAM void canvas_char_clean(void);
EXEC_RAM void canvas_char_draw_complete(void);

#endif /* __MAIN_H */
