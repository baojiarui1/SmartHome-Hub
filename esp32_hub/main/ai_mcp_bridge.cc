#include "ai_mcp_bridge.h"
#include "mcp_server.h"
#include "uart_stm.h"
#include <esp_log.h>

static const char *TAG = "AI_MCP_BRIDGE";

// 这个函数是大模型调用工具时，实际执行的代码
ReturnValue ControlLight(const PropertyList& properties) {
    // 从大模型传过来的参数里获取 state 值
    auto state = properties["state"].value<std::string>();
    
    uint8_t cmd_value = 0x00;
    std::string reply_str;

    if (state == "on" || state == "开") {
        cmd_value = 0x01;
        reply_str = "好的，照明已打开。";
    } else if (state == "off" || state == "关") {
        cmd_value = 0x00;
        reply_str = "好的，照明已关闭。";
    } else {
        return "无法识别的指令，请输入 on 或 off。";
    }

    // 组装 KLV 控制包
    uint8_t payload[] = {0x11, 0x01, cmd_value};
    
    // 调用在 uart_stm.cc 里写好的 CRC 函数
    uint8_t crc = Calculate_CRC8(payload, 3);
    
    // 组装完整数据帧：42 03 11 01 [cmd] [crc] 52
    uint8_t packet[] = {0x42, 0x03, 0x11, 0x01, cmd_value, crc, 0x52};
    
    // 通过底层串口发出去
    UartStm::SendData(packet, sizeof(packet));
    
    ESP_LOGI(TAG, "已发送控制指令，CRC: 0x%02X", crc);
    
    return reply_str; // 返回值传回给大模型，大模型会用语音播报
}

// 注册工具的函数
void RegisterAiLightControl() {
    auto& mcp_server = McpServer::GetInstance();
    
    mcp_server.AddTool("self.iot.control_light",
        "控制智能家居中的照明灯。当用户要求开灯、关灯或打开/关闭照明时，调用此工具。",
        PropertyList({
            Property("state", kPropertyTypeString, "照明灯的目标状态。只能是 'on'（开）或 'off'（关）")
        }),
        ControlLight
    );
    
    ESP_LOGI(TAG, "AI 控灯 MCP 工具注册成功！");
}