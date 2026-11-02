#include "stm32f10x.h"                  // Device header
#include "LED.h"

void LED_Init(void)
{
	// 开启 GPIOA 和 GPIOB 的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	// 初始化 PA1, PA2
	GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN;
	GPIO_Init(LED1_PORT, &GPIO_InitStructure);
	
	// 初始化 PB12, PB13, PB14
	GPIO_InitStructure.GPIO_Pin = LED3_PIN | LED4_PIN | LED5_PIN;
	GPIO_Init(LED3_PORT, &GPIO_InitStructure);
	
	// 默认全部关闭（低电平点亮，所以初始给高电平）
	GPIO_WriteBit(LED1_PORT, LED1_PIN | LED2_PIN, (BitAction)LED_STATE_OFF);
	GPIO_WriteBit(LED3_PORT, LED3_PIN | LED4_PIN | LED5_PIN, (BitAction)LED_STATE_OFF);
}

void LED1_ON(void) { GPIO_WriteBit(LED1_PORT, LED1_PIN, (BitAction)LED_STATE_ON); }
void LED1_OFF(void) { GPIO_WriteBit(LED1_PORT, LED1_PIN, (BitAction)LED_STATE_OFF); }
void LED1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED1_PORT, LED1_PIN) == LED_STATE_ON) 
        GPIO_WriteBit(LED1_PORT, LED1_PIN, (BitAction)LED_STATE_OFF);
	else 
        GPIO_WriteBit(LED1_PORT, LED1_PIN, (BitAction)LED_STATE_ON);
}

void LED2_ON(void) { GPIO_WriteBit(LED2_PORT, LED2_PIN, (BitAction)LED_STATE_ON); }
void LED2_OFF(void) { GPIO_WriteBit(LED2_PORT, LED2_PIN, (BitAction)LED_STATE_OFF); }
void LED2_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED2_PORT, LED2_PIN) == LED_STATE_ON) 
        GPIO_WriteBit(LED2_PORT, LED2_PIN, (BitAction)LED_STATE_OFF);
	else 
        GPIO_WriteBit(LED2_PORT, LED2_PIN, (BitAction)LED_STATE_ON);
}

void LED3_ON(void) { GPIO_WriteBit(LED3_PORT, LED3_PIN, (BitAction)LED_STATE_ON); }
void LED3_OFF(void) { GPIO_WriteBit(LED3_PORT, LED3_PIN, (BitAction)LED_STATE_OFF); }
void LED3_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED3_PORT, LED3_PIN) == LED_STATE_ON) 
        GPIO_WriteBit(LED3_PORT, LED3_PIN, (BitAction)LED_STATE_OFF);
	else 
        GPIO_WriteBit(LED3_PORT, LED3_PIN, (BitAction)LED_STATE_ON);
}

void LED4_ON(void) { GPIO_WriteBit(LED4_PORT, LED4_PIN, (BitAction)LED_STATE_ON); }
void LED4_OFF(void) { GPIO_WriteBit(LED4_PORT, LED4_PIN, (BitAction)LED_STATE_OFF); }
void LED4_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED4_PORT, LED4_PIN) == LED_STATE_ON) 
        GPIO_WriteBit(LED4_PORT, LED4_PIN, (BitAction)LED_STATE_OFF);
	else 
        GPIO_WriteBit(LED4_PORT, LED4_PIN, (BitAction)LED_STATE_ON);
}

void LED5_ON(void) { GPIO_WriteBit(LED5_PORT, LED5_PIN, (BitAction)LED_STATE_ON); }
void LED5_OFF(void) { GPIO_WriteBit(LED5_PORT, LED5_PIN, (BitAction)LED_STATE_OFF); }
void LED5_Turn(void)
{
	if (GPIO_ReadOutputDataBit(LED5_PORT, LED5_PIN) == LED_STATE_ON) 
        GPIO_WriteBit(LED5_PORT, LED5_PIN, (BitAction)LED_STATE_OFF);
	else 
        GPIO_WriteBit(LED5_PORT, LED5_PIN, (BitAction)LED_STATE_ON);
}
