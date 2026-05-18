#ifndef __HEADFILE_H_
#define __HEADFILE_H_
 
#include "ti_msp_dl_config.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
 
#include "common.h"
#include "font.h"




void Delay_ms(uint32_t ms);
void SysTick_Handler(void);
void func_int_to_str (char *str, int32_t number);
void func_uint_to_str (char *str, uint32_t number);
void func_double_to_str (char *str, double number, uint8_t point_bit);

#endif