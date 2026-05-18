#include "motor.h"

// 假设系统时钟为 32MHz (根据你的代码推算)
#define CPU_FREQ 32000000

// 1. 定义硬件映射表 (根据你的 syscfg 实际命名修改)
static Motor_Config_t motors[MOTOR_COUNT] = {
    [MOTOR_1] = {
        .timer_inst = TIMER_1_INST,
        .en   = {GPIO_MOTOR1_PIN_EN_PORT,   GPIO_MOTOR1_PIN_EN_PIN},
        .dir  = {GPIO_MOTOR1_PIN_DIR_PORT,  GPIO_MOTOR1_PIN_DIR_PIN},
        .step = {GPIO_MOTOR1_PIN_STEP_PORT, GPIO_MOTOR1_PIN_STEP_PIN},
        .ms1  = {GPIO_MOTOR1_PIN_MS1_PORT, GPIO_MOTOR1_PIN_MS1_PIN},
        .ms2  = {GPIO_MOTOR1_PIN_MS2_PORT, GPIO_MOTOR1_PIN_MS2_PIN},
        .ms3  = {GPIO_MOTOR1_PIN_MS3_PORT, GPIO_MOTOR1_PIN_MS3_PIN}, 
        .current_subdiv = STEP_1_8
    },
    [MOTOR_2] = {
        .timer_inst = TIMER_2_INST,
        .en   = {GPIO_MOTOR2_PIN2_EN_PORT,   GPIO_MOTOR2_PIN2_EN_PIN},
        .dir  = {GPIO_MOTOR2_PIN2_DIR_PORT,  GPIO_MOTOR2_PIN2_DIR_PIN},
        .step = {GPIO_MOTOR2_PIN2_STEP_PORT, GPIO_MOTOR2_PIN2_STEP_PIN},
        .ms1  = {GPIO_MOTOR2_PIN2_MS1_PORT, GPIO_MOTOR2_PIN2_MS1_PIN},
        .ms2  = {GPIO_MOTOR2_PIN2_MS2_PORT, GPIO_MOTOR2_PIN2_MS2_PIN},
        .ms3  = {GPIO_MOTOR2_PIN2_MS3_PORT, GPIO_MOTOR2_PIN2_MS3_PIN}, 
        .current_subdiv = STEP_1_8
    }
};

static Motor_Subdiv_t g_current_subdiv = STEP_1_8;

// 辅助函数：根据频率更新定时器周期
// static void UpdateTimerFreq(Motor_ID_t id, uint32_t freq) {
//     if (freq == 0) return;
//     // 脉冲翻转两次为一个周期，所以定时器中断频率应该是脉冲频率的 2 倍
//     uint32_t loadVal = CPU_FREQ / (2 * freq);
//     DL_TimerG_setLoadValue(motors[id].timer_inst, loadVal);
// }

void UpdateTimerFreq(Motor_ID_t id, uint32_t freq) {
    if (freq == 0) return;

    GPTIMER_Regs* timer_inst = motors[id].timer_inst;
    
    // 1. 算出理想状态下的 tick 数
    uint32_t raw_ticks = CPU_FREQ / (2 * freq);
    
    // 2. 软件模拟分频系数，默认为 1（不分频）
    uint32_t s_div = 1;

    // 3. 核心：如果 tick 数超过 16位最大值(65535)，就加大软件分频系数
    // 用 65000 留一点安全余量
    while (raw_ticks > 65000) {
        s_div++;
        raw_ticks = CPU_FREQ / (2 * freq * s_div);
    }

    // 4. 把算好的软件分频倍数存入结构体
    motors[id].soft_prescaler = s_div;
    
    // 5. 将安全截断后的 tick 写入硬件（绝对不会溢出了）
    DL_TimerG_setLoadValue(timer_inst, raw_ticks);
}

