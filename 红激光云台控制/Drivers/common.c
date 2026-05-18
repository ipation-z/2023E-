#include "headfile.h"
 
//此处不加volatile 编译优化会将这个计算优化 导致无法使用Delay_ms
volatile uint32_t delay_times = 0;
 
//搭配滴答定时器实现的精确ms延时
// void Delay_ms(uint32_t ms)
// {
//     delay_times = ms;
//     while( delay_times != 0 );
// }

void Delay_ms(uint32_t ms)
{
    delay_times = ms;
    uint32_t safety_timeout = ms * 50000; // 根据主频估算一个极大值
    
    // 即使中断不工作，运行完循环也会强制退出
    while( delay_times != 0 && safety_timeout > 0 )
    {
        safety_timeout--;
    }
}
 
//滴答定时器中断服务函数
void SysTick_Handler(void)
{
    if( delay_times != 0 )
    {
        delay_times--;
    }
    //Key_Tick();
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     整形数字转字符串 数据范围是 [-32768,32767]
// 参数说明     *str            字符串指针
// 参数说明     number          传入的数据
// 返回参数     void
// 使用示例     func_int_to_str(data_buffer, -300);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void func_int_to_str (char *str, int32_t number)
{
    uint8_t data_temp[16];                                                        // 缓冲区
    uint8_t bit = 0;                                                              // 数字位数
    int32_t number_temp = 0;
    do
    {
        if(NULL == str)
        {
            break;
        }
 
        if(0 > number)                                                          // 负数
        {
            *str ++ = '-';
            number = -number;
        }
        else if(0 == number)                                                    // 或者这是个 0
        {
            *str = '0';
            break;
        }
 
        while(0 != number)                                                      // 循环直到数值归零
        {
            number_temp = number % 10;
            data_temp[bit ++] = abs(number_temp);                          // 倒序将数值提取出来
            number /= 10;                                                       // 削减被提取的个位数
        }
        while(0 != bit)                                                         // 提取的数字个数递减处理
        {
            *str ++ = (data_temp[bit - 1] + 0x30);                              // 将数字从倒序数组中倒序取出 变成正序放入字符串
            bit --;
        }
    }while(0);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     整形数字转字符串 数据范围是 [0,65535]
// 参数说明     *str            字符串指针
// 参数说明     number          传入的数据
// 返回参数     void
// 使用示例     func_uint_to_str(data_buffer, 300);
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
// 在 common.c 中修改 func_uint_to_str 函数
void func_uint_to_str (char *str, uint32_t number)
{
    int8_t data_temp[16];
    uint8_t bit = 0;
    
    // 首先确保字符串以null结尾，清空可能的内容
    if(str != NULL) {
        str[0] = '\0';
    }
    
    do
    {
        if(NULL == str)
        {
            break;
        }
        
        if(0 == number)                                                         // 这是个 0
        {
            str[0] = '0';
            str[1] = '\0';  // 明确添加字符串终止符
            break;
        }
        
        while(0 != number)                                                      // 循环直到数值归零
        {
            data_temp[bit ++] = (number % 10);                                  // 倒序将数值提取出来
            number /= 10;                                                       // 削减被提取的个位数
        }
        while(0 != bit)                                                         // 提取的数字个数递减处理
        {
            *str++ = (data_temp[bit - 1] + 0x30);                              // 将数字从倒序数组中倒序取出
            bit--;
        }
        *str = '\0';  // 关键：添加字符串终止符
    } while(0);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     浮点数字转字符串
// 参数说明     *str            字符串指针
// 参数说明     number          传入的数据
// 参数说明     point_bit       小数点精度
// 返回参数     void
// 使用示例     func_double_to_str(data_buffer, 3.1415, 2);                     // 结果输出 data_buffer = "3.14"
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void func_double_to_str (char *str, double number, uint8_t point_bit)
{
    int data_int = 0;                                                           // 整数部分
    int data_float = 0;                                                         // 小数部分
    int data_temp[12];                                                          // 整数字符缓冲
    uint8_t bit = 0;                                                            // 通用位数计数器
    uint8_t i = 0;                                                              // 循环计数器

    do
    {
        if(NULL == str)
        {
            break;
        }

        // 处理数字为0的特殊情况
        if(0.0 == number)                                                      
        {
            *str++ = '0';
            // 只要要求显示小数位，就添加小数点
            if (point_bit > 0) {
                *str++ = '.';
                // 补足指定数量的小数位0
                for (i = 0; i < point_bit; i++) {
                    *str++ = '0';
                }
            }
            break;
        }
        
        // 处理负数
        if(number < 0.0)                                                        
        {
            *str++ = '-';
            number = -number; // 将number转为正数以便后续处理
        }

        // 提取整数部分
        data_int = (int)number;                                                 

        // 提取并放大指定位数的小数部分
        number = number - (double)data_int;                                     // 得到纯小数部分
        for (i = 0; i < point_bit; i++) {
            number = number * 10.0;                                             // 放大10^point_bit倍
        }
        // 四舍五入并转换为整数
        data_float = (int)(number + 0.5);                                       

        // 整数部分转为字符串
        bit = 0;
        if (data_int == 0) {
            // 整数部分为0，直接写'0'
            *str++ = '0';
        } else {
            // 正常转换整数部分
            while(data_int > 0) {
                data_temp[bit++] = data_int % 10;                              
                data_int /= 10;
            }
            while(bit > 0) {
                *str++ = (data_temp[bit - 1] + 0x30);                          
                bit--;
            }
        }

        // 小数部分转为字符串
        if(point_bit > 0)  // 【关键修改】: 移除了 `&& data_float != 0` 条件
        {
            *str++ = '.'; // 添加小数点
            
            if (data_float == 0) {
                // 小数部分为0，直接补足指定位数的'0'
                for (i = 0; i < point_bit; i++) {
                    *str++ = '0';
                }
            } else {
                // 正常转换小数部分
                bit = 0;
                // 将放大后的小数部分数字倒序存入缓冲区
                while(data_float > 0 && bit < point_bit) {
                    data_temp[bit++] = data_float % 10;
                    data_float /= 10;
                }
                // 如果转换后的位数不足指定位数，说明前面有缺失的0（例如0.004 -> 4），需要补足。
                while (bit < point_bit) {
                    data_temp[bit++] = 0; // 向前补0
                }
                // 从缓冲区倒序输出，得到正确顺序的小数位
                while(bit > 0) {
                    *str++ = (data_temp[bit - 1] + 0x30);
                    bit--;
                }
            }
        }

    } while(0);

    // 【统一终止符设置】
    if (str != NULL) {
        *str = '\0';
    }
}