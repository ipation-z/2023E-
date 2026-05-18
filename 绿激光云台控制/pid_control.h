#ifndef PID_CONTROL_H
#define PID_CONTROL_H

#include "motor.h"
#include "uart.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>  // for abs()

// ============================================================
//  2023年全国大学生电子设计大赛 E题
//  激光笔光斑自动定位与追踪控制
//
//  题目要求：
//  第一题：光斑移动到指定坐标（4个预设目标点）
//  第二题：光斑自动追踪并对准运动目标
// ============================================================

// ------------------------------------------------------------
//  坐标与分辨率定义
//  摄像头分辨率（根据实际摄像头配置修改）
// ------------------------------------------------------------
#define CAM_WIDTH   640
#define CAM_HEIGHT  480

// 中心点坐标（用于零位校准）
#define CAM_CENTER_X  (CAM_WIDTH  / 2)   //
#define CAM_CENTER_Y  (CAM_HEIGHT / 2)   //

// ------------------------------------------------------------
//  运动参数
//  1/8 细分下，1步 ≈ 1.5 像素（由实测得到）
//  因此：像素误差 → 步数 = error / 1.5 = error * 2 / 3
// ------------------------------------------------------------
#define PIXELS_PER_STEP_X_NUM   1    // 步数 = err * NUM / DEN
#define PIXELS_PER_STEP_X_DEN   3
#define PIXELS_PER_STEP_Y_NUM   1
#define PIXELS_PER_STEP_Y_DEN   3

// 动态死区：如果是相对追踪(SYS_TRACKING_REL，要准)，死区就是 1 像素；
// 如果是别的模式(如绝对坐标大范围移动，要快要稳)，死区就是 4 像素。
#define DEAD_ZONE_PX  ((g_ctrl.state == SYS_TRACKING_REL) ? 1 : 4)

// 步进频率范围（Hz）
#define MOTOR_FREQ_MIN      50
#define MOTOR_FREQ_MAX      200
#define MOTOR_ACCEL_STEPS   20   // 加减速步数

// 1米距离下，若摄像头视野宽度约0.6米(320px)，则 L 约为 500 像素
#define L_PX    500.0f

// 默认 4 个顶点 + 1 个原点 + 1个绝对原点= 6个点
#define CALIB_POINT_COUNT 6

// ------------------------------------------------------------
//  第一题：预设目标坐标（像素坐标，根据实际标靶位置修改）
//  按题目要求，通常为4个角点 + 中心点 + 绝对原点
// ------------------------------------------------------------
#define TARGET_COUNT    6

#define CYCLE_COUNT     4

// 蜂鸣器拉低时响 (Active Low)
#define BUZZER_ON()         DL_GPIO_clearPins(GPIO_WARN_PORT,GPIO_WARN_PIN_BUZZER_PIN)
#define BUZZER_OFF()        DL_GPIO_setPins(GPIO_WARN_PORT, GPIO_WARN_PIN_BUZZER_PIN)

// LED控制：假设高电平亮，低电平灭 (若你的板子是低电平亮，将 clear 和 set 互换即可)
#define LED_ON()            DL_GPIO_setPins(GPIO_WARN_PORT, GPIO_WARN_PIN_LED_PIN)
#define LED_OFF()           DL_GPIO_clearPins(GPIO_WARN_PORT, GPIO_WARN_PIN_LED_PIN)

typedef struct {
    int16_t x;
    int16_t y;
} Point_t;

// 预设目标坐标表（在 pid_control.c 中定义）
// 根据实际比赛靶标位置填写
extern Point_t g_target_list[TARGET_COUNT];

// ------------------------------------------------------------
//  PID 控制器结构体（每轴独立）
// ------------------------------------------------------------
typedef struct {
    float kp;           // 比例系数
    float ki;           // 积分系数
    float kd;           // 微分系数

    float integral;     // 积分累计值
    float prev_error;   // 上一次误差（用于计算微分）
    float integral_max; // 积分限幅，防止积分饱和
} PID_t;

// ------------------------------------------------------------
//  系统状态枚举
// ------------------------------------------------------------

