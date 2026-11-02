#include "stm32f10x.h"
#include "dht11.h"
#include "Delay.h" 

// 引脚输入/输出模式切换
static void DHT11_SetPinOut(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

static void DHT11_SetPinIn(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

void DHT11_Init(void) {
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);
    DHT11_SetPinOut();
    GPIO_SetBits(DHT11_PORT, DHT11_PIN); // 默认拉高
}

// 读取一个字节
static uint8_t DHT11_ReadByte(void) {
    uint8_t i, data = 0;
    for (i = 0; i < 8; i++) {
        // 等待低电平结束
        while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_RESET);
        
        // 延时 40us，判断高电平持续时间
        Delay_us(40);
        
        // 如果 40us 后还是高电平，说明数据是 1
        if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_SET) {
            data |= (1 << (7 - i));
            // 等待高电平结束
            while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_SET);
        }
    }
    return data;
}

// 读取温湿度 (返回 0 成功，1 失败)
uint8_t DHT11_ReadData(DHT11_Data_t *data) {
    uint8_t buf[5]; // DHT11 一次发 5 个字节：湿度整数, 湿度小数, 温度整数, 温度小数, 校验和
    uint8_t i;
    
    // 1. 主机发送开始信号
    DHT11_SetPinOut();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_ms(20); // 拉低至少 18ms
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30); // 拉高 20~40us
    
    // 2. 主机切换为输入模式，等待 DHT11 响应
    DHT11_SetPinIn();
    
    // 等待 DHT11 拉低 (响应信号)
    if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_SET) return 1; // 超时错误
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_RESET);
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == Bit_SET);
    
    // 3. 读取 40 bit 数据 (5个字节)
    for (i = 0; i < 5; i++) {
        buf[i] = DHT11_ReadByte();
    }
    
    // 4. 校验数据 (前四个字节相加，应该等于第五个字节)
    if (buf[0] + buf[1] + buf[2] + buf[3] == buf[4]) {
        data->humi_int = buf[0];
        data->humi_dec = buf[1];
        data->temp_int = buf[2];
        data->temp_dec  = buf[3];
        return 0; // 成功
    }
    
    return 1; // 校验失败
}
