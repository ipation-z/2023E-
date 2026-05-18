#ifndef UART_H
#define UART_H

#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define FRAME_HEAD  0xAA
#define FRAME_END   0xBB
#define FRAME_LEN   7

/**
 * @brief UART 数据包结构体
 * 用于存储从 7 字节 Hex 流中解析出的有效信息
 */
typedef struct {
    uint8_t mode;    /**< 模式选择: 0x01-坐标模式, 0x02-误差模式 */
    int16_t x;       /**< X 轴数值 (支持有符号 16 位整型) */
    int16_t y;       /**< Y 轴数值 (支持有符号 16 位整型) */
    bool isValid;    /**< 合法性标志: true 表示校验通过，false 表示数据错误 */
} UART_Packet;

// --- UART1 相关（原有） ---
extern volatile bool g_uart1_rx_complete;
extern UART_Packet g_uart1_packet;

// --- UART0 相关（新增） ---
extern volatile bool g_uart0_rx_complete;
extern UART_Packet g_uart0_packet;

/**
 * @brief 解析原始字节数组为结构体
 * @param buffer 指向包含 7 字节原始数据的数组首地址
 * @return 返回解析后的 UART_Packet 结构体
 * @note 内部处理遵循 <BBhhB 小端格式 (Little-Endian)
 */
UART_Packet UART_ParsePacket(uint8_t *buffer);

/**
 * @brief 按照协议格式发送数据包
 * @param uart 指向 UART 硬件实例 (如 UART_1_INST)
 * @param mode 模式字节
 * @param x X轴数值
 * @param y Y轴数值
 * @example UART_SendPacket(UART_1_INST, 0x01, 160, 120);
 */
void UART_SendPacket(UART_Regs *uart, uint8_t mode, int16_t x, int16_t y);

/**
 * @brief 格式化打印字符串到指定串口 (类似 printf)
 * @param uart 指向 UART 硬件实例 (如 UART_0_INST)
 * @param format 格式化字符串 (如 "Val: %d\r\n")
 * @param ... 可变参数
 */
void UART_Printf(UART_Regs *uart, const char *format, ...);

//UART端口中断初始化
void UART_Interrupt_Init(void);

//UART中断服务函数ISR
void UART_0_INST_IRQHandler(void);
void UART_1_INST_IRQHandler(void);
#endif