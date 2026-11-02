#ifndef __DHT11_H
#define __DHT11_H

// 定义引脚
#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_Pin_1
#define DHT11_RCC  RCC_APB2Periph_GPIOA

// 结构体：存放温湿度数据
typedef struct {
    uint8_t temp_int;   // 温度整数部分
    uint8_t temp_dec;   // 温度小数部分
    uint8_t humi_int;   // 湿度整数部分
    uint8_t humi_dec;   // 湿度小数部分
} DHT11_Data_t;

// 函数声明
void DHT11_Init(void);
uint8_t DHT11_ReadData(DHT11_Data_t *data);

#endif
