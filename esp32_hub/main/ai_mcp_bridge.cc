#include "ai_mcp_bridge.h"
#include "mcp_server.h"
#include "uart_stm.h"
#include <esp_log.h>

static const char *TAG = "AI_MCP_BRIDGE";

// 这个函数是大模型调用工具时，实际执行的代码
ReturnValue ControlLight(const PropertyList& properties) {
    // 1. 从大模型传过来的参数里获取动作和目标
    auto action = properties["action"].value<std::string>(); // "on" 或 "off"
    auto target = properties["target"].value<std::string>(); // "1", "2", "all"
    
    uint8_t light_mask = 0x00;

    // 2. 解析要控制哪个灯 (1号灯=0x01, 2号灯=0x02, 3号灯=0x04)
    if (target == "1") {
        light_mask = LIGHT_ID_1;
    } else if (target == "2") {
        light_mask = LIGHT_ID_2;
    } else if (target == "3") {
        light_mask = LIGHT_ID_3;
    } else if (target == "all" || target == "全部" || target == "所有") {
        light_mask = LIGHT_ID_ALL;
    } else {
        return "找不到指定的灯，请重试。";
    }

    // 3. 解析动作并计算最终的值
    uint8_t cmd_value = 0x00;
    std::string reply_str;

    if (action == "on" || action == "开" || action == "打开") {
        cmd_value = light_mask; // 传掩码，比如 0x01 或 0x07
        reply_str = "好的，照明已打开。";
    } else if (action == "off" || action == "关" || action == "关闭") {
        cmd_value = 0x00; // 关闭统一传 0x00
        reply_str = "好的，照明已关闭。";
    } else {
        return "无法识别的动作。";
    }

    // 4. 组装 KLV 控制包  
    uint8_t payload[LIGHT_CMD_LEN] = {KEY_LIGHT, LIGHT_V_LEN, cmd_value};
    
    // 5. 算 CRC 
    uint8_t crc_data[LIGHT_CRC_LEN] = {CMD_ESP_DOWN, LIGHT_CMD_LEN, KEY_LIGHT, LIGHT_V_LEN, cmd_value}; 
    uint8_t crc = Calculate_CRC8(crc_data, LIGHT_CRC_LEN);
    
    // 6. 组装完整数据帧
    uint8_t packet[] = {FRAME_HEAD, CMD_ESP_DOWN, LIGHT_CMD_LEN, KEY_LIGHT, LIGHT_V_LEN, cmd_value, crc, FRAME_TAIL};
    
    // 7. 通过底层串口发出去
    UartStm::SendData(packet, sizeof(packet));
    
    ESP_LOGI(TAG, "已发送控制指令: target=%s, action=%s, cmd=0x%02X, CRC: 0x%02X", target.c_str(), action.c_str(), cmd_value, crc);
    
    return reply_str; // 返回值传回给大模型，大模型会用语音播报
}

// 注册工具的函数
void RegisterAiLightControl() {
    auto& mcp_server = McpServer::GetInstance();
    
    mcp_server.AddTool("self.iot.control_light",
        "控制智能家居中的照明灯。当用户要求开灯、关灯或打开/关闭指定照明时，调用此工具。",
        PropertyList({
            Property("action", kPropertyTypeString, "要执行的动作，只能是 'on'（打开）或 'off'（关闭）"),
            Property("target", kPropertyTypeString, "要控制的灯编号，支持 '1' (照明1), '2' (照明2), 'all' (全部照明)")
        }),
        ControlLight
    );
    
    ESP_LOGI(TAG, "AI 控灯 MCP 工具注册成功！");
}
