#include "pid_control.h"

// 动态调整过程点数量的参数：设为1就是直接跑终点，设为4就是分4段，最高可设为10
uint8_t g_waypoint_count = 4;

// ------------------------------------------------------------
//  预存的标定点（电机绝对步数） —— 用于免校准直接调试
//  赛场上通过按键进行现场校准时，直接覆写数组里对应下标的值即可
// ------------------------------------------------------------
StepPoint_t g_calib_step_points[CALIB_POINT_COUNT] = {
    { -172, -152 },  // 索引 0: 左上
    {   32, -144 },  // 索引 1: 右上
    {   36,   84 },  // 索引 2: 右下
    { -176,   76 },  // 索引 3: 左下
    {   22,   -2 },  // 索引 4: 预存的中心原点
    {    0,    0 }   // 索引 5: 绝对机械零点
};

// StepPoint_t g_calib_step_points[CALIB_POINT_COUNT] = {
//     {  -54,  -60 },  // 索引 0: 预存的左上角顶点步数 (请替换为你平时调试的实际步数)
//     {  67,  -51 },  // 索引 1: 预存的右上角顶点步数
//     {  71,  61 },  // 索引 2: 预存的右下角顶点步数
//     {  -60,  66 },  // 索引 3: 预存的左下角顶点步数
//     {     11,     -1 },  // 索引 4: 预存的中心原点
//     {     0,     0 }   // 索引 5: 绝对机械零点
// };

// 方案一：分段矢量“配额制”约束变量
bool g_quota_limit_enable = false;
int32_t g_quota_total_x = 0; // 当前倾斜边段在 X 轴的物理总步数配额
int32_t g_quota_total_y = 0; // 当前倾斜边段在 Y 轴的物理总步数配额
int32_t g_step_count_x = 0;  // X轴当前已走步数累加器
int32_t g_step_count_y = 0;  // Y轴当前已走步数累加器

// 引入我们在 main.c 中定义好的当前绝对步数
extern int32_t g_manual_step_x; 
extern int32_t g_manual_step_y;

// ============================================================
//  2023年全国大学生电子设计大赛 E题
//  激光笔光斑自动定位与追踪 —— PID 控制实现
// ============================================================

// ------------------------------------------------------------
//  第一题：预设目标坐标表
//
//  按题目说明，靶标通常为纸质靶面上的4个固定圆圈中心。
//  坐标系原点为摄像头画面左上角，x 向右，y 向下。
//  【需根据实际比赛靶标位置实测后填写】
// ------------------------------------------------------------

Point_t g_target_list[TARGET_COUNT] = {
    {  183, 66 },   // 目标 0：左上区域
    {  490, 61 },   // 目标 1：右上区域
    {  493, 360 },   // 目标 2：右下区域
    {  185, 371 },   // 目标 3：左下区域
    {  348, 223 },   // 目标 4：原点
    {  316, 221},   // 目标 5: 绝对原点(水平竖直坐标)
};

// Point_t g_target_list[TARGET_COUNT] = {
//     {  382, 268 },   // 目标 0：左上区域
//     {  382, 268 },   // 目标 1：右上区域
//     {  382, 268 },   // 目标 2：右下区域
//     {  382, 268 },   // 目标 3：左下区域
//     {  382, 268 },   // 目标 4：原点
//     {  382, 268},   // 目标 5: 绝对原点(水平竖直坐标)
// };

uint8_t g_calib_select_idx = 0;

// --- 启动某条边的绕行，自动加载预存/校准的配额 ---
void PID_StartEdgeTracking(uint8_t edge_idx) {
    if (edge_idx >= 4) return; // 确保边号只有 0, 1, 2, 3

    // 自动推算当前边的起点和终点索引 (0->1, 1->2, 2->3, 3->0 闭环)
    uint8_t start_idx = edge_idx;
    uint8_t end_idx   = (edge_idx + 1) % 4; 

    // 1. 直接从数组中提取步数坐标（没校准就是预存值，校准了就是覆写后的最新值）
    int32_t start_x = g_calib_step_points[start_idx].x;
    int32_t start_y = g_calib_step_points[start_idx].y;
    int32_t end_x   = g_calib_step_points[end_idx].x;
    int32_t end_y   = g_calib_step_points[end_idx].y;

    // 2. 实时计算当前这条边的“物理步数配额”
    g_quota_total_x = abs(end_x - start_x);
    g_quota_total_y = abs(end_y - start_y);

    // 3. 清零消耗计数器并开启拦截
    g_step_count_x = 0;
    g_step_count_y = 0;
    g_quota_limit_enable = true;
    
    // 4. 清空积分项，平滑起步
    g_ctrl.pid_x.integral = 0;
    g_ctrl.pid_y.integral = 0;
}

// --- 修改：判断是否真正到达了倾斜的拐点 ---
bool PID_IsEdgeCompleted(void) {
    if (!g_quota_limit_enable) return false;
    
    // 只有当 X 轴和 Y 轴的步数都跑满了各自的配额，才认为光斑精确到达了倾斜顶点
    return (g_step_count_x >= g_quota_total_x) && (g_step_count_y >= g_quota_total_y);
}

