#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>

void Delay_ms(uint32_t ms);

void func_int_to_str(char *str, int32_t number);
void func_uint_to_str(char *str, uint32_t number);
void func_double_to_str(char *str, double number, uint8_t point_bit);

void Sys_Init(void);

void Error_Handler(void);

#define ASSERT(expr)  ((expr) ? (void)0 : Error_Handler())

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define TOGGLE_BIT(REG, BIT)  ((REG) ^= (BIT))

#endif