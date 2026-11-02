#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 照明字段
#define LIGHT_V_LEN     1    // 照明控制指令中 V 的长度
#define LIGHT_ID_1      0x01 // 照明1
#define LIGHT_ID_2      0x02 // 照明2
#define LIGHT_ID_3      0x04 // 照明3
#define LIGHT_ID_ALL    0x07 // 全部照明(1+2+3)

#define LIGHT_CMD_LEN   3    // 控制指令的长度 (K+L+V)
#define LIGHT_CRC_LEN   5    // 需要计算 CRC 的长度 (命令码+总长度+K+L+V)

// 注册 AI 控制工具的接口
void RegisterAiLightControl();

#ifdef __cplusplus
}
#endif