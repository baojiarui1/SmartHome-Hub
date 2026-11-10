#include "stm32f10x.h"                  
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "dht11.h"
#include "uart_to_esp.h"               
#include "LED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


// OLED 屏幕坐标定义
#define ROW_TITLE           1
#define COL_TITLE           1
#define ROW_TEMP            2
#define COL_TEMP_LABEL      1
#define COL_TEMP_VAL        7
#define COL_TEMP_DOT        9
#define COL_TEMP_DEC        10

#define ROW_HUMI            3
#define COL_HUMI_LABEL      1
#define COL_HUMI_VAL        7
#define COL_HUMI_DOT        9
#define COL_HUMI_DEC        10

#define ROW_STATUS          4
#define COL_STATUS_LABEL    1
#define COL_STATUS_VAL      9

// 业务阈值与状态
#define TEMP_ALARM_THRESHOLD 23        // 触发告警的温度阈值
#define ALARM_STATUS_NONE    0x00      // 无告警
#define ALARM_STATUS_TEMP    0x01      // 温度告警

// 协议数据长度与索引
#define TX_BUF_MAX_LEN       32        // 发送缓冲区最大长度
#define KLV_LEN_BYTE         2         // 温湿度数据 V 的长度 (2字节)
#define KLV_LEN_ALARM        1         // 告警数据 V 的长度 (1字节)

// FreeRTOS 任务与队列配置
#define TASK_STACK_SIZE      256       // 任务栈大小
#define QUEUE_LENGTH         5         // 队列长度
#define SENSOR_READ_DELAY    1500      // 传感器读取周期 (ms)
// ==================================================

// 定义传递给队列的数据结构体
typedef struct {
    DHT11_Data_t dht_data;
    uint8_t alarm_status;
} SensorMessage_t;

// 全局队列句柄
QueueHandle_t xSensorQueue;

// ================== 任务函数声明 ==================
void vSensorTask(void *pvParameters);
void vCommTask(void *pvParameters);

int main(void)
{
    // 1. 初始化外设
    OLED_Init();   
    DHT11_Init();  
    Serial_Init(); 
    LED_Init();    
    
    // 2. 屏幕静态 UI 框架
    OLED_Clear();
    OLED_ShowString(ROW_TITLE, COL_TITLE, "Smart Home Hub"); 
    OLED_ShowString(ROW_TEMP, COL_TEMP_LABEL, "Temp:    C");     
    OLED_ShowString(ROW_HUMI, COL_HUMI_LABEL, "Humi:    %");     
    OLED_ShowString(ROW_STATUS, COL_STATUS_LABEL, "Status: OK  ");   

    // 3. 创建队列
    xSensorQueue = xQueueCreate(QUEUE_LENGTH, sizeof(SensorMessage_t));

    if (xSensorQueue != NULL) {
        // 4. 创建任务
        // 任务1：传感器采集与显示 (优先级 2)
        xTaskCreate(vSensorTask, "SensorTask", TASK_STACK_SIZE, NULL, 2, NULL);
        // 任务2：串口通信发送 (优先级 1)
        xTaskCreate(vCommTask, "CommTask", TASK_STACK_SIZE, NULL, 1, NULL);

        // 5. 启动调度器
        vTaskStartScheduler();
    }

    // 如果队列创建失败或调度器返回，死循环防止意外
    while (1);
}

// ================== 任务1：传感器采集与显示 ==================
void vSensorTask(void *pvParameters)
{
    DHT11_Data_t dht_data;
    SensorMessage_t msg;

    while (1)
    {
        if (DHT11_ReadData(&dht_data) == 0) 
        {
            // --- 刷新屏幕温湿度 ---
            OLED_ShowNum(ROW_TEMP, COL_TEMP_VAL, dht_data.temp_int, 2); 
            OLED_ShowChar(ROW_TEMP, COL_TEMP_DOT, '.');
            OLED_ShowNum(ROW_TEMP, COL_TEMP_DEC, dht_data.temp_dec, 1);
            
            OLED_ShowNum(ROW_HUMI, COL_HUMI_VAL, dht_data.humi_int, 2); 
            OLED_ShowChar(ROW_HUMI, COL_HUMI_DOT, '.');
            OLED_ShowNum(ROW_HUMI, COL_HUMI_DEC, dht_data.humi_dec, 1);

            // --- 告警逻辑判断 ---
            msg.alarm_status = ALARM_STATUS_NONE;
            if (dht_data.temp_int >= TEMP_ALARM_THRESHOLD) {
                msg.alarm_status = ALARM_STATUS_TEMP;
                OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "ALARM ");
            } else {
                OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "OK    "); 
            }
            
            // 把温湿度数据存入结构体，准备发送到队列
            msg.dht_data = dht_data;
            
            // 发送到队列，如果队列满了最多等 10ms，不阻塞死等
            xQueueSend(xSensorQueue, &msg, pdMS_TO_TICKS(10));
        } 
        else 
        {
            OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "Error ");
        }

        // 使用 FreeRTOS 延时，让出 CPU
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_DELAY)); 
    }
}

// ================== 任务2：串口通信发送 ==================
void vCommTask(void *pvParameters)
{
    SensorMessage_t msg;
    uint8_t tx_buffer[TX_BUF_MAX_LEN];
    uint8_t tx_index = 0;

    while (1)
    {
        // 阻塞等待队列中的数据（如果没有数据，就一直休眠，不占用 CPU）
        if (xQueueReceive(xSensorQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            tx_index = 0;

            // K1: 温度
            tx_buffer[tx_index++] = KEY_TEMP; 
            tx_buffer[tx_index++] = KLV_LEN_BYTE; 
            tx_buffer[tx_index++] = msg.dht_data.temp_int; 
            tx_buffer[tx_index++] = msg.dht_data.temp_dec; 

            // K2: 湿度
            tx_buffer[tx_index++] = KEY_HUMI; 
            tx_buffer[tx_index++] = KLV_LEN_BYTE; 
            tx_buffer[tx_index++] = msg.dht_data.humi_int; 
            tx_buffer[tx_index++] = msg.dht_data.humi_dec; 
            
            // K3: 告警状态 
            tx_buffer[tx_index++] = KEY_ALARM; 
            tx_buffer[tx_index++] = KLV_LEN_ALARM; 
            tx_buffer[tx_index++] = msg.alarm_status; 

            // 发送所有数据给 ESP32
            UART_ESP_Send(CMD_STM_UP, tx_buffer, tx_index);
        }
    }
}
