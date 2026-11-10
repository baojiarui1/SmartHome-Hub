#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "dht11.h"
#include "uart_to_esp.h"                // 通信协议层头文件
#include "LED.h"
#include <rtthread.h>                   // 引入 RT-Thread 头文件

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
#define TEMP_ALARM_THRESHOLD 22        // 触发告警的温度阈值
#define ALARM_STATUS_NONE    0x00      // 无告警
#define ALARM_STATUS_TEMP    0x01      // 温度告警

// 协议数据长度与索引
#define TX_BUF_MAX_LEN       32        // 发送缓冲区最大长度
#define KLV_LEN_BYTE         2         // 温湿度数据 V 的长度 (2字节)
#define KLV_LEN_ALARM        1         // 告警数据 V 的长度 (1字节)

// RT-Thread 线程与队列配置
#define THREAD_STACK_SIZE    2048      // 给足 2KB 栈空间刷屏幕
#define THREAD_PRIORITY      10        // 线程优先级
#define SENSOR_READ_DELAY    1500      // 传感器读取周期 (ms)


// 定义传递给队列的数据结构体
typedef struct {
    DHT11_Data_t dht_data;
    uint8_t alarm_status;
} SensorMessage_t;

// 全局消息队列句柄
static rt_mq_t sensor_mq = RT_NULL;

// ================== 线程入口函数声明 ==================
static void sensor_thread_entry(void *parameter);
static void comm_thread_entry(void *parameter);

int main(void)
{
    // 1. 硬件外设初始化 (放在 main 里，此时调度器未启动，保证 OLED 时序)
    LED_Init();    
    Serial_Init(); 
    
    // 2. 创建消息队列
    sensor_mq = rt_mq_create("sensor_mq", sizeof(SensorMessage_t), 5, RT_IPC_FLAG_FIFO);
    if (sensor_mq == RT_NULL) {
        while(1); // 队列失败死循环
    }

    // 3. 创建线程 1：传感器采集与显示
    rt_thread_t tid_sensor = rt_thread_create("sensor", 
                                              sensor_thread_entry, 
                                              RT_NULL, 
                                              THREAD_STACK_SIZE, 
                                              THREAD_PRIORITY, 
                                              10);
    if (tid_sensor != RT_NULL) {
        rt_thread_startup(tid_sensor);
    }

    // 4. 创建线程 2：串口通信发送 (优先级略高一点)
    rt_thread_t tid_comm = rt_thread_create("comm", 
                                            comm_thread_entry, 
                                            RT_NULL, 
                                            THREAD_STACK_SIZE, 
                                            THREAD_PRIORITY + 1,
                                            10);
    if (tid_comm != RT_NULL) {
        rt_thread_startup(tid_comm);
    }

    return 0; // 自动启动调度器
}

// ================== 线程1：传感器采集与显示 ==================
static void sensor_thread_entry(void *parameter)
{
    // 把 OLED 和 DHT11 的初始化放在线程里，以免影响 RTOS 启动
    OLED_Init();   
    DHT11_Init();  
    
    // 屏幕静态 UI 框架
    OLED_Clear();
    OLED_ShowString(ROW_TITLE, COL_TITLE, "Smart Home Hub"); 
    OLED_ShowString(ROW_TEMP, COL_TEMP_LABEL, "Temp:    C");     
    OLED_ShowString(ROW_HUMI, COL_HUMI_LABEL, "Humi:    %");     
    OLED_ShowString(ROW_STATUS, COL_STATUS_LABEL, "Status: OK  ");   

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
            
            // 把数据塞进消息队列
            msg.dht_data = dht_data;
            rt_mq_send(sensor_mq, &msg, sizeof(SensorMessage_t));
        } 
        else 
        {
            OLED_ShowString(ROW_STATUS, COL_STATUS_VAL, "Error ");
        }

        // 让出 CPU，休息 1.5 秒
        rt_thread_mdelay(SENSOR_READ_DELAY); 
    }
}

// ================== 线程2：串口通信发送 ==================
static void comm_thread_entry(void *parameter)
{
    SensorMessage_t msg;
    uint8_t tx_buffer[TX_BUF_MAX_LEN];
    uint8_t tx_index = 0;

    while (1)
    {
        // 阻塞等待队列里的数据，没有数据就休眠
        if (rt_mq_recv(sensor_mq, &msg, sizeof(SensorMessage_t), RT_WAITING_FOREVER) == RT_EOK)
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

            // 发送给 ESP32
            UART_ESP_Send(CMD_STM_UP, tx_buffer, tx_index);
        }
    }
}