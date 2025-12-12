
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
#define COLUMN_SIZE                             38

#define VISUAL_PICTURE_LINE_NS                  49000
#define LINE_START_DELAY_NS                     5500

#define NS_TO_TICKS(ns)                         (((ns) * 170UL) / 1000UL)
#define VISUAL_PICTURE_LINE_TICKS_MAX           (NS_TO_TICKS(VISUAL_PICTURE_LINE_NS))
#define PIXELS_PER_LINE                         (COLUMN_SIZE * 12)
#define TIM1_AUTORELOAD                         ((uint32_t)(VISUAL_PICTURE_LINE_TICKS_MAX / PIXELS_PER_LINE))
#define VISUAL_PICTURE_LINE_TICKS               (TIM1_AUTORELOAD * PIXELS_PER_LINE)
#define LINE_START_DELAY                        ((VISUAL_PICTURE_LINE_TICKS_MAX - VISUAL_PICTURE_LINE_TICKS) / 2 + NS_TO_TICKS(LINE_START_DELAY_NS))


#define BLACK_LEVEL_ADC_DELAY_NS                3300
#define LOW_SYNC_ADC_DELAY_NS                   6000

typedef enum {
  PX_BLACK = 0,
  PX_TRANSPARENT,
  PX_WHITE,
  PX_GRAY
} px_t;

/* --------- Logical channel order in the regular sequence ---------
 * R1: PA-VDET (PB14/IN14)
 * R2: TempSensor (internal)
 * R3: reserved
 */
typedef enum {
  ADC_CH_PA_VDET = 0,
  ADC_CH_VCC5 = 1,
  ADC_CH_TEMP = 2,
  ADC_CH_VREF_INT  = 3, // internal VREFINT
  ADC_CH_COUNT
} adc_ch_t;


#define OPAMP1_VOUT_VIDEO_OUT_Pin               LL_GPIO_PIN_2
#define OPAMP1_VOUT_VIDEO_OUT_GPIO_Port         GPIOA

#define OPAMP1_VINPIO0_VIDEO2_IN_Pin            LL_GPIO_PIN_3
#define OPAMP1_VINPIO0_VIDEO2_IN_GPIO_Port      GPIOA

#define OPAMP1_VINPIO2_VIDEO1_IN_Pin            LL_GPIO_PIN_7
#define OPAMP1_VINPIO2_VIDEO1_IN_GPIO_Port      GPIOA

//#define COMP_INP_VIDEO_SYNC_IN_Pin              LL_GPIO_PIN_3
//#define COMP_INP_VIDEO_SYNC_IN_GPIO_Port        GPIOA

#define SPI2_CS_Pin                             LL_GPIO_PIN_7
#define SPI2_CS_GPIO_Port                       GPIOB
#define SPI2_SCK_Pin                            LL_GPIO_PIN_6
#define SPI2_SCK_GPIO_Port                      GPIOB
#define SPI2_MOSI_Pin                           LL_GPIO_PIN_9
#define SPI2_MOSI_GPIO_Port                     GPIOB

#define USER_KEY_Pin                            LL_GPIO_PIN_15
#define USER_KEY_GPIO_Port                      GPIOC
#define LED_STATE_Pin                           LL_GPIO_PIN_14
#define LED_STATE_GPIO_Port                     GPIOC
#define BOOT_KEY_Pin                            LL_GPIO_PIN_8
#define BOOT_KEY_GPIO_Port                      GPIOB

//Test point
#define TP1_Pin                                 LL_GPIO_PIN_1
#define TP1_GPIO_Port                           GPIOB


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


#define VIDEO1_INPUT_GAIN                       2
#define VIDEO2_INPUT_GAIN                       1
#define VIDEO_TOTAL_GAIN                        2

#define BOXID_CAM_SWITCH                        MSP_BOXID_CAMERA_CONTROL_1

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
void OPAMP6_Init(void);

void TIM1_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);
void TIM7_Init(void);
void TIM15_Init(void);
void TIM17_Init(void);

void COMP2_Init(void);
void COMP3_Init(void);

#endif /* __MAIN_H */
