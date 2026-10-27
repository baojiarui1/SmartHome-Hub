#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// 告诉 UI 刷新界面的数据结构
typedef struct {
    float temp;
    float humi;
} UiUpdateMsg_t;

// 告诉底层控制 STM32 的指令结构
typedef struct {
    uint8_t key;
    uint8_t value;
} ControlCmd_t;

// 声明业务层的队列（在 main.cc 里统一创建）
extern QueueHandle_t queue_to_ui;
extern QueueHandle_t queue_to_control;