void Motor_Init(void) {
    for(int i=0; i<MOTOR_COUNT; i++) {
        Motor_SetDirection(i, MOTOR_DIR_CW);
        Motor_ChangeSubdivision(i, STEP_1_16);
        Motor_SetEnable(i, false);
        DL_TimerG_stopCounter(motors[i].timer_inst);
    }
    
}

void Motor_Timer_Init(void)
{
    // 将 NVIC 中断使能移入此处
    NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_2_INST_INT_IRQN);
}

void Motor_SetEnable(Motor_ID_t id, bool enable) {
    // 通过索引 id 获取对应电机的硬件配置指针
    Motor_Config_t *m = &motors[id];

    if (enable) {
        // 使用结构体中存储的该电机特有的端口和引脚号
        DL_GPIO_clearPins(m->en.port, m->en.pin);
    } else {
        DL_GPIO_setPins(m->en.port, m->en.pin);
    }
}

void Motor_SetDirection(Motor_ID_t id, Motor_Dir_t dir) {
    // 获取对应电机的硬件配置指针
    Motor_Config_t *m = &motors[id];

    if (dir == MOTOR_DIR_CW) {
        DL_GPIO_setPins(m->dir.port, m->dir.pin);
    } else {
        DL_GPIO_clearPins(m->dir.port, m->dir.pin);
    }
}

bool Motor_IsRunning(Motor_ID_t id) {
    return motors[id].is_running;
}

void Motor_ChangeSubdivision(Motor_ID_t id, Motor_Subdiv_t target_subdiv) {
    Motor_Config_t *m = &motors[id]; // 获取对应电机的配置

    // --- 核心逻辑判断 ---
    if (target_subdiv == STEP_NEXT) {
        // 如果是 STEP_NEXT，则自动计算下一位（0->1->2->3->0）
        m->current_subdiv = (Motor_Subdiv_t)((m->current_subdiv + 1) % STEP_MODE_COUNT);
    } else {
        // 如果传入了具体细分（如 STEP_1_8），则直接赋值
        // 增加一个简单的安全检查，防止索引越界
        if (target_subdiv < STEP_MODE_COUNT) {
            m->current_subdiv = target_subdiv;
        }
    }

    // --- 硬件操作 ---
    // 因为 MS 引脚可能在不同 Port，必须逐个清除 [参考之前对结构体的修改]
    DL_GPIO_clearPins(m->ms1.port, m->ms1.pin);
    DL_GPIO_clearPins(m->ms2.port, m->ms2.pin);
    //DL_GPIO_clearPins(m->ms3.port, m->ms3.pin);
    
    // 根据确定后的 m->current_subdiv 设置引脚
    switch (m->current_subdiv) {
        case STEP_1_8: 
            break; 
        case STEP_1_16:
            DL_GPIO_setPins(m->ms1.port, m->ms1.pin);
            DL_GPIO_setPins(m->ms2.port, m->ms2.pin);
            break;
        case STEP_1_32:
            DL_GPIO_setPins(m->ms1.port, m->ms1.pin);
            break;
        case STEP_1_64:
            DL_GPIO_setPins(m->ms2.port, m->ms2.pin);
            break;
        default: break;
    }
}

void Motor_Step(Motor_ID_t id, uint32_t steps, uint32_t freq) {
    Motor_Config_t *m = &motors[id];
    if (m->is_running || steps == 0) return;
    
    m->total_steps = steps;
    m->current_step = 0;
    m->is_running = true;
    m->toggle_count = 0;
    m->accel_steps = 0; 
    
    //Motor_SetDirection(id, MOTOR_DIR_CW); // 显式锁定一个方向测试
    // 关键：确保引脚从低电平开始
    DL_GPIO_clearPins(m->step.port, m->step.pin);
    
    UpdateTimerFreq(id, freq);
    Motor_SetEnable(id, true);
    DL_TimerG_startCounter(m->timer_inst);
}