typedef enum {
    SYS_IDLE = 0,       // 待机状态
    // SYS_CALIBRATING,    // 校准模式
    // SYS_TRACKING_ABS,   // 绝对坐标追踪（总模式）
    SYS_TRACKING_REL,   // 相对坐标追踪
    // SYS_MOVING,         // 移动中（子状态/过程状态）
    // SYS_ARRIVED,        // 已到达（触发跳转）
    // SYS_RESET,          // 复位
    SYS_TRACKING_REL_2 //第二个相对坐标追踪
} SysState_t;

// ------------------------------------------------------------
//  PID 控制主结构体
// ------------------------------------------------------------
typedef struct {
    PID_t       pid_x;          // X 轴 PID
    PID_t       pid_y;          // Y 轴 PID

    Point_t     target;         // 当前目标坐标（像素）
    Point_t     current;        // 当前实测坐标（像素，由串口更新）

    // --- 新增：多段过程点追踪相关 ---
    Point_t     final_target;   // 这一轮追踪的最终目标点

// 【修改1】把数组容量从 4 改成 10，给动态调整留足空间
    Point_t     sub_targets[10]; 
    uint8_t     sub_target_idx; // 当前正在追踪的子目标点索引
    
    // 【修改2】加一个变量，用来动态记录这一次总共有几个点
    uint8_t     sub_target_total;

    bool        use_waypoints;  // 标志位：是否正在进行多段追踪

// 相对误差项（用于 SYS_TRACKING_REL）
    float       rel_err_x;
    float       rel_err_y;

    uint8_t     target_idx;     // 仅用于校准点的索引
    SysState_t  state;          // 当前运行状态
    bool        new_data;

    SysState_t  saved_state;    // 暂存按下前的状态
    Point_t     saved_target;   // 暂存按下前的目标位置
    
} PID_Control_t;

// 定义 32 位整型坐标，防止步数溢出
typedef struct {
    int32_t x;
    int32_t y;
} StepPoint_t;

// 开环校准的步数记录数组 (对应你的 CALIB_POINT_COUNT)
extern StepPoint_t g_calib_step_points[CALIB_POINT_COUNT];

// 边段限幅使能标志
extern bool g_edge_limit_enable;

// 声明边段限制初始化函数
void PID_StartEdgeTracking(uint8_t edge_idx);

// 检查是否已经到达当前边的终点
bool PID_IsEdgeCompleted(void);

// 全局控制器实例（在 pid_control.c 中定义）
extern PID_Control_t g_ctrl;

extern uint8_t g_calib_select_idx; 

// ------------------------------------------------------------
//  API 函数声明
// ------------------------------------------------------------

/**
 * @brief 初始化 PID 控制器（参数与状态清零）
 */
void PID_Init(void);

/**
 * @brief 根据当前误差计算 PID 输出（步数和频率），并驱动电机
 *        应在主循环中、收到新坐标后调用
 */
void PID_Update(void);

/**
 * @brief 第一题：切换到下一个预设目标点
 *        每次按键或收到切换指令时调用
 */
void PID_NextTarget(void);

/**
 * @brief 第一题：直接设置目标序号（0 ~ TARGET_COUNT-1）
 */
void PID_SetTargetIndex(uint8_t idx);

/**
 * @brief 第二题：进入追踪模式，目标坐标由串口实时更新
 */
void PID_StartTracking(void);

/**
 * @brief 停止所有运动，回到 IDLE 状态
 */
void PID_Stop(void);

/**
 * @brief 计算两点之间的像素距离（曼哈顿距离，快速估算）
 */
static inline int32_t Point_ManhattanDist(Point_t a, Point_t b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

/**
 * @brief 生成追踪时的过程追踪点
 */
static void Generate_Waypoints(Point_t start, Point_t end) ;

/**
 * @brief 存储当前点的坐标，传入idx下标
 */
void PID_SaveCalibrationPoint(uint8_t idx);
#endif // PID_CONTROL_H

// 外部按键接口声明
void PID_Handle_SW1_StateTransform(void); // 状态循环切换
// void PID_Handle_SW2_SwitchCalibIdx(void); // 切换校准点索引
// void PID_Handle_SW3_ResetPress(void);    // 复位按下
// void PID_Handle_SW3_ResetRelease(void);  // 复位松开
void PID_Handle_SW4_Stop(void);           // 紧急停止

void Indicator_Update(float,float);