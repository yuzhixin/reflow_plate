#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include "gd32e23x.h"

/* =========================================================================
 * 硬件引脚定义 (基于找羊加热台仿制版)
 * ========================================================================= */
// SSR 控制引脚
#define SSR_PORT GPIOB
#define SSR_PIN  GPIO_PIN_8

// 蜂鸣器引脚 (无源蜂鸣器, 需复用为 PWM 输出)
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_PIN_8

// EC11 旋钮
#define EC11_PORT GPIOA
#define EC11_A_PIN GPIO_PIN_1
#define EC11_B_PIN GPIO_PIN_2
#define EC11_SW_PIN GPIO_PIN_0

// SPI 共享时钟
#define SPI_PORT GPIOA
#define SPI_SCK_PIN GPIO_PIN_5

// MAX6675 引脚
#define MAX_CS_PORT GPIOB
#define MAX_CS_PIN  GPIO_PIN_10
#define MAX_SO_PORT GPIOA
#define MAX_SO_PIN  GPIO_PIN_6

// TFT 屏幕引脚
#define TFT_CS_PORT GPIOA
#define TFT_CS_PIN  GPIO_PIN_4
#define TFT_SDA_PORT GPIOA
#define TFT_SDA_PIN  GPIO_PIN_7
#define TFT_RS_PORT GPIOB
#define TFT_RS_PIN  GPIO_PIN_0
#define TFT_RES_PORT GPIOB
#define TFT_RES_PIN  GPIO_PIN_1

// 风扇控制引脚
#define FAN_PORT GPIOB
#define FAN_PIN  GPIO_PIN_9

#endif /* HW_CONFIG_H */