// 非阻塞启动：平滑加减速
void Motor_Step_Smooth(Motor_ID_t id, uint32_t steps, uint32_t start_freq, uint32_t target_freq, uint32_t accel_steps) {
    Motor_Config_t *m = &motors[id];
    if (m->is_running || steps == 0) return;

    m->total_steps = steps;
    m->current_step = 0;
    m->toggle_count = 0;
    m->start_freq = start_freq;
    m->target_freq = target_freq;
    m->accel_steps = (accel_steps * 2 > steps) ? (steps / 2) : accel_steps;
    m->is_running = true;

    UpdateTimerFreq(id, start_freq);
    Motor_SetEnable(id, true);
    DL_TimerG_startCounter(m->timer_inst);
}

/**
 * @brief 定时器中断处理逻辑 
 * 必须在相应的 TIMER_x_INST_IRQHandler 中调用
 */
void Motor_OnTimerInterrupt(Motor_ID_t id) {
    Motor_Config_t *m = &motors[id];
    if (!m->is_running) return;

    // 1. 物理翻转
    DL_GPIO_togglePins(m->step.port, m->step.pin);

    // 2. 累加翻转次数（每 2 次翻转代表 1 步）
    // 我们定义一个静态变量或者在结构体里增加一个 toggle_count
    //static uint32_t toggle_count[MOTOR_COUNT] = {0};
    m->toggle_count++;

    // 3. 计算当前步数 (每 2 次翻转 = 1 步)
    m->current_step = m->toggle_count / 2;

    // 4. 平滑曲线处理 (仅在完成一个完整脉冲，即偶数次翻转时更新频率)
    if (m->accel_steps > 0 && (m->toggle_count % 2 == 0)) {
        uint32_t current_f = m->target_freq;
        if (m->current_step < m->accel_steps) {
            current_f = m->start_freq + (m->target_freq - m->start_freq) * m->current_step / m->accel_steps;
        } else if (m->current_step >= (m->total_steps - m->accel_steps)) {
            uint32_t deaccel_i = m->total_steps - m->current_step;
            current_f = m->start_freq + (m->target_freq - m->start_freq) * deaccel_i / m->accel_steps;
        }
        UpdateTimerFreq(id, current_f);
    }

    // 5. 检查是否达到总翻转次数 (steps * 2)
    if (m->toggle_count >= (m->total_steps * 2)) {
        m->is_running = false;
        DL_TimerG_stopCounter(m->timer_inst);
        
        // 强制归零，确保引脚回到初始低电平
        DL_GPIO_clearPins(m->step.port, m->step.pin);
        Motor_SetEnable(id, false);
        
        // 重置本次运动的翻转计数
        //m->toggle_count = 0;
    }
}


// --- 新增：硬件定时器中断服务函数 ---
void TIMER_1_INST_IRQHandler(void) {
    // 检查是否是加载值中断 (Zero interrupt)
    switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST)) {
        case DL_TIMER_IIDX_ZERO:
            // 【核心修复：软件模拟分频拦截】
            motors[MOTOR_1].soft_counter++;
            // 如果还没达到分频倍数，直接 return 退出，不翻转引脚
            if (motors[MOTOR_1].soft_counter < motors[MOTOR_1].soft_prescaler) {
                return; 
            }
            // 达到倍数，计数器清零，放行下方的真实运动代码
            motors[MOTOR_1].soft_counter = 0;

            Motor_OnTimerInterrupt(MOTOR_1);
            break;
        default: break;
    }
}

void TIMER_2_INST_IRQHandler(void) {
    switch (DL_TimerG_getPendingInterrupt(TIMER_2_INST)) {
        case DL_TIMER_IIDX_ZERO:
            // 【核心修复：软件模拟分频拦截】
            motors[MOTOR_2].soft_counter++;
            // 如果还没达到分频倍数，直接 return 退出，不翻转引脚
            if (motors[MOTOR_2].soft_counter < motors[MOTOR_2].soft_prescaler) {
                return; 
            }
            // 达到倍数，计数器清零，放行下方的真实运动代码
            motors[MOTOR_2].soft_counter = 0;
            Motor_OnTimerInterrupt(MOTOR_2);
            break;
        default: break;
    }
}

//