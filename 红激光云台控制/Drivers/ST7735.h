#ifndef __ST7735_H_
#define __ST7735_H_
 
#include "headfile.h"

#include <stdio.h>   // 新增：用于 vsnprintf
#include <stdarg.h>  // 新增：用于变长参数处理
//---------------------------------------------------------------------------------------------------------------
/* 硬件连接配置 - 已适配跨端口引脚 (RES在GPIOA, DC/CS在GPIOB) */
#define SPI_INST                    (ST7735_SPI_INST)        // SPI通道

/* 为每个控制引脚定义独立的端口和引脚 */
#define GPIO_RES_PORT               (ST7735_PORT)        // RESET引脚端口: GPIOA
#define GPIO_RES_PIN                (ST7735_RES_PIN)         // RESET引脚编号: PIN_7

#define GPIO_DC_PORT                (ST7735_PORT)         // DC引脚端口: GPIOB
#define GPIO_DC_PIN                 (ST7735_DC_PIN)          // DC引脚编号: PIN_2

#define GPIO_CS_PORT                (ST7735_PORT)         // CS引脚端口: GPIOB
#define GPIO_CS_PIN                 (ST7735_CS_PIN)          // CS引脚编号: PIN_3

#define GPIO_BL_PORT                (ST7735_PORT)         // 背光引脚端口: GPIOA
#define GPIO_BL_PIN                 (ST7735_BL_PIN)          // 背光引脚编号: PIN_8
//---------------------------------------------------------------------------------------------------------------
 
#define TFT180_DEFAULT_DISPLAY_DIR      (TFT180_PORTAIT)                      // 默认的显示方向
#define TFT180_DEFAULT_PENCOLOR         (RGB565_RED)                            // 默认的画笔颜色
#define TFT180_DEFAULT_BGCOLOR          (RGB565_WHITE)                          // 默认的背景颜色
#define TFT180_DEFAULT_DISPLAY_FONT     (TFT180_6X8_FONT)                      	// 默认的字体模式
#define LCD_W 128																// 屏幕尺寸W
#define LCD_H 160																// 屏幕尺寸H
 
/* 修改控制宏，使其使用各自独立的端口 */
#define TFT180_RES(x)               ( (x) ? DL_GPIO_setPins(GPIO_RES_PORT, GPIO_RES_PIN) : DL_GPIO_clearPins(GPIO_RES_PORT, GPIO_RES_PIN) )
#define TFT180_DC(x)                ( (x) ? DL_GPIO_setPins(GPIO_DC_PORT, GPIO_DC_PIN) : DL_GPIO_clearPins(GPIO_DC_PORT, GPIO_DC_PIN) )
#define TFT180_CS(x)                ( (x) ? DL_GPIO_setPins(GPIO_CS_PORT, GPIO_CS_PIN) : DL_GPIO_clearPins(GPIO_CS_PORT, GPIO_CS_PIN) )
#define TFT180_BL(x)                ( (x) ? DL_GPIO_setPins(GPIO_BL_PORT, GPIO_BL_PIN) : DL_GPIO_clearPins(GPIO_BL_PORT, GPIO_BL_PIN) )
 
//---------------------------------------------------------------------------------------------------------------
typedef enum
{
    TFT180_PORTAIT                      = 0,                                    // 竖屏模式
    TFT180_PORTAIT_180                  = 1,                                    // 竖屏模式  旋转180
    TFT180_CROSSWISE                    = 2,                                    // 横屏模式
    TFT180_CROSSWISE_180                = 3,                                    // 横屏模式  旋转180
}tft180_dir_enum;
 
typedef enum
{
    TFT180_6X8_FONT                     = 0,                                    // 6x8      字体
    TFT180_8X16_FONT                    = 1,                                    // 8x16     字体
    TFT180_16X16_FONT                   = 2,                                    // 16x16    字体 目前不支持
}tft180_font_size_enum;

extern tft180_font_size_enum tft180_display_font;
//---------------------------------------------------------------------------------------------------------------
 
void tft180_init (void);
 
void tft180_clear (void);
void tft180_full (const uint16_t color);
void tft180_set_color (uint16_t pen, const uint16_t bgcolor);
void tft180_draw_point (uint16_t x, uint16_t y, const uint16_t color);
void tft180_draw_line (uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, const uint16_t color);
void tft180_show_char (uint16_t x, uint16_t y, const char dat);
void tft180_show_string (uint16_t x, uint16_t y, const char dat[]);
void tft180_show_int (uint16_t x, uint16_t y, const int32_t dat, uint8_t num);
void tft180_show_uint (uint16_t x, uint16_t y, const uint32_t dat, uint8_t num);
void tft180_show_float (uint16_t x, uint16_t y, const double dat, uint8_t num, uint8_t pointnum);

// 新增 printf 函数声明
void tft180_printf(uint16_t x, uint16_t y, const char *fmt, ...);

#endif