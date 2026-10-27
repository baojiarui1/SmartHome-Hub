#include "uart_stm.h"
#include "sys_messages.h"
#include "application.h"
#include "sensor_hub_task.h"
#include <esp_log.h>
#include <string_view>

static const char *TAG = "SENSOR_HUB";

// ==========================================
// 自定义音频
// ==========================================

//温度告警
extern const char temp_alarm_ogg_start[] asm("_binary_temp_alarm_ogg_start");
extern const char temp_alarm_ogg_end[]   asm("_binary_temp_alarm_ogg_end");
//危险气体告警
extern const char gas_alarm_ogg_start[] asm("_binary_gas_alarm_ogg_start");
extern const char gas_alarm_ogg_end[]   asm("_binary_gas_alarm_ogg_end");

static const std::string_view MY_TEMP_SOUND(temp_alarm_ogg_start, temp_alarm_ogg_end - temp_alarm_ogg_start);
static const std::string_view MY_GAS_SOUND(gas_alarm_ogg_start, gas_alarm_ogg_end - gas_alarm_ogg_start);

// ================================================
// 串口数据处理任务：处理uart_stm端收到的完整数据帧
// ================================================
void SensorHubTask(void *pvParameters) {
    UartDataPacket_t packet;
    
    while (1) {
        if (xQueueReceive(UartStm::uart_queue, &packet, portMAX_DELAY) == pdTRUE) {
            
            uint8_t key = packet.payload[0]; 
            uint8_t len = packet.payload[1];
            
            ESP_LOGI(TAG, "收到合法数据包: Key=0x%02X", key);

            switch (key) {
                case 0x01: { // 温度报警
                    uint8_t temp_val = packet.payload[2];
                    ESP_LOGW(TAG, "温度过高: %d 度", temp_val);
                    Application::GetInstance().Alert("WARNING", "High Temp!", "sad", MY_TEMP_SOUND);
                    vTaskDelay(pdMS_TO_TICKS(4000)); 
                    Application::GetInstance().DismissAlert();
                    break;
                }
                case 0x02: { // 燃气报警
                    ESP_LOGW(TAG, "燃气泄漏!");
                    Application::GetInstance().Alert("DANGER", "Gas Leak!", "sad", MY_GAS_SOUND);
                    vTaskDelay(pdMS_TO_TICKS(4000)); 
                    Application::GetInstance().DismissAlert();
                    break;
                }
                default:
                    break;
            }
        }
    }
}