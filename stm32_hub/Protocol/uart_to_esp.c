#include "uart_to_esp.h"
#include "Serial.h" 
#include "LED.h" 

// CRC-8 算法
static uint8_t Calc_CRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

// UART发送函数：和esp通信
void UART_ESP_Send(uint8_t cmd, uint8_t *data_content, uint8_t content_len) {
    uint8_t packet[UART_RX_BUF_SIZE];
    uint8_t index = 0;
    uint8_t crc_data[UART_RX_BUF_SIZE]; // 缩减到32字节，防止栈溢出
    
    crc_data[0] = cmd;
    crc_data[1] = content_len;
    for (uint8_t i = 0; i < content_len; i++) {
        crc_data[UART_HEADER_LEN + i] = data_content[i];
    }
    uint8_t crc = Calc_CRC8(crc_data, content_len + UART_HEADER_LEN);
    
    packet[index++] = FRAME_HEAD;
    packet[index++] = cmd;
    packet[index++] = content_len;
    for (uint8_t i = 0; i < content_len; i++) packet[index++] = data_content[i];
    packet[index++] = crc;
    packet[index++] = FRAME_TAIL;
    
    Serial_SendArray(packet, index); 
}

// 接收解析状态机
static ParseState_t state = STATE_WAIT_HEAD;
static UartRxPacket_t rx_packet; // 
static uint8_t rx_index = 0;
static uint8_t rx_crc = 0;

void UART_ESP_RxCallback(uint8_t data) {
    switch (state) {
        case STATE_WAIT_HEAD:
            if (data == FRAME_HEAD) state = STATE_WAIT_CMD;
            break;
            
        case STATE_WAIT_CMD:
            rx_packet.cmd = data; 
            state = STATE_WAIT_LEN;
            break;
            
        case STATE_WAIT_LEN:
            rx_packet.len = data;
            if (rx_packet.len > UART_MAX_PAYLOAD || rx_packet.len == 0) {
                state = STATE_WAIT_HEAD;
            } else { 
                rx_index = 0; 
                state = STATE_READ_DATA; 
            }
            break;
            
        case STATE_READ_DATA:
            rx_packet.payload[rx_index++] = data;
            if (rx_index >= rx_packet.len) state = STATE_WAIT_CRC;
            break;
            
        case STATE_WAIT_CRC:
            rx_crc = data;
            {
                uint8_t crc_data[UART_RX_BUF_SIZE];
                crc_data[0] = rx_packet.cmd;
                crc_data[1] = rx_packet.len;
                for(uint8_t i=0; i<rx_packet.len; i++) {
                    crc_data[UART_HEADER_LEN + i] = rx_packet.payload[i];
                }
                
                if (Calc_CRC8(crc_data, rx_packet.len + UART_HEADER_LEN) == rx_crc) {
                    state = STATE_WAIT_TAIL;
                } else {
                    state = STATE_WAIT_HEAD; 
                }
            }
            break;
            
        case STATE_WAIT_TAIL:
            if (data == FRAME_TAIL) {
                if (rx_packet.cmd == CMD_ESP_DOWN) { 
                    uint8_t ptr = 0;
                    while (ptr + UART_HEADER_LEN <= rx_packet.len) {
                        uint8_t k = rx_packet.payload[ptr];
                        uint8_t v_len = rx_packet.payload[ptr + UART_V_IDX_1];
                        
                        if (ptr + UART_HEADER_LEN + v_len > rx_packet.len) break;
                        
                        uint8_t *v = &rx_packet.payload[ptr + UART_V_IDX_2];
                        
                        if (k == KEY_LIGHT) { 
                            if (v[UART_V_IDX_0] & 0x01) LED3_ON(); else LED3_OFF(); 
                            if (v[UART_V_IDX_0] & 0x02) LED4_ON(); else LED4_OFF(); 
                            if (v[UART_V_IDX_0] & 0x04) LED5_ON(); else LED5_OFF(); 
                        }
                        ptr += (UART_HEADER_LEN + v_len);
                    }
                }
            }
            state = STATE_WAIT_HEAD; 
            break;
            
        default:
            state = STATE_WAIT_HEAD;
            break;
    }
}
