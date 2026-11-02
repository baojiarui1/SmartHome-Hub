#ifndef __UART_TO_ESP_H
#define __UART_TO_ESP_H

#include "stm32f10x.h"

// 协议帧定义
#define FRAME_HEAD   0x42
#define FRAME_TAIL   0x52
#define CMD_STM_UP   0x01 // STM32 上报
#define CMD_ESP_DOWN 0x02 // ESP 下发
#define KEY_TEMP     0x01 // 温度
#define KEY_HUMI     0x02 // 湿度
#define KEY_LIGHT    0x03 // 照明
#define KEY_ALARM    0x0A // 告警


#define UART_RX_BUF_SIZE  32  // 接收和 CRC 运算的缓冲区大小
#define UART_MAX_PAYLOAD  30  // 有效数据最大长度
#define UART_HEADER_LEN   2   // 命令码 + 长度字节数
#define UART_V_IDX_0      0   // 数组下标 0
#define UART_V_IDX_1      1   // 数组下标 1
#define UART_V_IDX_2      2   // 数组下标 2

// 接收数据包结构体
typedef struct {
    uint8_t cmd;                       // 命令码
    uint8_t len;                       // 数据长度
    uint8_t payload[UART_RX_BUF_SIZE]; // 数据内容 (KLV)
} UartRxPacket_t;


typedef enum {
    STATE_WAIT_HEAD, 
    STATE_WAIT_CMD, 
    STATE_WAIT_LEN,
    STATE_READ_DATA, 
    STATE_WAIT_CRC, 
    STATE_WAIT_TAIL
} ParseState_t;

void UART_ESP_Send(uint8_t cmd, uint8_t *data_content, uint8_t content_len);
void UART_ESP_RxCallback(uint8_t data);

#endif
