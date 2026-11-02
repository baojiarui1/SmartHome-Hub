#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_NONE      0x00 // 正常
#define ALARM_TEMP      0x01 // 温度告警
#define ALARM_GAS       0x02 // 气体告警

#define TEMP_VALUE_LEN  2    // 温度字段 V 的长度 (整数+小数)
#define HUMI_VALUE_LEN  2    // 湿度字段 V 的长度 (整数+小数)
#define ALARM_VALUE_LEN 1    // 告警字段 V 的长度

#define KLV_K_LEN       1    // 数据包中 Key 占用的字节数
#define KLV_L_LEN       1    // 数据包中 Length 占用的字节数
#define KLV_V_FIRST_IDX 0    // V (Value) 数组的第一个字节下标

// 专门用于温湿度的结构体
typedef struct __attribute__((packed)) {
    uint8_t integer; // 整数部分
    uint8_t decimal; // 小数部分
} SensorVal_t;

void SensorHubTask(void *pvParameters);

#ifdef __cplusplus
}
#endif