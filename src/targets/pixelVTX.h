#pragma once


#define SPI2_CS_Pin                             LL_GPIO_PIN_7
#define SPI2_CS_GPIO_Port                       GPIOB
#define SPI2_SCK_Pin                            LL_GPIO_PIN_6
#define SPI2_SCK_GPIO_Port                      GPIOB
#define SPI2_MOSI_Pin                           LL_GPIO_PIN_9
#define SPI2_MOSI_GPIO_Port                     GPIOB

#define ADC_RESERVED_Pin                        LL_GPIO_PIN_14
#define ADC_RESERVED_GPIO_Port                  GPIOB
#define ADC_RESERVED_Channel                    LL_ADC_CHANNEL_5

#define ADC_PA_VDET_Pin                         LL_GPIO_PIN_0
#define ADC_PA_VDET_GPIO_Port                   GPIOB
#define ADC_PA_VDET_Channel                     LL_ADC_CHANNEL_15

#define USER_KEY_Pin                            LL_GPIO_PIN_15
#define USER_KEY_GPIO_Port                      GPIOC
#define LED_STATE_Pin                           LL_GPIO_PIN_13
#define LED_STATE_GPIO_Port                     GPIOC
#define LED2_Pin                                LL_GPIO_PIN_14
#define LED2_GPIO_Port                          GPIOC
#define BOOT_KEY_Pin                            LL_GPIO_PIN_8
#define BOOT_KEY_GPIO_Port                      GPIOB

//Test point
#define TP1_Pin                                 LL_GPIO_PIN_6
#define TP1_GPIO_Port                           GPIOA
#define TP2_Pin                                 LL_GPIO_PIN_1
#define TP2_GPIO_Port                           GPIOB

//Set input gain and total gain to 2 if FMS6141 video filter is used
#define VIDEO_TOTAL_GAIN                        2

// Video input 1 PA3
#define VIDEO1_INPUT_ENABLED                    false
#define VIDEO1_INPUT_GAIN                       2

// Video input 2 PA7
#define VIDEO2_INPUT_ENABLED                    true
#define VIDEO2_INPUT_GAIN                       1


#define USE_COLOR                               1