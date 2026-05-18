#include "LcdUI.h"

/**
 * @brief 将整数转换为4位固定宽度字符串（前面补零）
 * @param str 输出缓冲区（至少6字节）
 * @param number 要转换的整数
 * @note 输出格式：始终显示4位数字，负数有负号，正数无符号
 *       例如：123 -> "0123", -45 -> "-0045", 0 -> "0000"
 */
static void int_to_fixed_width_4(char *str, int16_t number)
{
    uint8_t i = 0;
    
    if (str == NULL) {
        return;
    }
    
    // 处理负数
    if (number < 0) {
        str[0] = '-';
        number = -number;
        i = 1;
    } else {
        i = 0;
    }
    
    // 转换为字符串（无符号）
    func_uint_to_str(&str[i], (uint32_t)number);
    
    // 计算当前数字位数
    uint8_t len = 0;
    while (str[i + len] != '\0' && len < 5) {
        len++;
    }
    
    // 补零到4位
    if (len < 4) {
        // 移动字符串到右边，腾出补零的位置
        for (uint8_t j = len; j > 0; j--) {
            str[i + 4 - len + j - 1] = str[i + j - 1];
        }
        // 在前面补零
        for (uint8_t j = 0; j < 4 - len; j++) {
            str[i + j] = '0';
        }
        str[i + 4] = '\0';
    } else {
        // 已经是4位或更多，保持不变
        str[i + 4] = '\0';  // 如果超过4位，截断为4位
    }
}

/**
 * @brief 初始化LCD UI
 * @note 设置默认字体和颜色
 */
void lcd_ui_init(void)
{
    // 设置显示颜色：白色文字，黑色背景
    tft180_set_color(RGB565_WHITE, RGB565_BLACK);
    
    // 设置字体为8x16（更大的字体，更易读）
    tft180_display_font = TFT180_8X16_FONT;
}

/**
 * @brief 显示UART_Packet结构体内容
 * @param packet 指向UART_Packet结构体的指针
 * @param startX 显示起始X坐标
 * @param startY 显示起始Y坐标
 */
void display_uart_packet(const UART_Packet *packet, uint16_t startX, uint16_t startY)
{
    char buffer[20];  // 字符串缓冲区
    
    if (packet == NULL) {
        return;
    }
    
    // 显示mode字段
    tft180_show_string(startX, startY, "Mode:0x");
    if (packet->mode < 0x10) {
        buffer[0] = '0';  // 补0
        func_uint_to_str(&buffer[1], packet->mode);
        buffer[3] = '\0';  // 确保字符串结束
    } else {
        func_uint_to_str(buffer, packet->mode);
    }
    tft180_show_string(startX + 56, startY, buffer);
    
    // 显示x字段（4位固定宽度）
    tft180_show_string(startX, startY + 20, "X:");
    int_to_fixed_width_4(buffer, packet->x);
    tft180_show_string(startX + 20, startY + 20, buffer);
    
    // 显示y字段（4位固定宽度）
    tft180_show_string(startX, startY + 40, "Y:");
    int_to_fixed_width_4(buffer, packet->y);
    tft180_show_string(startX + 20, startY + 40, buffer);
    
    // 显示isValid字段
    tft180_show_string(startX, startY + 60, "Valid:");
    if (packet->isValid) {
        tft180_show_string(startX + 48, startY + 60, "true");
    } else {
        tft180_show_string(startX + 48, startY + 60, "false");
    }
}