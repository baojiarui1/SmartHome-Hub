#include "uart_stm.h"
#include "application.h"
#include <esp_log.h>
#include "assets/lang_config.h"
#include <cJSON.h>

static const char *TAG = "UART_STM";
QueueHandle_t UartStm::uart_queue = NULL; 

// --- 标准 CRC-8 算法实现 (多项式 0x07) ---
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
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // 安装驱动并配置引脚
    uart_driver_install(STM_UART_PORT, 2048, 0, 0, NULL, 0);
    uart_param_config(STM_UART_PORT, &uart_config);
    uart_set_pin(STM_UART_PORT, STM_TXD_PIN, STM_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    ESP_LOGI(TAG, ">>> 串口初始化完成! 端口:%d, TX: IO%d, RX: IO%d", STM_UART_PORT, STM_TXD_PIN, STM_RXD_PIN);

    // 创建用于接收完整数据包的队列（最多存5个包）
    uart_queue = xQueueCreate(5, sizeof(UartDataPacket_t));
    
    if (uart_queue != NULL) {
        ESP_LOGI(TAG, ">>> UART 通信层初始化完成，队列已创建");
    } else {
        ESP_LOGE(TAG, ">>> UART 队列创建失败！内存不足");
    }
}

// 提供给上层业务调用的发送接口
// void UartStm::SendData(const uint8_t* data, uint8_t len) {
//     if (data != NULL && len > 0) {
//         uart_write_bytes(STM_UART_PORT, (const char*)data, len);
//     }
// }

// ================== 回环测试任务 ==================
void UartStm::EchoTestTask(void *pvParameters) {
    ESP_LOGI(TAG, ">>> 回环测试任务已启动，进入监听循环...");
    uint8_t *data = (uint8_t *) malloc(256);
    
    while (1) {
        // 这里是阻塞读取，如果没有数据，它会在这里死等，不会消耗 CPU
        int len = uart_read_bytes(STM_UART_PORT, data, 256, pdMS_TO_TICKS(100));
        
        if (len > 0) {
            // 如果收到数据，立刻打印日志并回发
            ESP_LOGW(TAG, ">>> 【收到数据】: 长度 %d 字节，内容: %.*s", len, len, data);
            
            uart_write_bytes(STM_UART_PORT, (const char *) data, len);
            ESP_LOGI(TAG, ">>> 【已回发】");
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void UartStm::Task(void *pvParameters) {
    uint8_t byte;
    UartParseState state = STATE_WAIT_HEAD; 
    
    //定义了用于发送到队列的完整结构体
    UartDataPacket_t packet; 
    uint8_t payload_index = 0;
    
    while (1) {
        if (uart_read_bytes(STM_UART_PORT, &byte, 1, portMAX_DELAY) == 1) {
            switch (state) {
                case STATE_WAIT_HEAD: // 找帧头
                    if (byte == FRAME_HEAD) { 
                        state = STATE_READ_LEN; 
                        payload_index = 0;
                    }
                    break;
                    
                case STATE_READ_LEN: // 读数据长度
                    packet.len = byte; 
                    if (packet.len > MAX_PAYLOAD_LEN || packet.len == 0) {
                        state = STATE_WAIT_HEAD; // 长度离谱，重来
                    } else {
                        state = STATE_READ_PAYLOAD;
                    }
                    break;
                    
                case STATE_READ_PAYLOAD: // 读数据内容 (KLV)
                    // 存下当前字节
                    packet.payload[payload_index] = byte;

                    payload_index++;

                    // 判断是否收齐了所有数据
                    if (payload_index >= packet.len) {
                        state = STATE_CHECK_CRC;
                    }
                    break;
                    
                case STATE_CHECK_CRC: // 读 CRC 校验码
                    {
                        uint8_t calc_crc = Calculate_CRC8(packet.payload, packet.len);
                        if (calc_crc == byte) {
                            state = STATE_WAIT_TAIL; 
                            ESP_LOGW(TAG, "------CRC 校验通过，等帧尾------");
                        } else {
                            state = STATE_WAIT_HEAD; 
                            ESP_LOGW(TAG, "------CRC 校验失败!calc:%02X, recv:%02X------", calc_crc, byte);
                        }
                    }
                    break;
                    
                case STATE_WAIT_TAIL: 
                    if (byte == FRAME_TAIL) {
                        // 🎯 完整收到一包，扔进队列给业务层
                        // 如果队列满了，xQueueSend 返回失败，这里加个延时防止阻塞
                        ESP_LOGW(TAG, "------向队列中发送数据------");
                        if (xQueueSend(uart_queue, &packet, pdMS_TO_TICKS(10)) != pdTRUE) {
                            ESP_LOGW(TAG, "队列已满，丢弃一包数据");
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