// ------------------------------------------------------------
//  按键逻辑处理：状态转移 (SW1)
//  IDLE -> CALIB -> TRACK_ABS -> TRACK_REL -> IDLE
// ------------------------------------------------------------
void PID_Handle_SW1_StateTransform(void) {
    // 先备份当前状态，防止被 PID_Stop 冲掉
    SysState_t current_state = g_ctrl.state;
    
    PID_Stop(); 
    
    switch (current_state) {
        case SYS_IDLE:
            g_ctrl.state = SYS_CALIBRATING;
            g_calib_select_idx = 0; 
            UART_Printf(UART_0_INST,"sys_calib");
            break;
        case SYS_CALIBRATING:
            // g_ctrl.state = SYS_TRACKING_ABS;
            // g_ctrl.target_idx = 0;
            // g_ctrl.target = g_target_list[0];
            // 现改为直接调用接口，以触发路径拆分和积分清理
            PID_SetTargetIndex(0);
            break;
        case SYS_TRACKING_ABS:
            g_ctrl.state = SYS_TRACKING_REL;
            break;
        case SYS_TRACKING_REL:
        default:
            g_ctrl.state = SYS_IDLE;
            break;
    }
}

// ------------------------------------------------------------
//  按键逻辑处理：切换校准点索引 (SW2)
// ------------------------------------------------------------
void PID_Handle_SW2_SwitchCalibIdx(void) {
    if (g_ctrl.state == SYS_CALIBRATING) {
        g_calib_select_idx = (g_calib_select_idx + 1) % CALIB_POINT_COUNT;
        // 可以在此处通过串口发回上位机提示当前在校准哪个点
    }
}

// ------------------------------------------------------------
//  按键逻辑处理：复位逻辑 (SW3)
// ------------------------------------------------------------

// --- 新增：用于深度备份被打断时的路径状态 ---
static Point_t backup_final_target;
static Point_t backup_sub_targets[10];
static uint8_t backup_sub_target_idx;
static uint8_t backup_sub_target_total;  // 【新增】备份总细分点数
static bool    backup_use_waypoints;     // 【新增】备份多段追踪使能标志

// void PID_Handle_SW3_ResetPress(void) {
//     if (g_ctrl.state != SYS_RESET) {
//         g_ctrl.saved_state = g_ctrl.state; // 备份当前模式
//         // // 【修正死锁】这里应该备份之前的“目标坐标”而不是“当前坐标”
//         // // 否则松手后，电机就把刚才停下的位置当成新目标，不会继续走完原来的路程
//         // g_ctrl.saved_target.x = g_ctrl.target.x; 
//         // g_ctrl.saved_target.y = g_ctrl.target.y;
//         g_ctrl.saved_target.x = g_ctrl.final_target.x; 
//         g_ctrl.saved_target.y = g_ctrl.final_target.y;
//         g_ctrl.state = SYS_RESET;

//         // 设置目标为原点（第5个点），并采用多段过程点移动
//         Generate_Waypoints(g_ctrl.current, g_target_list[4]);

//         // 清除历史积分，防止切目标瞬间电机暴冲
//         g_ctrl.pid_x.integral = 0.0f;
//         g_ctrl.pid_y.integral = 0.0f;
//     }
// }

void PID_Handle_SW3_ResetPress(void) {
    if (g_ctrl.state != SYS_RESET) {
        // 只有在非恢复状态下被打断，才做深度备份
        if (g_ctrl.state != SYS_MOVING) {
            g_ctrl.saved_state = g_ctrl.state; 
            g_ctrl.saved_target = g_ctrl.target; // 备份当时【正在追的细分点】
            
            // 深度备份整个路径数组，防止被接下来去原点的路线覆盖
            backup_final_target = g_ctrl.final_target;
            for(int i = 0; i < 10; i++) {
                backup_sub_targets[i] = g_ctrl.sub_targets[i];
            }
            backup_sub_target_idx   = g_ctrl.sub_target_idx;
            backup_sub_target_total = g_ctrl.sub_target_total;
            backup_use_waypoints    = g_ctrl.use_waypoints;
        }
        
        g_ctrl.state = SYS_RESET;

        // 【修改点】：临时修改细分数设为1，实现不增加过程点，直接跑到原点
        uint8_t temp_count = g_waypoint_count;
        g_waypoint_count = 1; 
        Generate_Waypoints(g_ctrl.current, g_target_list[4]);
        g_waypoint_count = temp_count; // 恢复之前的分段数量配置

        // 清除历史积分
        g_ctrl.pid_x.integral = 0.0f;
        g_ctrl.pid_y.integral = 0.0f;
    }
}

void PID_Handle_SW3_ResetRelease(void) {
    if (g_ctrl.state == SYS_RESET) {
        // // 松手时，不直接恢复模式，而是进入一个“回归中(SYS_MOVING)”的过渡状态
        // g_ctrl.state = SYS_MOVING; 
        
        // // 【修改点】：从当前位置直接重新规划回到刚才被打断时的目标，不增加过程点
        // uint8_t temp_count = g_waypoint_count;
        // g_waypoint_count = 1;
        // Generate_Waypoints(g_ctrl.current, g_ctrl.saved_target);
        // g_waypoint_count = temp_count; // 恢复配置
        // 1. 直接恢复被打断前的真实工作模式 (例如 SYS_TRACKING_ABS)
        g_ctrl.state = g_ctrl.saved_state; 
        
        // 2. 【核心修复】：将5个核心状态机变量原封不动地还回去
        g_ctrl.final_target = backup_final_target;
        for(int i = 0; i < 10; i++) {
            g_ctrl.sub_targets[i] = backup_sub_targets[i];
        }
        g_ctrl.sub_target_idx   = backup_sub_target_idx;
        g_ctrl.sub_target_total = backup_sub_target_total;
        g_ctrl.use_waypoints    = backup_use_waypoints;
        
        // 3. 【不增加点，直接到位】直接将当下的目标指向打断点
        g_ctrl.target = g_ctrl.saved_target;
        
        g_ctrl.pid_x.integral = 0.0f;
        g_ctrl.pid_y.integral = 0.0f;
    }
}

