#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ================= 通信协议宏定义 =================
#define STM_UART_PORT UART_NUM_1
#define STM_TXD_PIN   GPIO_NUM_18
#define STM_RXD_PIN   GPIO_NUM_8

// 鲍嘉睿专属协议
#define FRAME_HEAD    0x42  // 'B'
#define FRAME_TAIL    0x52  // 'R'
#define MAX_PAYLOAD_LEN 32  // 数据段最大长度，防止溢出

// 告警类型
#define KEY_TEMP_ALERT 0x01
#define KEY_GAS_ALERT  0x02

// ================= 状态机枚举定义 =================
enum UartParseState {
    STATE_WAIT_HEAD = 0,    // 等待帧头 'B'
    STATE_READ_LEN,         // 读取数据长度
    STATE_READ_PAYLOAD,     // 读取 KLV 数据内容
    STATE_CHECK_CRC,        // 校验 CRC-8
    STATE_WAIT_TAIL         // 等待帧尾 'R'
};

// 定义一个结构体，专门用于在队列中传递数据包
typedef struct {
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD_LEN];
} UartDataPacket_t;

// 声明 CRC-8 校验函数
uint8_t Calculate_CRC8(uint8_t *data, uint8_t len);

class UartStm {
public:
    static void Init();
    static void Task(void *pvParameters);
    static void EchoTestTask(void *pvParameters);
    // static void SendData(const uint8_t* data, uint8_t len);
    static QueueHandle_t uart_queue; 
};