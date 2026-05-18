#ifndef __COMMON_H
#define __COMMON_H

#include "ti_msp_dl_config.h"
#include "ST7735.h"
#include "uart.h"

/**
 * @brief 显示UART_Packet结构体内容
 * @param packet 指向UART_Packet结构体的指针
 * @param startX 显示起始X坐标
 * @param startY 显示起始Y坐标
 */
void display_uart_packet(const UART_Packet *packet, uint16_t startX, uint16_t startY);

/**
 * @brief 初始化LCD UI
 * @note 设置默认字体和颜色
 */
void lcd_ui_init(void);

#endif