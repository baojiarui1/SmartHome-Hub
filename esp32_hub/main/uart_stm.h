#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// UART配置
#define STM_UART_PORT   UART_NUM_1
#define STM_TXD_PIN     GPIO_NUM_18
#define STM_RXD_PIN     GPIO_NUM_8
#define UART_BAUD_RATE  9600  

// 消灭魔法数字的宏定义
#define UART_READ_SIZE          1       // 每次读取的字节数
#define UART_QUEUE_LENGTH       5       // 队列最多存放的数据包数量
#define UART_QUEUE_SEND_TICKS   10      // 发送到队列的超时时间(ms)
#define UART_RX_BUF_SIZE        2048    // 底层接收环形缓冲区大小
#define CRC_HEADER_OFFSET_LEN   2       // CRC计算时前面的附加长度（命令码+数据长度）


// 数据帧格式：帧头 命令码 数据长度 数据内容（KLV） 校验码 帧尾
#define FRAME_HEAD      0x42  // 'B'
#define FRAME_TAIL      0x52  // 'R'
#define MAX_PAYLOAD_LEN 32    // 数据段最大长度
#define CMD_STM_UP      0x01  // STM32 上报
#define CMD_ESP_DOWN    0x02  // ESP32 下发

// 数据内容
#define KEY_TEMP        0x01  // 温度 (L=2, V=整数+小数)
#define KEY_HUMI        0x02  // 湿度 (L=2, V=整数+小数)
#define KEY_LIGHT       0x03  // 照明 (L=1, V=位掩码 0x00~0x07)
#define KEY_FAN         0x04  // 风扇 (L=1, V=0/1)
#define KEY_ALARM       0x0A  // 告警 (L=1, V=0/1/2)

// 状态机
enum UartParseState {
    STATE_WAIT_HEAD = 0,    // 等待帧头 'B' (0x42)
    STATE_READ_CMD,         // 读取命令码
    STATE_READ_LEN,         // 读取数据长度
    STATE_READ_PAYLOAD,     // 读取 KLV 数据内容
    STATE_CHECK_CRC,        // 校验 CRC-8
    STATE_WAIT_TAIL         // 等待帧尾 'R' (0x52)
};

// 数据包
typedef struct {
    uint8_t cmd;                       // 命令码 (0x01 / 0x02)
    uint8_t len;                       // 数据内容 (KLV) 的总长度
    uint8_t payload[MAX_PAYLOAD_LEN];  // 存放数据内容 (KLV)
} UartDataPacket_t;


uint8_t Calculate_CRC8(uint8_t *data, uint8_t len);

class UartStm {
public:
    static void Init();
    static void Task(void *pvParameters);
    static void SendData(const uint8_t* data, uint8_t len);
    static QueueHandle_t uart_queue; 
};