// ------------------------------------------------------------
//  按键逻辑处理：紧急停止 (SW4)
// ------------------------------------------------------------
void PID_Handle_SW4_Stop(void) {
    g_ctrl.state = SYS_IDLE;
    
    // 清空PID积分与相对误差状态
    g_ctrl.pid_x.integral = 0.0f;
    g_ctrl.pid_y.integral = 0.0f;
    g_ctrl.rel_err_x = 0.0f;
    g_ctrl.rel_err_y = 0.0f;
    PID_Stop();
}


// ------------------------------------------------------------
//  全局 PID 控制器实例
// ------------------------------------------------------------
PID_Control_t g_ctrl;

// ------------------------------------------------------------
//  内部辅助：单轴 PID 计算
//  输入：误差值（像素）
//  输出：控制量（浮点，单位像素，后续转换为步数）
// ------------------------------------------------------------
static float PID_Compute(PID_t *pid, float error) {
    // 比例项
    float p_out = pid->kp * error;

    // 积分项（带限幅，防止饱和）
    pid->integral += error;
    if (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
    if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;
    float i_out = pid->ki * pid->integral;

    // 微分项
    float d_out = pid->kd * (error - pid->prev_error);
    pid->prev_error = error;

    return p_out + i_out + d_out;
}

/*

(1) 水平角增量反解：$$d\theta_x = \frac{\cos^2\theta_x}{L} dx
$$直观理解： $d\theta_x$ 只受 $dx$ 影响，且随着角度 $\theta_x$ 增大，同样的 $dx$ 所需的转角会变小（因为边缘处激光点移动极快）。
(2) 俯仰角增量反解：$$d\theta_y = \frac{\cos\theta_x \cos^2\theta_y}{L} dy - (\sin\theta_y \cos\theta_y \tan\theta_x) d\theta_x$$
直观理解： 这里出现了耦合项。当你为了改变 $x$ 坐标而转动 $\theta_x$ 时，为了保持 $y$ 坐标不变（即 $dy=0$），$\theta_y$ 必须做出补偿运动。

*/
/**
 * @brief 解耦后的 PID 计算
 * @param err_x X轴像素误差
 * @param err_y Y轴像素误差
 * @param out_x [输出] 解耦后的 X 轴控制量
 * @param out_y [输出] 解耦后的 Y 轴控制量
 */
static void PID_Compute_Decoupled(float err_x, float err_y, float *out_x, float *out_y) {
    // // --- 1. 常规 PID 增量计算 (计算理想的 dx, dy) ---
    
    // // X 轴 PID
    // float p_x = g_ctrl.pid_x.kp * err_x;
    // g_ctrl.pid_x.integral += err_x;
    // if (g_ctrl.pid_x.integral > g_ctrl.pid_x.integral_max) g_ctrl.pid_x.integral = g_ctrl.pid_x.integral_max;
    // if (g_ctrl.pid_x.integral < -g_ctrl.pid_x.integral_max) g_ctrl.pid_x.integral = -g_ctrl.pid_x.integral_max;
    // float i_x = g_ctrl.pid_x.ki * g_ctrl.pid_x.integral;
    // float d_x = g_ctrl.pid_x.kd * (err_x - g_ctrl.pid_x.prev_error);
    // g_ctrl.pid_x.prev_error = err_x;
    // float dx = p_x + i_x + d_x;

    // // Y 轴 PID
    // float p_y = g_ctrl.pid_y.kp * err_y;
    // g_ctrl.pid_y.integral += err_y;
    // if (g_ctrl.pid_y.integral > g_ctrl.pid_y.integral_max) g_ctrl.pid_y.integral = g_ctrl.pid_y.integral_max;
    // if (g_ctrl.pid_y.integral < -g_ctrl.pid_y.integral_max) g_ctrl.pid_y.integral = -g_ctrl.pid_y.integral_max;
    // float i_y = g_ctrl.pid_y.ki * g_ctrl.pid_y.integral;
    // float d_y = g_ctrl.pid_y.kd * (err_y - g_ctrl.pid_y.prev_error);
    // g_ctrl.pid_y.prev_error = err_y;
    // float dy = p_y + i_y + d_y;

// --- 1. 常规 PID 增量计算 (计算理想的 dx, dy) ---
    
    // ================= 【X 轴 PID (爷爷加的智能刹车)】 =================
    float p_x = g_ctrl.pid_x.kp * err_x;
    
    // 积分分离：距离较远时不积分，防止严重过冲
    if (err_x > -30.0f && err_x < 30.0f) {
        g_ctrl.pid_x.integral += err_x;
    } else {
        g_ctrl.pid_x.integral = 0.0f;
    }
    
    // 过零清零：误差正负号一变（说明越过终点），立刻紧急清零刹车
    if ((err_x > 0.0f && g_ctrl.pid_x.prev_error < 0.0f) || (err_x < 0.0f && g_ctrl.pid_x.prev_error > 0.0f)) {
        g_ctrl.pid_x.integral = 0.0f;
    }

    if (g_ctrl.pid_x.integral > g_ctrl.pid_x.integral_max) g_ctrl.pid_x.integral = g_ctrl.pid_x.integral_max;
    if (g_ctrl.pid_x.integral < -g_ctrl.pid_x.integral_max) g_ctrl.pid_x.integral = -g_ctrl.pid_x.integral_max;
    float i_x = g_ctrl.pid_x.ki * g_ctrl.pid_x.integral;
    float d_x = g_ctrl.pid_x.kd * (err_x - g_ctrl.pid_x.prev_error);
    g_ctrl.pid_x.prev_error = err_x;
    float dx = p_x + i_x + d_x;

    // ================= 【Y 轴 PID (爷爷加的智能刹车)】 =================
    float p_y = g_ctrl.pid_y.kp * err_y;
    
    // 积分分离
    if (err_y > -30.0f && err_y < 30.0f) {
        g_ctrl.pid_y.integral += err_y;
    } else {
        g_ctrl.pid_y.integral = 0.0f;
    }
    
    // 过零清零
    if ((err_y > 0.0f && g_ctrl.pid_y.prev_error < 0.0f) || (err_y < 0.0f && g_ctrl.pid_y.prev_error > 0.0f)) {
        g_ctrl.pid_y.integral = 0.0f;
    }

    if (g_ctrl.pid_y.integral > g_ctrl.pid_y.integral_max) g_ctrl.pid_y.integral = g_ctrl.pid_y.integral_max;
    if (g_ctrl.pid_y.integral < -g_ctrl.pid_y.integral_max) g_ctrl.pid_y.integral = -g_ctrl.pid_y.integral_max;
    float i_y = g_ctrl.pid_y.ki * g_ctrl.pid_y.integral;
    float d_y = g_ctrl.pid_y.kd * (err_y - g_ctrl.pid_y.prev_error);
    g_ctrl.pid_y.prev_error = err_y;
    float dy = p_y + i_y + d_y;
    // --- 2. 雅可比逆变换 (去耦合) ---
    
    // 获取当前光斑相对于中心点的像素偏移
    float cur_x = (float)(g_ctrl.current.x - g_target_list[5].x);//g_target_list[5]存储绝对坐标原点
    float cur_y = (float)(g_ctrl.current.y - g_target_list[5].y);
    
    // 预计算中间变量
    float r2 = cur_x * cur_x + L_PX * L_PX; // L^2 + x^2
    
    // 根据逆雅可比矩阵推导：
    // d_theta_x = (cos^2_theta_x / L) * dx
    // 转换成像素坐标表达（利用 cos^2_theta_x = L^2 / (L^2 + x^2)）:
    *out_x = dx * (L_PX * L_PX / r2);

    // d_theta_y = (cos_theta_x * cos^2_theta_y / L) * dy - (补偿项)
    // 简化的耦合补偿公式，主要解决 x 轴转动对 y 轴的拉伸影响：
    float cos_tx = L_PX / sqrtf(r2);
    *out_y = (dy * cos_tx) - (cur_x * cur_y / r2) * (*out_x);
}


// ------------------------------------------------------------
//  内部辅助：将 PID 输出（像素控制量）转换为步数和频率
//  并驱动对应电机运动（非阻塞）
//
//  Motor_1 控制 X 轴，Motor_2 控制 Y 轴
//  1/8 细分：1步 ≈ 1.5像素 → steps = |output| * 2 / 3
// ------------------------------------------------------------
static void Drive_Axis(Motor_ID_t id, float pid_output) {
    if (Motor_IsRunning(id)) return; // 电机忙则跳过本次

    int32_t output_px = (int32_t)pid_output; // 取整
    //UART_Printf(UART_0_INST,"px:%d",output_px);
    // 死区判断：误差过小则不驱动
    if (abs(output_px) < DEAD_ZONE_PX) return;
    //UART_Printf(UART_0_INST,"drive it.");
    // 方向
    // --- 方向定义修改部分 ---
    Motor_Dir_t dir;
    if (id == MOTOR_1) {
        // Motor 1 (X轴): CW=左, CCW=右
        // Error > 0 (目标在右) -> 需要向右移动 -> CCW
        dir = (output_px > 0) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
    } else {
        // Motor 2 (Y轴): CCW=上, CW=下
        // Error > 0 (目标在下) -> 需要向下移动 -> CW
        dir = (output_px > 0) ? MOTOR_DIR_CW : MOTOR_DIR_CCW;
    }
    Motor_SetDirection(id, dir);
    // -----------------------

    // 步数：像素量 → 步数（1步 ≈ 1.5px → steps = px * 2/3）
    // 保证至少1步，避免原地打转
    uint32_t steps = (uint32_t)(abs(output_px) * PIXELS_PER_STEP_X_NUM
                                / PIXELS_PER_STEP_X_DEN);
    if (steps < 1) steps = 1;

    // 频率：误差越大速度越快，线性映射到 [MIN, MAX]
    uint32_t err_abs = (uint32_t)abs(output_px);
    uint32_t freq = MOTOR_FREQ_MIN + err_abs * 10; // 每像素误差+10Hz
    if (freq > MOTOR_FREQ_MAX) freq = MOTOR_FREQ_MAX;

    // 使用平滑启动（带加减速），步数较少时退化为普通步进
    // if (steps > (MOTOR_ACCEL_STEPS * 2)) {
    //     Motor_Step_Smooth(id, steps,
    //                       MOTOR_FREQ_MIN, freq,
    //                       MOTOR_ACCEL_STEPS);
    // } else {
    //     Motor_Step(id, steps, freq);
    // }
    Motor_Step(id, steps, freq);
}

// ------------------------------------------------------------
// 优化版：XY 轴同步插补驱动（解决阶梯状和平滑度问题）
// ------------------------------------------------------------
// #define MAX_STEPS_PER_FRAME  25  // 【关键参数】每次 PID 更新最多允许走的步数。建议根据摄像头帧率调整(20~40)

// static void Drive_Axes_Synchronized(float out_x, float out_y) {
//     // 1. 如果任一电机仍在运行，说明上一帧的切片还没走完，直接返回等下一帧
//     if (Motor_IsRunning(MOTOR_1) || Motor_IsRunning(MOTOR_2)) return;

//     int32_t px_x = (int32_t)out_x;
//     int32_t px_y = (int32_t)out_y;

//     // 将像素转化为绝对步数
//     uint32_t steps_x = (uint32_t)(abs(px_x) * PIXELS_PER_STEP_X_NUM / PIXELS_PER_STEP_X_DEN);
//     uint32_t steps_y = (uint32_t)(abs(px_y) * PIXELS_PER_STEP_Y_NUM / PIXELS_PER_STEP_Y_DEN);

//     if (steps_x == 0 && steps_y == 0) return;

//     // 2. 设定方向
//     Motor_Dir_t dir_x = (px_x > 0) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
//     Motor_Dir_t dir_y = (px_y > 0) ? MOTOR_DIR_CW : MOTOR_DIR_CCW;
//     Motor_SetDirection(MOTOR_1, dir_x);
//     Motor_SetDirection(MOTOR_2, dir_y);

//     // 3. 找出主导轴（步数最多的轴）
//     uint32_t max_steps = (steps_x > steps_y) ? steps_x : steps_y;

//     // === 核心优化 1：步数切片（防止长时间阻塞） ===
//     // 如果单次算出的步数过大，将路程缩短，剩下的留给下一帧 PID 继续走
//     if (max_steps > MAX_STEPS_PER_FRAME) {
//         float scale = (float)MAX_STEPS_PER_FRAME / (float)max_steps;
//         steps_x = (uint32_t)(steps_x * scale + 0.5f); // +0.5 为了四舍五入更平滑
//         steps_y = (uint32_t)(steps_y * scale + 0.5f);
//         max_steps = MAX_STEPS_PER_FRAME;
//     }

//     // 4. 计算主轴的运行频率（根据原始像素误差动态调速）
//     uint32_t original_max_err = (abs(px_x) > abs(px_y)) ? abs(px_x) : abs(px_y);
//     uint32_t main_freq = MOTOR_FREQ_MIN + original_max_err * 10; 
//     if (main_freq > MOTOR_FREQ_MAX) main_freq = MOTOR_FREQ_MAX;

//     // === 核心优化 2：XY 频率同步（软件插补） ===
//     // 为了让 X 和 Y 同时走完，速度（频率）必须和它们的步数成正比
//     // Freq = Steps * (Main_Freq / Max_Steps)
//     uint32_t freq_x = (max_steps == 0) ? 0 : (steps_x * main_freq / max_steps);
//     uint32_t freq_y = (max_steps == 0) ? 0 : (steps_y * main_freq / max_steps);

//     // 频率兜底保护：防止频率过低导致电机卡死或共振
//     uint32_t safe_min_freq = MOTOR_FREQ_MIN / 2; // 给一个安全下限（比如 100Hz）
//     if (steps_x > 0 && freq_x < safe_min_freq) freq_x = safe_min_freq;
//     if (steps_y > 0 && freq_y < safe_min_freq) freq_y = safe_min_freq;

//     // 5. 同时触发电机
//     if (steps_x > 0) Motor_Step(MOTOR_1, steps_x, freq_x);
//     if (steps_y > 0) Motor_Step(MOTOR_2, steps_y, freq_y);
// }

static void Drive_Axes_Synchronized(float out_x, float out_y) {
    // 1. 如果任一电机仍在运行，说明上一帧的切片还没走完，直接返回等下一帧 (非阻塞核心)
    if (Motor_IsRunning(MOTOR_1) || Motor_IsRunning(MOTOR_2)) return;

    int32_t px_x = (int32_t)out_x;
    int32_t px_y = (int32_t)out_y;

    // 将像素转化为绝对步数
    uint32_t steps_x = (uint32_t)(abs(px_x) * PIXELS_PER_STEP_X_NUM / PIXELS_PER_STEP_X_DEN);
    uint32_t steps_y = (uint32_t)(abs(px_y) * PIXELS_PER_STEP_Y_NUM / PIXELS_PER_STEP_Y_DEN);

    // ================= 【核心新增：矢量配额拦截】 =================
    if (g_quota_limit_enable) {
        // 拦截 X 轴
        if (g_step_count_x + steps_x > g_quota_total_x) {
            steps_x = g_quota_total_x - g_step_count_x; // 强制截断，只走剩余配额
        }
        g_step_count_x += steps_x; // 扣除配额
        
        // 拦截 Y 轴
        if (g_step_count_y + steps_y > g_quota_total_y) {
            steps_y = g_quota_total_y - g_step_count_y; 
        }
        g_step_count_y += steps_y; 

        // 积分清零保护：某轴配额耗尽后，强制关闭该轴的积分累积，防止原地打转积攒爆发
        if (steps_x == 0) g_ctrl.pid_x.integral = 0;
        if (steps_y == 0) g_ctrl.pid_y.integral = 0;
    }
    // =============================================================
    
    if (steps_x == 0 && steps_y == 0) return;

    // ================= 【相对误差模式使用更小的细分】 =================
    // 在过完配额检查后，如果是相对追踪(SYS_TRACKING_REL)模式，就把细分调得更细（这里用1/32细分）
    if (g_ctrl.state == SYS_TRACKING_REL) {
        Motor_ChangeSubdivision(MOTOR_1, STEP_1_32); // 切到 1/32 细分 (比原先的 1/8 细了4倍)
        Motor_ChangeSubdivision(MOTOR_2, STEP_1_32);
        steps_x = steps_x * 4; // 因为步子变细了，为了走一样远物理距离，步数要乘以 4
        steps_y = steps_y * 4;
    } else {
        Motor_ChangeSubdivision(MOTOR_1, STEP_1_16);  // 其他模式自动恢复原来的 1/8 细分
        Motor_ChangeSubdivision(MOTOR_2, STEP_1_16);
    }
    // ==============================================================================
    // 2. 设定方向
    Motor_Dir_t dir_x = (px_x > 0) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
    Motor_Dir_t dir_y = (px_y > 0) ? MOTOR_DIR_CW  : MOTOR_DIR_CCW; 
    
    Motor_SetDirection(MOTOR_1, dir_x);
    Motor_SetDirection(MOTOR_2, dir_y);

    // ================= 【频率等比例插补计算】 =================
    // 找出步数较多的那个轴（主导轴）
    uint32_t max_steps = (steps_x > steps_y) ? steps_x : steps_y;
    
    // 设定主导轴的基准运行频率（重要：因为去掉了加减速，启动频率不宜过高，需防堵转）
    uint32_t BASE_FREQ = 360;  // 建议范围 400 ~ 800，根据电机实际承载能力微调

    // ================= 【配合更小的细分，频率要加快】 =================
    if (g_ctrl.state == SYS_TRACKING_REL) {
        BASE_FREQ = BASE_FREQ * 0.5; // 因为细分小了4倍，想保持原本的速度，发脉冲的频率也得快4倍
    }
    // ==================================================================================

    if(g_ctrl.state == SYS_RESET)
    {
        BASE_FREQ = BASE_FREQ * 2;
    }
    // 根据步数比例，动态压缩 X 轴和 Y 轴的频率
    // 保证它们严格同起同停，走出平滑斜线
    uint32_t freq_x = BASE_FREQ * steps_x / max_steps;
    uint32_t freq_y = BASE_FREQ * steps_y / max_steps;

    // 安全防线：防止路程极短的轴被算出极低频率（甚至 0Hz）导致定时器卡死溢出
    if (steps_x > 0 && freq_x < 20) freq_x = 20;
    if (steps_y > 0 && freq_y < 20) freq_y = 20;

    // 分别下发指令启动硬件定时器 (使用普通的 Motor_Step)
    if (steps_x > 0) {
        Motor_Step(MOTOR_1, steps_x, freq_x);
    }
    if (steps_y > 0) {
        Motor_Step(MOTOR_2, steps_y, freq_y);
    }
}

/**
 * @brief 生成过程目标点（起点与终点的 1/3 和 2/3 处）
 */
static void Generate_Waypoints(Point_t start, Point_t end) {
    // 1. 获取动态设置的点数，并做好保护防止超出数组上限
    uint8_t count = g_waypoint_count;
    if (count < 1) count = 1;
    if (count > 10) count = 10;
    
    g_ctrl.sub_target_total = count; // 记录到结构体里
    
    // 2. 动态等比例平分线段
    for (int i = 0; i < count - 1; i++) {
        g_ctrl.sub_targets[i].x = start.x + (i + 1) * (end.x - start.x) / count;
        g_ctrl.sub_targets[i].y = start.y + (i + 1) * (end.y - start.y) / count;
    }
    // 最终点: 目标本身（放在数组当前使用的最后一位）
    g_ctrl.sub_targets[count - 1] = end;

    // 3. 重置状态
    g_ctrl.sub_target_idx = 0;
    g_ctrl.final_target = end;
    g_ctrl.target = g_ctrl.sub_targets[0]; // 将当前目标设为第一个过程点
    g_ctrl.use_waypoints = true;
}

// ============================================================
//  公开 API 实现
// ============================================================

void PID_Init(void) {
    // ---- X 轴 PID 参数（可根据实测调整）----OpenMV
    g_ctrl.pid_x.kp           = 0.7f;
    g_ctrl.pid_x.ki           = 0.0006f;
    g_ctrl.pid_x.kd           = 0.06f;
    g_ctrl.pid_x.integral     = 0.0f;
    g_ctrl.pid_x.prev_error   = 0.0f;
    g_ctrl.pid_x.integral_max = 50.0f; // 最大积分量（像素）

    // ---- Y 轴 PID 参数 ----
    g_ctrl.pid_y.kp           = 0.7f;
    g_ctrl.pid_y.ki           = 0.0006f;
    g_ctrl.pid_y.kd           = 0.06f;
    g_ctrl.pid_y.integral     = 0.0f;
    g_ctrl.pid_y.prev_error   = 0.0f;
    g_ctrl.pid_y.integral_max = 30.0f;

    // ---- 初始目标：第一个预设点 ----
    g_ctrl.target_idx = 0;
    g_ctrl.target     = g_target_list[0];

    // ---- 当前坐标：先假设在中心（摄像头视野中心） ----
    g_ctrl.current.x = CAM_CENTER_X;
    g_ctrl.current.y = CAM_CENTER_Y;

    g_ctrl.state    = SYS_IDLE;
    g_ctrl.new_data = false;

    //注意！这里写5是因为默认第一个点是绝对原点
    g_calib_select_idx = 5;
}

// 参数备份
// void PID_Init(void) {
//     // ---- X 轴 PID 参数（可根据实测调整）----
//     g_ctrl.pid_x.kp           = 0.545f;
//     g_ctrl.pid_x.ki           = 0.0105f;
//     g_ctrl.pid_x.kd           = 0.0567f;
//     g_ctrl.pid_x.integral     = 0.0f;
//     g_ctrl.pid_x.prev_error   = 0.0f;
//     g_ctrl.pid_x.integral_max = 30.0f; // 最大积分量（像素）

//     // ---- Y 轴 PID 参数 ----
//     g_ctrl.pid_y.kp           = 0.605f;
//     g_ctrl.pid_y.ki           = 0.0065f;
//     g_ctrl.pid_y.kd           = 0.043f;
//     g_ctrl.pid_y.integral     = 0.0f;
//     g_ctrl.pid_y.prev_error   = 0.0f;
//     g_ctrl.pid_y.integral_max = 30.0f;

//     // ---- 初始目标：第一个预设点 ----
//     g_ctrl.target_idx = 0;
//     g_ctrl.target     = g_target_list[0];

//     // ---- 当前坐标：先假设在中心（摄像头视野中心） ----
//     g_ctrl.current.x = CAM_CENTER_X;
//     g_ctrl.current.y = CAM_CENTER_Y;

//     g_ctrl.state    = SYS_IDLE;
//     g_ctrl.new_data = false;

//     g_calib_select_idx = 0;
// }

// void PID_Init(void) {
//     // ---- X 轴 PID 参数（可根据实测调整）---- CamMV数据
//     g_ctrl.pid_x.kp           = 0.7f;
//     g_ctrl.pid_x.ki           = 0.007f;
//     g_ctrl.pid_x.kd           = 0.09f;
//     g_ctrl.pid_x.integral     = 0.0f;
//     g_ctrl.pid_x.prev_error   = 0.0f;
//     g_ctrl.pid_x.integral_max = 30.0f; // 最大积分量（像素）

//     // ---- Y 轴 PID 参数 ----
//     g_ctrl.pid_y.kp           = 0.7f;
//     g_ctrl.pid_y.ki           = 0.007f;
//     g_ctrl.pid_y.kd           = 0.09f;
//     g_ctrl.pid_y.integral     = 0.0f;
//     g_ctrl.pid_y.prev_error   = 0.0f;
//     g_ctrl.pid_y.integral_max = 30.0f;

//     // ---- 初始目标：第一个预设点 ----
//     g_ctrl.target_idx = 0;
//     g_ctrl.target     = g_target_list[0];

//     // ---- 当前坐标：先假设在中心（摄像头视野中心） ----
//     g_ctrl.current.x = CAM_CENTER_X;
//     g_ctrl.current.y = CAM_CENTER_Y;

//     g_ctrl.state    = SYS_IDLE;
//     g_ctrl.new_data = false;

//     g_calib_select_idx = 0;
// }

void PID_Update(void) {
    // 只有收到新坐标数据时才做计算，避免重复驱动
    if (!g_ctrl.new_data) return;
    g_ctrl.new_data = false;

        // 如果是 IDLE 状态，不驱动电机
    if (g_ctrl.state == SYS_IDLE) return;

    float err_x = 0, err_y = 0;
    float ctrl_x = 0, ctrl_y = 0;

// --- 1. 误差来源选择 ---
    if (g_ctrl.state == SYS_TRACKING_ABS || g_ctrl.state == SYS_MOVING || g_ctrl.state == SYS_RESET) {
        err_x = (float)(g_ctrl.target.x - g_ctrl.current.x);
        err_y = (float)(g_ctrl.target.y - g_ctrl.current.y);
    } 
    else if (g_ctrl.state == SYS_TRACKING_REL) {
        err_x = g_ctrl.rel_err_x;
        err_y = g_ctrl.rel_err_y;
    }
    else if (g_ctrl.state == SYS_CALIBRATING) {
        //g_target_list[g_calib_select_idx].x = g_ctrl.current.x;
        //g_target_list[g_calib_select_idx].y = g_ctrl.current.y;
        return;
    }

    // --- 2. 状态转移逻辑 (核心修正部分) ---
    
    // // 检查是否进入死区（到达目标）
    // if (abs((int)err_x) <= DEAD_ZONE_PX && abs((int)err_y) <= DEAD_ZONE_PX) {
    //     // 如果当前处于绝对追踪，说明抵达当前靶心，自动切下一个
    //     if (g_ctrl.state == SYS_TRACKING_ABS) {
            
    //         // 清除积分，防止切换目标时过冲
    //         g_ctrl.pid_x.integral = 0;
    //         g_ctrl.pid_y.integral = 0;

    //         // --- 自动切换下一个目标 ---
    //         g_ctrl.target_idx = (g_ctrl.target_idx + 1) % CYCLE_COUNT; 
    //         g_ctrl.target = g_target_list[g_ctrl.target_idx];
            
    //         // 注意：这里不要再去改变 g_ctrl.state，让它保持 SYS_TRACKING_ABS 模式即可
    //         // 下一帧计算时误差拉大，自然会驱动电机前往新目标
            
    //         tft180_printf(0,7*16, "arr,Next: %d\r\n", g_ctrl.target_idx);
    //     }
    //     return; 
    // }

    // --- 2. 状态转移逻辑 (引入多段追踪) ---
    
    // 检查是否进入死区（到达目标）
    if (abs((int)err_x) <= DEAD_ZONE_PX && abs((int)err_y) <= DEAD_ZONE_PX) {
        
        // 2.1 如果正在多段追踪，且尚未到达最终点，则切换到下一个过程点
        if (g_ctrl.use_waypoints && g_ctrl.sub_target_idx < (g_ctrl.sub_target_total - 1)) {
            g_ctrl.sub_target_idx++;
            g_ctrl.target = g_ctrl.sub_targets[g_ctrl.sub_target_idx];
            
            // 清除积分，防止切换过程点瞬间 PID 积分累积导致过冲
            //g_ctrl.pid_x.integral = 0;
            //g_ctrl.pid_y.integral = 0;
            return; // 暂退，下一帧开始向新的过程点移动
        }

        // 2.2 如果过程点已全部走完 (到达了最终点)
        g_ctrl.use_waypoints = false;

        // 如果现在是“正在回到打断点”的过程中到达了该点
        if (g_ctrl.state == SYS_MOVING) {
            // 1. 恢复回原来的主状态 (比如 SYS_TRACKING_ABS)
            g_ctrl.state = g_ctrl.saved_state;
            
            // 2. 把被覆盖的原始任务路径完整还原回去
            g_ctrl.final_target = backup_final_target;
            for(int i = 0; i < 4; i++) {
                g_ctrl.sub_targets[i] = backup_sub_targets[i];
            }
            g_ctrl.sub_target_idx = backup_sub_target_idx;
            g_ctrl.target = g_ctrl.sub_targets[g_ctrl.sub_target_idx];
            g_ctrl.use_waypoints = true; // 重新启用分段移动
            
            // 清理积分，下一帧将无缝衔接，继续沿着原路径走剩余的路
            g_ctrl.pid_x.integral = 0;
            g_ctrl.pid_y.integral = 0;
            return; 
        }
        
        // 如果当前处于绝对追踪，说明抵达当前靶心，自动切下一个靶心
        if (g_ctrl.state == SYS_TRACKING_ABS) {
            g_ctrl.pid_x.integral = 0;
            g_ctrl.pid_y.integral = 0;

            // 切换下一个主目标点
            g_ctrl.target_idx = (g_ctrl.target_idx + 1) % CYCLE_COUNT; 
            
            // 【关键修改】不要直接赋 g_ctrl.target，而是重新生成三段路径
            Generate_Waypoints(g_ctrl.current, g_target_list[g_ctrl.target_idx]);
            
            //tft180_printf(0,7*16, "arr,Next: %d\r\n", g_ctrl.target_idx);
        }
        return; 
    }

    // PID 计算
    //ctrl_x = PID_Compute(&g_ctrl.pid_x, err_x);
    //ctrl_y = PID_Compute(&g_ctrl.pid_y, err_y);
    
    // 使用新的解耦函数
    PID_Compute_Decoupled(err_x, err_y, &ctrl_x, &ctrl_y);
    
    //UART_Printf(UART_0_INST,"cx:%.3f,cy:%.3f",ctrl_x,ctrl_y);
    // 驱动电机（非阻塞，若电机忙则本次跳过）
    //Drive_Axis(MOTOR_1, ctrl_x);
    //Drive_Axis(MOTOR_2, ctrl_y);
    Drive_Axes_Synchronized(ctrl_x, ctrl_y);
}

void PID_NextTarget(void) {
    // 循环切换到下一个目标点
    g_ctrl.target_idx = (g_ctrl.target_idx + 1) % CYCLE_COUNT;
    PID_SetTargetIndex(g_ctrl.target_idx);
}

// void PID_SetTargetIndex(uint8_t idx) {
//     if (idx >= TARGET_COUNT) return;

//     g_ctrl.target_idx = idx;
//     g_ctrl.target     = g_target_list[idx];

//     // 切换目标时清除积分（避免旧误差对新目标产生干扰）
//     g_ctrl.pid_x.integral = 0.0f;
//     g_ctrl.pid_y.integral = 0.0f;
//     g_ctrl.pid_x.prev_error = 0.0f;
//     g_ctrl.pid_y.prev_error = 0.0f;

//     g_ctrl.state = SYS_TRACKING_ABS;

// }

void PID_SetTargetIndex(uint8_t idx) {
    if (idx >= TARGET_COUNT) return;

    g_ctrl.target_idx = idx;
    
    // 原代码: g_ctrl.target = g_target_list[idx];
    // 新代码: 动态生成过程点
    Generate_Waypoints(g_ctrl.current, g_target_list[idx]);

    g_ctrl.pid_x.integral = 0.0f;
    g_ctrl.pid_y.integral = 0.0f;
    g_ctrl.pid_x.prev_error = 0.0f;
    g_ctrl.pid_y.prev_error = 0.0f;

    g_ctrl.state = SYS_TRACKING_ABS;

    UART_SendPacket(UART_0_INST, 0x01, g_ctrl.final_target.x, g_ctrl.final_target.y);
}


void PID_StartTracking(void) {
    // 清除积分，进入追踪模式
    g_ctrl.pid_x.integral   = 0.0f;
    g_ctrl.pid_y.integral   = 0.0f;
    g_ctrl.pid_x.prev_error = 0.0f;
    g_ctrl.pid_y.prev_error = 0.0f;
    g_ctrl.state = SYS_TRACKING_ABS;
}

void PID_Stop(void) {
    g_ctrl.state = SYS_IDLE;
    g_ctrl.pid_x.integral = 0.0f;
    g_ctrl.pid_y.integral = 0.0f;
    // 注意：步进电机当前运动步数会自然走完，
    // 不强制中断（以免丢失步数造成机械冲击）
}

void PID_StartCalibration(void) {
    g_ctrl.state = SYS_CALIBRATING;
    // 逻辑：例如将当前坐标记录为原点，或移动到机械限位
}

void PID_SaveCalibrationPoint(uint8_t idx) {
    if (idx >= TARGET_COUNT) return;

    // 将当前反馈的坐标保存到对应的预设点中
    g_target_list[idx].x = g_ctrl.current.x;
    g_target_list[idx].y = g_ctrl.current.y;

    // (可选) 立即更新当前目标，方便观察效果
    // if (g_ctrl.target_idx == idx) {
    //     g_ctrl.target = g_target_list[idx];
    // }
    
    // 串口打印确认
    // UART_Printf(UART_0_INST, "Saved to Index %d: [%d, %d]\r\n", 
    //             idx, g_target_list[idx].x, g_target_list[idx].y);
}
