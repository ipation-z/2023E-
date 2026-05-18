#include "uart.h"

//此处为中断状态机相关变量
// UART1 变量
volatile bool g_uart1_rx_complete = false;
UART_Packet g_uart1_packet;
static uint8_t g_uart1_temp_buf[FRAME_LEN];
static uint8_t g_uart1_index = 0;

// UART0 变量
volatile bool g_uart0_rx_complete = false;
UART_Packet g_uart0_packet;
static uint8_t g_uart0_temp_buf[FRAME_LEN];
static uint8_t g_uart0_index = 0;



/**
 * @brief 解析 7 字节原始数据并返回结构体
 * 格式: [AA] [Mode] [X_Low] [X_High] [Y_Low] [Y_High] [BB]
 */
UART_Packet UART_ParsePacket(uint8_t *buffer) {
    UART_Packet packet;
    
    // 基础校验（虽然主循环判断过，这里建议二次确认）
    if (buffer[0] == FRAME_HEAD && buffer[6] == FRAME_END) {
        packet.mode = buffer[1];
        // 小端拼接: (高位 << 8) | 低位
        packet.x = (int16_t)((buffer[3] << 8) | buffer[2]);
        packet.y = (int16_t)((buffer[5] << 8) | buffer[4]);
        packet.isValid = true;
    } else {
        packet.isValid = false;
    }
    
    return packet;
}

/**
 * @brief 发送相同格式的 7 字节 Hex 包
 */
void UART_SendPacket(UART_Regs *uart, uint8_t mode, int16_t x, int16_t y) {
    uint8_t sendBuf[FRAME_LEN];
    
    sendBuf[0] = FRAME_HEAD;
    sendBuf[1] = mode;
    // 小端拆分: 低位在前
    sendBuf[2] = (uint8_t)(x & 0xFF);         // X Low
    sendBuf[3] = (uint8_t)((x >> 8) & 0xFF);  // X High
    sendBuf[4] = (uint8_t)(y & 0xFF);         // Y Low
    sendBuf[5] = (uint8_t)((y >> 8) & 0xFF);  // Y High
    sendBuf[6] = FRAME_END;
    
    for (int i = 0; i < FRAME_LEN; i++) {
        DL_UART_Main_transmitDataBlocking(uart, sendBuf[i]);
    }
}

/**
 * @brief 自定义 Printf，用于向指定串口打印调试字符串
 */
void UART_Printf(UART_Regs *uart, const char *format, ...) {
    char buffer[128]; // 根据需求调整缓冲区大小
    va_list args;
    
    va_start(args, format);
    uint32_t length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    for (uint32_t i = 0; i < length; i++) {
        DL_UART_Main_transmitDataBlocking(uart, buffer[i]);
    }
}

/**
 * @brief 通用字节处理状态机
 */
void UART_Process_Byte(UART_Regs *uart, uint8_t data) {
    uint8_t *idx;
    uint8_t *buf;
    UART_Packet *pkt;
    volatile bool *complete_flag;

    // 根据传入的串口实例，选择对应的缓冲区和标志位
    if (uart == UART_1_INST) {
        idx = &g_uart1_index;
        buf = g_uart1_temp_buf;
        pkt = &g_uart1_packet;
        complete_flag = &g_uart1_rx_complete;
    } else {
        idx = &g_uart0_index;
        buf = g_uart0_temp_buf;
        pkt = &g_uart0_packet;
        complete_flag = &g_uart0_rx_complete;
    }

    // 状态机逻辑
    if (*idx == 0 && data == FRAME_HEAD) {
        buf[(*idx)++] = data;
    } else if (*idx > 0 && *idx < (FRAME_LEN - 1)) {
        buf[(*idx)++] = data;
    } else if (*idx == (FRAME_LEN - 1)) {
        if (data == FRAME_END) {
            buf[*idx] = data;
            *pkt = UART_ParsePacket(buf);
            if (pkt->isValid) *complete_flag = true;
        }
        *idx = 0; // 重置
    }
}

// 中断初始化函数
void UART_Interrupt_Init(void) {
    DL_UART_Main_enableInterrupt(UART_0_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    DL_UART_Main_enableInterrupt(UART_1_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
}

/**
 * 统一的中断入口处理
 */
void UART_0_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(UART_0_INST) == DL_UART_IIDX_RX) {
        UART_Process_Byte(UART_0_INST, DL_UART_Main_receiveData(UART_0_INST));
    }
}

void UART_1_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(UART_1_INST) == DL_UART_IIDX_RX) {
        UART_Process_Byte(UART_1_INST, DL_UART_Main_receiveData(UART_1_INST));
    }
}