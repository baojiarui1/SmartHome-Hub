#include "uart_stm.h"
#include <esp_log.h>
#include <cstring>

static const char *TAG = "UART_STM";
QueueHandle_t UartStm::uart_queue = NULL;

// 校验码算法: 标准 CRC-8 
uint8_t Calculate_CRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void UartStm::Init() {
    ESP_LOGI(TAG, ">>> 开始初始化串口...");
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(STM_UART_PORT, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(STM_UART_PORT, &uart_config);
    uart_set_pin(STM_UART_PORT, STM_TXD_PIN, STM_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    ESP_LOGI(TAG, ">>> 串口初始化完成! 端口:%d, TX: IO%d, RX: IO%d", STM_UART_PORT, STM_TXD_PIN, STM_RXD_PIN);

    // 初始化队列，解析到完整数据帧后，通过队列发送到业务层
    uart_queue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(UartDataPacket_t));
    
    if (uart_queue != NULL) {
        ESP_LOGI(TAG, ">>> UART 通信层初始化完成，队列已创建");
    } else {
        ESP_LOGE(TAG, ">>> UART 队列创建失败！内存不足");
    }
}

// 提供给上层业务调用的发送接口
void UartStm::SendData(const uint8_t* data, uint8_t len) {
    if (data != NULL && len > 0) {
        uart_write_bytes(STM_UART_PORT, (const char*)data, len);
    }
}

// 从串口底层获取数据，经过缓冲区和状态机得到完整的数据帧，通过队列发给SensorHubTask进行处理
void UartStm::Task(void *pvParameters) {
    uint8_t byte;
    UartParseState state = STATE_WAIT_HEAD; 
    
    // 用来存储正在解析的包
    UartDataPacket_t packet; 
    uint8_t payload_index = 0;
    
    while (1) {
        if (uart_read_bytes(STM_UART_PORT, &byte, UART_READ_SIZE, portMAX_DELAY) == UART_READ_SIZE) {
            switch (state) {
                case STATE_WAIT_HEAD: 
                    if (byte == FRAME_HEAD) { 
                        state = STATE_READ_CMD;
                    }
                    break;
                    
                case STATE_READ_CMD: 
                    packet.cmd = byte;
                    state = STATE_READ_LEN;
                    break;
                    
                case STATE_READ_LEN: 
                    packet.len = byte; 
                    if (packet.len > MAX_PAYLOAD_LEN || packet.len == 0) {
                        state = STATE_WAIT_HEAD; 
                    } else {
                        state = STATE_READ_PAYLOAD;
                        payload_index = 0; // 重置索引
                    }
                    break;
                    
                case STATE_READ_PAYLOAD: 
                    packet.payload[payload_index++] = byte;
                    if (payload_index >= packet.len) {
                        state = STATE_CHECK_CRC;
                    }
                    break;
                    
                case STATE_CHECK_CRC: 
                    {
                        // 校验范围：命令码 + 数据长度 + 数据内容
                        uint8_t crc_data[MAX_PAYLOAD_LEN + CRC_HEADER_OFFSET_LEN];
                        crc_data[0] = packet.cmd;
                        crc_data[1] = packet.len;
                        memcpy(&crc_data[CRC_HEADER_OFFSET_LEN], packet.payload, packet.len);
                        
                        uint8_t calc_crc = Calculate_CRC8(crc_data, packet.len + CRC_HEADER_OFFSET_LEN);
                        
                        if (calc_crc == byte) {
                            state = STATE_WAIT_TAIL; 
                            // ESP_LOGW(TAG, "------CRC 校验通过，等帧尾------");
                        } else {
                            state = STATE_WAIT_HEAD; 
                            ESP_LOGW(TAG, "------CRC 校验失败!calc:%02X, recv:%02X------", calc_crc, byte);
                        }
                    }
                    break;
                    
                case STATE_WAIT_TAIL: 
                    if (byte == FRAME_TAIL) {
                        // 完整收到一包，扔进队列给业务层
                        if (xQueueSend(uart_queue, &packet, pdMS_TO_TICKS(UART_QUEUE_SEND_TICKS)) != pdTRUE) {
                            ESP_LOGW(TAG, "队列已满，丢弃一包数据");
                        } else {
                            // ESP_LOGW(TAG, "------成功向队列发送一包数据------");
                        }
                    } 
                    state = STATE_WAIT_HEAD; 
                    break;
                    
                default:
                    state = STATE_WAIT_HEAD;
                    break;
            }
        }
    }
}