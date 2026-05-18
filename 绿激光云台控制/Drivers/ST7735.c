#include "ST7735.h"
 
#define tft180_write_8bit_data(data)                ( ST7735_WriteByte(data) )
#define tft180_write_16bit_data(data)				( ST7735_Write2Byte(data) )
 
static	uint8_t							tft180_show_h		= LCD_H;									// 屏幕高
static	uint8_t							tft180_show_w		= LCD_W;									// 屏幕宽
static 	uint16_t                   		tft180_pencolor     = TFT180_DEFAULT_PENCOLOR;					// 画笔颜色(字体色)
static 	uint16_t                   		tft180_bgcolor      = TFT180_DEFAULT_BGCOLOR; 					// 背景颜色
tft180_font_size_enum    		tft180_display_font = TFT180_DEFAULT_DISPLAY_FONT;      		        // 显示字体类型
 
//-------------------------------------------------------------------------------------------------------------------
 
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     发送一个字节（根据平台自行实现）
// 参数说明     TxData           	发送字节
// 返回参数     void
// 使用示例     ST7735_WriteByte(0xff);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
// void ST7735_WriteByte(uint8_t TxData)
// {		
// 	DL_SPI_transmitData8(SPI_INST, TxData);
// 	while(DL_SPI_isBusy(SPI_INST));
// }

