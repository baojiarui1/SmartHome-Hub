#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "application.h"
#include "system_info.h"
#include "uart_stm.h"
#include "sensor_hub_task.h"
#include "ai_mcp_bridge.h"

#define TAG "main"

extern "C" void app_main(void)
{
    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    
    // 初始化串口
    UartStm::Init();

    // 串口数据处理任务
    xTaskCreate(UartStm::Task, "uart_stm_task", 4096, NULL, 5, NULL);

    // 传感器数据处理任务
    xTaskCreate(SensorHubTask, "sensor_hub", 4096, NULL, 4, NULL);

    // 注册 AI 控灯工具
    RegisterAiLightControl();

    // Launch the application
    auto& app = Application::GetInstance();
    app.Start();

    
}
