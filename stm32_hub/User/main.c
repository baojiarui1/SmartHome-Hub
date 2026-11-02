#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "dht11.h"
#include "uart_to_esp.h"                // 通信协议层头文件
#include "LED.h"


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
#define TEMP_ALARM_THRESHOLD 25        // 触发告警的温度阈值
#define ALARM_STATUS_NONE    0x00      // 无告警
#define ALARM_STATUS_TEMP    0x01      // 温度告警

// 协议数据长度与索引
#define TX_BUF_MAX_LEN       32        // 发送缓冲区最大长度
#define KLV_LEN_BYTE         2         // 温湿度数据 V 的长度 (2字节)
#define KLV_LEN_ALARM        1         // 告警数据 V 的长度 (1字节)
// ==================================================

int main(void)
{
    // 1. 初始化外设
    OLED_Init();   
    DHT11_Init();  
    Serial_Init(); 
    LED_Init();    
    
    DHT11_Data_t dht_data;
    
    // 2. 屏幕静态 UI 框架
    OLED_Clear();
    OLED_ShowString(ROW_TITLE, COL_TITLE, "Smart Home Hub"); 
    OLED_ShowString(ROW_TEMP, COL_TEMP_LABEL, "Temp:    C");     
    OLED_ShowString(ROW_HUMI, COL_HUMI_LABEL, "Humi:    %");     
    OLED_ShowString(ROW_STATUS, COL_STATUS_LABEL, "Status: OK  ");   

    while (1)
    {
        // 3. 读取 DHT11 数据
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
            uint8_t alarm_status = ALARM_STATUS_NONE; // 默认无告警
            if (dht_data.temp_int >= TEMP_ALARM_THRESHOLD) {
                alarm_status = ALARM_STATUS_TEMP; // 触发温度过高告警
                OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "ALARM ");
            } else {
                OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "OK    "); 
            }
            
            // --- 组装 KLV 数据包 ---
            uint8_t tx_buffer[TX_BUF_MAX_LEN];
            uint8_t tx_index = 0;

            // K1: 温度
            tx_buffer[tx_index++] = KEY_TEMP; 
            tx_buffer[tx_index++] = KLV_LEN_BYTE; 
            tx_buffer[tx_index++] = dht_data.temp_int; 
            tx_buffer[tx_index++] = dht_data.temp_dec; 

            // K2: 湿度
            tx_buffer[tx_index++] = KEY_HUMI; 
            tx_buffer[tx_index++] = KLV_LEN_BYTE; 
            tx_buffer[tx_index++] = dht_data.humi_int; 
            tx_buffer[tx_index++] = dht_data.humi_dec; 
            
            // K3: 告警状态 
            tx_buffer[tx_index++] = KEY_ALARM; 
            tx_buffer[tx_index++] = KLV_LEN_ALARM; 
            tx_buffer[tx_index++] = alarm_status; 

            // 4. 发送所有数据
            UART_ESP_Send(CMD_STM_UP, tx_buffer, tx_index);
            
        } 
        else 
        {
            // 读取失败
            OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "Error ");
        }

        // 延时 1.5 秒
        Delay_ms(1500); 
    }
}
