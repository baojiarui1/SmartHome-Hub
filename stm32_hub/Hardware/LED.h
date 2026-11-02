#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

// 定义 LED 所在端口和引脚
#define LED1_PORT   GPIOA
#define LED1_PIN    GPIO_Pin_1
#define LED2_PORT   GPIOA
#define LED2_PIN    GPIO_Pin_2

#define LED3_PORT   GPIOB
#define LED3_PIN    GPIO_Pin_12
#define LED4_PORT   GPIOB
#define LED4_PIN    GPIO_Pin_13
#define LED5_PORT   GPIOB
#define LED5_PIN    GPIO_Pin_14

#define LED_STATE_ON  0  // 低电平点亮
#define LED_STATE_OFF 1  // 高电平熄灭

// 原有的 LED 接口 (PA1, PA2)
void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED1_Turn(void);
void LED2_ON(void);
void LED2_OFF(void);
void LED2_Turn(void);

// 新增的 LED 接口 (PB12, PB13, PB14)
void LED3_ON(void);
void LED3_OFF(void);
void LED3_Turn(void);
void LED4_ON(void);
void LED4_OFF(void);
void LED4_Turn(void);
void LED5_ON(void);
void LED5_OFF(void);
void LED5_Turn(void);

#endif
