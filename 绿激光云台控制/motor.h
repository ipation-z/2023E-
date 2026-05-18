#ifndef MOTOR_H
#define MOTOR_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

// 保持原有的枚举定义
typedef enum {
    STEP_1_8 = 0,
    STEP_1_16,
    STEP_1_32,
    STEP_1_64,
    STEP_MODE_COUNT,
    STEP_NEXT
} Motor_Subdiv_t;

typedef enum {
    MOTOR_DIR_CW = 0,
    MOTOR_DIR_CCW
} Motor_Dir_t;

// 定义电机索引，方便调用
typedef enum {
    MOTOR_1 = 0,
    MOTOR_2,
    MOTOR_COUNT
} Motor_ID_t;

typedef struct {
    GPIO_Regs* port;
    uint32_t pin;
} Motor_Pin_t;

typedef struct {
    GPTIMER_Regs* timer_inst; // 新增：指向硬件定时器实例 (如 TIMER_0_INST)
    Motor_Pin_t en;     // 使能引脚
    Motor_Pin_t dir;    // 方向引脚
    Motor_Pin_t step;   // 脉冲引脚
    Motor_Pin_t ms1;    // 细分 1
    Motor_Pin_t ms2;    // 细分 2
    Motor_Pin_t ms3;    // 细分 3
    Motor_Subdiv_t current_subdiv;

    // 状态控制变量
    uint32_t total_steps;    // 剩余总步数
    uint32_t current_step;  // 当前已走步数
    uint32_t step_delay;    // 当前频率对应的延迟计数
    uint32_t last_tick;     // 上一次翻转的时间戳
    bool is_running;        // 电机运行标志
    bool pin_state;         // 脉冲引脚当前状态 (高/低)
    uint32_t toggle_count;  //引脚翻转次数

    // 平滑运动参数
    uint32_t start_freq;
    uint32_t target_freq;
    uint32_t accel_steps;

    // 【新增】软件模拟分频变量
    uint32_t soft_prescaler; // 软件算出的分频倍数
    uint32_t soft_counter;   // 中断里用来累加的计数器
    
} Motor_Config_t;

void Motor_Init(void); // 初始化所有电机
void Motor_SetEnable(Motor_ID_t id, bool enable);
void Motor_SetDirection(Motor_ID_t id, Motor_Dir_t dir);
void Motor_ChangeSubdivision(Motor_ID_t id, Motor_Subdiv_t target_subdiv);
void Motor_Step(Motor_ID_t id, uint32_t steps, uint32_t freq);

// 返回电机是否正在运行
bool Motor_IsRunning(Motor_ID_t id);

// 新增中断处理逻辑声明
void Motor_OnTimerInterrupt(Motor_ID_t id);

/**
 * @brief 平滑旋转指定步数（带线性加减速）
 * @param steps 总步数
 * @param start_freq 起始频率 (Hz)
 * @param target_freq 运行最高频率 (Hz)
 * @param accel_steps 加速阶段占用的步数
 */
void Motor_Step_Smooth(Motor_ID_t id, uint32_t steps, uint32_t start_freq, uint32_t target_freq, uint32_t accel_steps);

/** 
 * @brief 初始化马达计数器
 */
void Motor_Timer_Init();
#endif