#include "uart_stm.h"
#include "sys_messages.h"
#include "application.h"
#include "sensor_hub_task.h"
#include <esp_log.h>
#include <string_view>
#include "freertos/timers.h"

static const char *TAG = "SENSOR_HUB";

// 自定义音频 音频路径为main/assets/custom
// 在 CMakeLists.txt 里把 .ogg 文件加进 EMBED_FILES 时，
// ESP-IDF 底层的编译器会自动把这个音频文件转成一大堆十六进制的机器码，并按顺序塞进 Flash 里
// 编译器会给这段数据的开头自动打上一个标签 _binary_文件名_start，给结尾打上 _binary_文件名_end
extern const char temp_alarm_ogg_start[] asm("_binary_temp_alarm_ogg_start");
extern const char temp_alarm_ogg_end[]   asm("_binary_temp_alarm_ogg_end");
extern const char gas_alarm_ogg_start[] asm("_binary_gas_alarm_ogg_start");
extern const char gas_alarm_ogg_end[]   asm("_binary_gas_alarm_ogg_end");

// 记录要播放音频的起始地址和长度
static const std::string_view MY_TEMP_SOUND(temp_alarm_ogg_start, temp_alarm_ogg_end - temp_alarm_ogg_start);
static const std::string_view MY_GAS_SOUND(gas_alarm_ogg_start, gas_alarm_ogg_end - gas_alarm_ogg_start);

static TimerHandle_t alarm_timer = NULL;

// 定时器任务：解除告警
static void AlarmTimeoutCallback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "4秒定时结束，解除告警 UI");
    Application::GetInstance().DismissAlert();
}

// 告警处理函数
static void HandleAlarm(uint8_t alarm_val, uint8_t *current_state) {
    
    //初始化定时器
    if (alarm_timer == NULL) {
        alarm_timer = xTimerCreate(
            "AlarmTimer", 
            pdMS_TO_TICKS(4000), // 4秒周期
            pdFALSE,             // pdFALSE 代表单次定时，不循环
            (void *)0, 
            AlarmTimeoutCallback 
        );
    }

    if (alarm_val == ALARM_TEMP) {
        if (*current_state != ALARM_TEMP) {
            ESP_LOGW(TAG, "触发高温告警！");
            *current_state = ALARM_TEMP; 
            
            // 触发告警UI
            Application::GetInstance().Alert("WARNING", "High Temp!", "sad", MY_TEMP_SOUND);
        
            // 启动定时器
            if (alarm_timer != NULL) {
                xTimerStart(alarm_timer, 0); 
            }
        }
    } else if (alarm_val == ALARM_NONE) {
        if (*current_state != ALARM_NONE) {
            ESP_LOGI(TAG, "温度恢复正常，解除告警");
            *current_state = ALARM_NONE; 
            
            // 停止定时器
            if (alarm_timer != NULL) {
                xTimerStop(alarm_timer, 10);
            }
            
            // 解除告警UI
            Application::GetInstance().DismissAlert();
        }
    }
}

// 传感器数据处理任务
void SensorHubTask(void *pvParameters) {
    UartDataPacket_t packet;
    
    // 记录当前是否处于告警状态 (0=正常, 1=温度告警, 2=气体告警)
    uint8_t current_alarm_state = ALARM_NONE; 
    
    while (1) {
        if (xQueueReceive(UartStm::uart_queue, &packet, portMAX_DELAY) == pdTRUE) {
            
            if (packet.cmd != CMD_STM_UP) {
                continue;
            }

            // 遍历 KLV 数据区
            uint8_t ptr = 0;
            uint8_t alarm_val = ALARM_NONE;
            bool has_alarm_data = false;

            while (ptr + KLV_K_LEN + KLV_L_LEN <= packet.len) {
                uint8_t k = packet.payload[ptr];       
                uint8_t v_len = packet.payload[ptr + KLV_K_LEN]; 
                uint8_t *v = &packet.payload[ptr + KLV_K_LEN + KLV_L_LEN];   
                
                if (ptr + KLV_K_LEN + KLV_L_LEN + v_len > packet.len) {
                    ESP_LOGW(TAG, "数据包异常，KLV长度越界！丢弃该包");
                    break; 
                }
                
                if (k == KEY_TEMP) { 
                    if (v_len >= TEMP_VALUE_LEN) { 
                        SensorVal_t *temp = (SensorVal_t *)v;
                        ESP_LOGI(TAG, "🌡️ 温度: %d.%d C", temp->integer, temp->decimal);
                    }
                } else if (k == KEY_HUMI) { 
                    if (v_len >= HUMI_VALUE_LEN) { 
                        SensorVal_t *humi = (SensorVal_t *)v;
                        ESP_LOGI(TAG, "💧 湿度: %d.%d %%", humi->integer, humi->decimal);
                    }
                } else if (k == KEY_ALARM) { 
                    if (v_len >= ALARM_VALUE_LEN) { 
                        alarm_val = v[KLV_V_FIRST_IDX]; 
                        has_alarm_data = true;
                    }
                }
                ptr += (KLV_K_LEN + KLV_L_LEN + v_len); 
            }

            // 告警防抖与控制逻辑
            if (has_alarm_data) {
                HandleAlarm(alarm_val, &current_alarm_state);
            }
        }
    }
}