void ST7735_WriteByte(uint8_t TxData)
{		
    uint32_t timeout = 0x1FFFF; // 超时计数器
    
	DL_SPI_transmitData8(SPI_INST, TxData);
    
    // 【修改】增加超时判断，避免外设异常时死机
	while(DL_SPI_isBusy(SPI_INST) && timeout)
    {
        timeout--;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     发送两个字节（根据平台自行实现）
// 参数说明     TxData           	发送字节
// 返回参数     void
// 使用示例     ST7735_Write2Byte(RGB565_BLACK);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
// void ST7735_Write2Byte(uint16_t TxData)
// {		
// 	DL_SPI_transmitData8(SPI_INST, (uint8_t)((TxData & 0xFF00) >> 8));
// 	while (DL_SPI_isBusy(SPI_INST));
// 	DL_SPI_transmitData8(SPI_INST, (uint8_t)(TxData & 0x00FF));
// 	while (DL_SPI_isBusy(SPI_INST));
// }
 
void ST7735_Write2Byte(uint16_t TxData)
{		
    uint32_t timeout;
	DL_SPI_transmitData8(SPI_INST, (uint8_t)((TxData & 0xFF00) >> 8));
    
    timeout = 0x1FFFF;
	while (DL_SPI_isBusy(SPI_INST) && timeout) { timeout--; }
    
	DL_SPI_transmitData8(SPI_INST, (uint8_t)(TxData & 0x00FF));
    
    timeout = 0x1FFFF;
	while (DL_SPI_isBusy(SPI_INST) && timeout) { timeout--; }
}

//-------------------------------------------------------------------------------------------------------------------
 
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     写命令
// 参数说明     dat             数据
// 返回参数     void
// 使用示例     tft180_write_index(0x2a);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static void tft180_write_index (const uint8_t dat)
{
    TFT180_DC(0);
    tft180_write_8bit_data(dat);
    TFT180_DC(1);
}
 
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示区域 内部调用
// 参数说明     x1              起始x轴坐标
// 参数说明     y1              起始y轴坐标
// 参数说明     x2              结束x轴坐标
// 参数说明     y2              结束y轴坐标
// 返回参数     void
// 使用示例     tft180_set_region(0, 0, tft180_width_max - 1, tft180_height_max - 1);
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static void tft180_set_region (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	if(tft180_show_h == LCD_H) {
		tft180_write_index(0x2a);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(x1 + 2);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(x2 + 2);
 
		tft180_write_index(0x2b);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(y1 + 1);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(y2 + 1);
	} else {
		tft180_write_index(0x2a);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(x1 + 1);
		tft180_write_8bit_data(0x0);
		tft180_write_8bit_data(x2 + 1);
 
		tft180_write_index(0x2b);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(y1 + 2);
		tft180_write_8bit_data(0x00);
		tft180_write_8bit_data(y2 + 2);
	}
	tft180_write_index(0x2c);
}
 
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 清屏函数
// 参数说明     void
// 返回参数     void
// 使用示例     tft180_clear();
// 备注信息     将屏幕清空成背景颜色
//-------------------------------------------------------------------------------------------------------------------
void tft180_clear (void)
{
    uint16_t i = tft180_show_w * tft180_show_h;
    TFT180_CS(0);
    tft180_set_region(0, 0, tft180_show_w - 1, tft180_show_h - 1);
    for (; i != 0; i --)
    {
        tft180_write_16bit_data(tft180_bgcolor);
    }
    TFT180_CS(1);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 清屏函数
// 参数说明     color           颜色格式 RGB565 或者可以使用 zf_common_font.h 内常用颜色宏定义
// 返回参数     void
// 使用示例     tft180_full(RGB565_YELLOW);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_full (const uint16_t color)
{
    uint16_t i = tft180_show_w * tft180_show_h;
    TFT180_CS(0);
    tft180_set_region(0, 0, tft180_show_w - 1, tft180_show_h - 1);
    for (; i != 0; i --)
    {
        tft180_write_16bit_data(color);
    }
    TFT180_CS(1);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置显示颜色
// 参数说明     pen             颜色格式 RGB565 或者可以使用 zf_common_font.h 内常用颜色宏定义
// 参数说明     bgcolor         颜色格式 RGB565 或者可以使用 zf_common_font.h 内常用颜色宏定义
// 返回参数     void
// 使用示例     tft180_set_color(RGB565_WHITE, RGB565_BLACK);
// 备注信息     字体颜色和背景颜色也可以随时自由设置 设置后生效
//-------------------------------------------------------------------------------------------------------------------
void tft180_set_color (uint16_t pen, const uint16_t bgcolor)
{
    tft180_pencolor = pen;
    tft180_bgcolor = bgcolor;
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 画点
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的颜色
// 返回参数     void
// 使用示例     tft180_draw_point(0, 0, RGB565_RED);            // 坐标 0,0 画一个红色的点
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_draw_point (uint16_t x, uint16_t y, const uint16_t color)
{
    TFT180_CS(0);
    tft180_set_region(x, y, x, y);
    tft180_write_16bit_data(color);
    TFT180_CS(1);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 画线
// 参数说明     x_start         坐标x方向的起点
// 参数说明     y_start         坐标y方向的起点
// 参数说明     x_end           坐标x方向的终点
// 参数说明     y_end           坐标y方向的终点
// 参数说明     dat             需要显示的颜色
// 返回参数     void
// 使用示例     tft180_draw_line(0, 0, 10, 10,RGB565_RED);      // 坐标 0,0 到 10,10 画一条红色的线
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_draw_line (uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, const uint16_t color)
{
    uint16_t x_dir = (x_start < x_end ? 1 : -1);
    uint16_t y_dir = (y_start < y_end ? 1 : -1);
    float temp_rate = 0;
    float temp_b = 0;
    do
    {
        if(x_start != x_end)
        {
            temp_rate = (float)(y_start - y_end) / (float)(x_start - x_end);
            temp_b = (float)y_start - (float)x_start * temp_rate;
        }
        else
        {
            while(y_start != y_end)
            {
                tft180_draw_point(x_start, y_start, color);
                y_start += y_dir;
            }
            tft180_draw_point(x_start, y_start, color);
            break;
        }
        
        if(abs(y_start - y_end) > abs(x_start - x_end))
        {
            while(y_start != y_end)
            {
                tft180_draw_point(x_start, y_start, color);
                y_start += y_dir;
                x_start = (int16_t)(((float)y_start - temp_b) / temp_rate);
            }
            tft180_draw_point(x_start, y_start, color);
        }
        else
        {
            while(x_start != x_end)
            {
                tft180_draw_point(x_start, y_start, color);
                x_start += x_dir;
                y_start = (int16_t)((float)x_start * temp_rate + temp_b);
            }
            tft180_draw_point(x_start, y_start, color);
        }
    }while(0);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 显示字符
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的字符
// 返回参数     void
// 使用示例     tft180_show_char(0, 0, 'x');                    // 坐标 0,0 写一个字符 x
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_show_char (uint16_t x, uint16_t y, const char dat)
{
    uint8_t i = 0, j = 0;
    TFT180_CS(0);
	switch(tft180_display_font)
	{
		case TFT180_6X8_FONT:
		tft180_set_region(x, y, x + 5, y + 7);
		for(i = 0; 8 > i; i ++)
		{
			for(j = 0; 6 > j; j ++)
			{		
				// 减 32 因为是取模是从空格开始取得 空格在 ascii 中序号是 32
				uint8_t temp_top = ascii_font_6x8[dat - 32][j];
				if(temp_top & (0x01 << i))
				{
						tft180_write_16bit_data(tft180_pencolor);
				}
				else
				{
						tft180_write_16bit_data(tft180_bgcolor);
				}
				temp_top >>= 1;
			}
		}
		break;
		case TFT180_8X16_FONT:
		tft180_set_region(x, y, x + 7, y + 15);
		
		for(i = 0; 8 > i; i ++)
		{
			for(j = 0; 8 > j; j ++)
			{
				uint8_t temp_top = ascii_font_8x16[dat - 32][j];
				if(temp_top & (0x01 << i))
				{
						tft180_write_16bit_data(tft180_pencolor);
				}
				else
				{
						tft180_write_16bit_data(tft180_bgcolor);
				}
				temp_top >>= 1;
			}
		}
		for(i = 0; 8 > i; i ++)
		{
			for(j = 8; 16 > j; j ++)
			{
				uint8_t temp_bottom = ascii_font_8x16[dat - 32][j];
				if(temp_bottom & (0x01 << i))
				{
						tft180_write_16bit_data(tft180_pencolor);
				}
				else
				{
						tft180_write_16bit_data(tft180_bgcolor);
				}
				temp_bottom >>= 1;
			}
		}
		break;
		default:
		break;
	}
    TFT180_CS(1);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 显示字符串
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的字符串
// 返回参数     void
// 使用示例     tft180_show_string(0, 0, "seekfree");
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_show_string (uint16_t x, uint16_t y, const char dat[])
{   
    uint16_t j = 0;
    while('\0' != dat[j])
    {
        switch(tft180_display_font)
        {
            case TFT180_6X8_FONT:   tft180_show_char(x + 6 * j, y, dat[j]); break;
            case TFT180_8X16_FONT:  tft180_show_char(x + 8 * j, y, dat[j]); break;
            case TFT180_16X16_FONT: break;                                      // 暂不支持
        }
        j ++;
    }
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 显示32位有符号 (去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的变量 数据类型 int32
// 参数说明     num             需要显示的位数 最高10位  不包含正负号
// 返回参数     void
// 使用示例     tft180_show_int(0, 0, x, 3);                    // x 可以为 int32 int16 int8 类型
// 备注信息     负数会显示一个 ‘-’号
//-------------------------------------------------------------------------------------------------------------------
void tft180_show_int (uint16_t x, uint16_t y, const int32_t dat, uint8_t num)
{
    int32_t dat_temp = dat;
    int32_t offset = 1;
    char data_buffer[12];
 
    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num + 1);
 
    // 用来计算余数显示 123 显示 2 位则应该显示 23
    if(10 > num)
    {
        for(; 0 < num; num --)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_int_to_str(data_buffer, dat_temp);
    tft180_show_string(x, y, (const char *)&data_buffer);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 显示32位无符号 (去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的变量 数据类型 uint32
// 参数说明     num             需要显示的位数 最高10位  不包含正负号
// 返回参数     void
// 使用示例     tft180_show_uint(0, 0, x, 3);                   // x 可以为 uint32 uint16 uint8 类型
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_show_uint (uint16_t x, uint16_t y, const uint32_t dat, uint8_t num)
{
    uint32_t dat_temp = dat;
    int32_t offset = 1;
    char data_buffer[12];
    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num);
 
    // 用来计算余数显示 123 显示 2 位则应该显示 23
    if(10 > num)
    {
        for(; 0 < num; num --)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_uint_to_str(data_buffer, dat_temp);
    tft180_show_string(x, y, (const char *)&data_buffer);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 显示浮点数(去除整数部分无效的0)
// 参数说明     x               坐标x方向的起点 参数范围 [0, tft180_width_max-1]
// 参数说明     y               坐标y方向的起点 参数范围 [0, tft180_height_max-1]
// 参数说明     dat             需要显示的变量 数据类型 double
// 参数说明     num             整数位显示长度   最高8位  
// 参数说明     pointnum        小数位显示长度   最高6位
// 返回参数     void
// 使用示例     tft180_show_float(0, 0, x, 2, 3);               // 显示浮点数   整数显示2位   小数显示三位
// 备注信息     特别注意当发现小数部分显示的值与你写入的值不一样的时候，
//              可能是由于浮点数精度丢失问题导致的，这并不是显示函数的问题，
//              有关问题的详情，请自行百度学习   浮点数精度丢失问题。
//              负数会显示一个 ‘-’号
//-------------------------------------------------------------------------------------------------------------------
void tft180_show_float (uint16_t x, uint16_t y, const double dat, uint8_t num, uint8_t pointnum)
{
    double dat_temp = dat;
    double offset = 1.0;
    char data_buffer[17];
    memset(data_buffer, 0, 17);
    memset(data_buffer, ' ', num + pointnum + 2);
 
    // 用来计算余数显示 123 显示 2 位则应该显示 23
    for(; 0 < num; num --)
    {
        offset *= 10;
    }
    dat_temp = dat_temp - ((int)dat_temp / (int)offset) * offset;
    func_double_to_str(data_buffer, dat_temp, pointnum);
    tft180_show_string(x, y, data_buffer);
}

/**
 * @brief  TFT180 格式化输出函数
 * @param  x      起始x轴坐标
 * @param  y      起始y轴坐标
 * @param  fmt    格式化字符串 (类似 printf)
 * @param  ...    可变参数
 */
void tft180_printf(uint16_t x, uint16_t y, const char *fmt, ...)
{
    char buffer[64]; // 根据实际需求调整缓冲区大小，128x160屏幕建议64或128
    va_list args;
    
    va_start(args, fmt);
    // 使用 vsnprintf 进行格式化，防止缓冲区溢出
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    // 调用现有的字符串显示函数
    tft180_show_string(x, y, buffer);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     TFT180 初始化
// 返回参数     void
// 返回参数     void
// 使用示例     tft180_init();
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void tft180_init (void)
{
    tft180_set_color(tft180_pencolor, tft180_bgcolor);
 
    TFT180_RES(0);
    Delay_ms(10);
 
    TFT180_RES(1);
    Delay_ms(120);
    TFT180_CS(0);
 
    tft180_write_index(0x11);
    Delay_ms(120);
 
    tft180_write_index(0xB1);
    tft180_write_8bit_data(0x01);
    tft180_write_8bit_data(0x2C);
    tft180_write_8bit_data(0x2D);
 
    tft180_write_index(0xB2);
    tft180_write_8bit_data(0x01);
    tft180_write_8bit_data(0x2C);
    tft180_write_8bit_data(0x2D);
 
    tft180_write_index(0xB3);
    tft180_write_8bit_data(0x01);
    tft180_write_8bit_data(0x2C);
    tft180_write_8bit_data(0x2D);
    tft180_write_8bit_data(0x01);
    tft180_write_8bit_data(0x2C);
    tft180_write_8bit_data(0x2D);
 
    tft180_write_index(0xB4);
    tft180_write_8bit_data(0x07);
 
    tft180_write_index(0xC0);
    tft180_write_8bit_data(0xA2);
    tft180_write_8bit_data(0x02);
    tft180_write_8bit_data(0x84);
    tft180_write_index(0xC1);
    tft180_write_8bit_data(0xC5);
 
    tft180_write_index(0xC2);
    tft180_write_8bit_data(0x0A);
    tft180_write_8bit_data(0x00);
 
    tft180_write_index(0xC3);
    tft180_write_8bit_data(0x8A);
    tft180_write_8bit_data(0x2A);
    tft180_write_index(0xC4);
    tft180_write_8bit_data(0x8A);
    tft180_write_8bit_data(0xEE);
 
    tft180_write_index(0xC5);
    tft180_write_8bit_data(0x0E);
 
    tft180_write_index(0x36);
    switch(TFT180_DEFAULT_DISPLAY_DIR)                                                  // y x v
    {
        case TFT180_PORTAIT:        
			tft180_write_8bit_data(1<<7 | 1<<6 | 0<<5);  
			break;
        case TFT180_PORTAIT_180:    
			tft180_write_8bit_data(0<<7 | 0<<6 | 0<<5);  
			break;
        case TFT180_CROSSWISE:      
			tft180_write_8bit_data(1<<7 | 0<<6 | 1<<5);
			tft180_show_h = LCD_W;
			tft180_show_w = LCD_H;
			break;
        case TFT180_CROSSWISE_180:  
			tft180_write_8bit_data(0<<7 | 1<<6 | 1<<5);
			tft180_show_h = LCD_W;
			tft180_show_w = LCD_H;
			break;
    }
 
    tft180_write_index(0xe0);
    tft180_write_8bit_data(0x0f);
    tft180_write_8bit_data(0x1a);
    tft180_write_8bit_data(0x0f);
    tft180_write_8bit_data(0x18);
    tft180_write_8bit_data(0x2f);
    tft180_write_8bit_data(0x28);
    tft180_write_8bit_data(0x20);
    tft180_write_8bit_data(0x22);
    tft180_write_8bit_data(0x1f);
    tft180_write_8bit_data(0x1b);
    tft180_write_8bit_data(0x23);
    tft180_write_8bit_data(0x37);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x07);
    tft180_write_8bit_data(0x02);
    tft180_write_8bit_data(0x10);
 
    tft180_write_index(0xe1);
    tft180_write_8bit_data(0x0f);
    tft180_write_8bit_data(0x1b);
    tft180_write_8bit_data(0x0f);
    tft180_write_8bit_data(0x17);
    tft180_write_8bit_data(0x33);
    tft180_write_8bit_data(0x2c);
    tft180_write_8bit_data(0x29);
    tft180_write_8bit_data(0x2e);
    tft180_write_8bit_data(0x30);
    tft180_write_8bit_data(0x30);
    tft180_write_8bit_data(0x39);
    tft180_write_8bit_data(0x3f);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x07);
    tft180_write_8bit_data(0x03);
    tft180_write_8bit_data(0x10);
 
    tft180_write_index(0x2a);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x00 + 2);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x80 + 2);
 
    tft180_write_index(0x2b);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x00 + 3);
    tft180_write_8bit_data(0x00);
    tft180_write_8bit_data(0x80 + 3);
 
    tft180_write_index(0xF0);
    tft180_write_8bit_data(0x01);
    tft180_write_index(0xF6);
    tft180_write_8bit_data(0x00);
 
    tft180_write_index(0x3A);
    tft180_write_8bit_data(0x05);
    tft180_write_index(0x29);
    TFT180_CS(1);
 
    tft180_clear();
}