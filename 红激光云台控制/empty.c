/*
//                            _ooOoo_  
//                           o8888888o  
//                           88" . "88  
//                           (| -_- |)  
//                            O\ = /O  
//                        ____/`---'\____  
//                      .   ' \\| |// `.  
//                       / \\||| : |||// \  
//                     / _||||| -:- |||||- \  
//                       | | \\\ - /// | |  
//                     | \_| ''\---/'' | |  
//                      \ .-\__ `-` ___/-. /  
//                   ___`. .' /--.--\ `. . __  
//                ."" '< `.___\_<|>_/___.' >'"".  
//               | | : `- \`.;`\ _ /`;.`/ - ` : | |  
//                 \ \ `-. \_ __\ /__ _/ .-` / /  
//         ======`-.____`-.___\_____/___.-`____.-'======  
//                            `=---='  
//  
//         .............................................  
//                  佛祖保佑             永无BUG 
//          佛曰:  
//                  写字楼里写字间，写字间里程序员；  
//                  程序人员写程序，又拿程序换酒钱。  
//                  酒醒只在网上坐，酒醉还来网下眠；  
//                  酒醉酒醒日复日，网上网下年复年。  
//                  但愿老死电脑间，不愿鞠躬老板前；  
//                  奔驰宝马贵者趣，公交自行程序员。  
//                  别人笑我忒疯癫，我笑自己命太贱；  
//                  不见满街漂亮妹，哪个归得程序员？
*/

#include "ti_msp_dl_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "LcdUI.h"
#include "ST7735.h"

#include "uart.h"
/* * 使用说明示例：
 * * 1. 接收与解析：
 * 当 rxIndex 累加到 7 且校验通过时：
 * UART_Packet pkt = UART_ParsePacket(rxBuffer);
 * if(pkt.isValid) {
 * int16_t current_x = pkt.x; // 直接获取解析好的短整型数据
 * }
 * * 2. 打印调试：
 * UART_Printf(UART_0_INST, "系统启动成功，当前模式: %d\r\n", mode);
 * * 3. 向上位机反馈：
 * // 发送一个模式为2，X为-50，Y为100的数据包
 * UART_SendPacket(UART_1_INST, 0x02, -50, 100); 
 */

#include "motor.h"

#include "pid_control.h"

uint32_t uart_rx_timeout = 0;  // 串口接收超时计数器

//x,y计步器
int32_t g_manual_step_x; 
int32_t g_manual_step_y;

/**
 * @brief 产生 +-1 像素的随机扰动误差
 * @return int8_t 返回 1 或 -1
 */
int8_t Generate_Pixel_Perturbation(void) {
    // rand() & 1 会随机取最低位，结果为 0 或 1
    // 如果为 1，则返回 1；如果为 0，则返回 -1
    return (rand() & 1) ? 1 : -1;
}

/**
 * @brief 按键处理通用抽象函数
 * @param port      GPIO端口 (例如 GPIO_MOTOR1_PIN_SW_PORT)
 * @param pin       GPIO引脚 (例如 GPIO_MOTOR1_PIN_SW_PIN)
 * @param msg       TFT显示的字符串
 * @param callback  按键按下后执行的回调函数
 */
void Button_Process(GPIO_Regs *port, uint32_t pin, const char* msg, void (*callback)(void)) {
    // 1. 检测按下 (低电平)
    if (DL_GPIO_readPins(port, pin) == 0) {
        
        // 2. 消抖处理
        delay_cycles(320000); 
        
        if (DL_GPIO_readPins(port, pin) == 0) {
            // 3. 执行显示逻辑
            //tft180_show_string(0, 6*16, (char*)msg);
            //统一显示在最底层
            tft180_printf(0,9*16,(char*)msg);
            
            // 4. 执行具体业务逻辑 (回调)
            if (callback != NULL) {
                callback();
            }
            
            // 5. 等待释放 (死循环阻塞，直到按键抬起)
            while(DL_GPIO_readPins(port, pin) == 0);
        }
    }
}

// ==================== 电机1 (控制 X轴 左右) ====================

// 对应按键: M2 SW1 -> 动作: 左
void Action_M1_Step_CW(void)
{
    if (!Motor_IsRunning(MOTOR_1)) 
    {
        Motor_SetDirection(MOTOR_1, MOTOR_DIR_CW);
        Motor_Step(MOTOR_1, 1, 100);
        
        // 向左移动，X坐标减少
        if(g_ctrl.state == SYS_CALIBRATING)
            g_manual_step_x -= 1;
    }
}

// 对应按键: M2 SW2 -> 动作: 右
void Action_M1_Step_CCW(void)
{
    if (!Motor_IsRunning(MOTOR_1)) 
    {
        Motor_SetDirection(MOTOR_1, MOTOR_DIR_CCW);
        Motor_Step(MOTOR_1, 1, 100);
        
        // 向右移动，X坐标增加
        if(g_ctrl.state == SYS_CALIBRATING)
            g_manual_step_x += 1;
    }
}


// ==================== 电机2 (控制 Y轴 上下) ====================

// 对应按键: M1 SW1 -> 动作: 上
void Action_M2_Step_CCW(void)
{
    if (!Motor_IsRunning(MOTOR_2)) 
    {
        Motor_SetDirection(MOTOR_2, MOTOR_DIR_CCW);
        Motor_Step(MOTOR_2, 1, 100);
        
        // 向上移动，Y坐标减少
        if(g_ctrl.state == SYS_CALIBRATING)
            g_manual_step_y -= 1;
    }
}

// 对应按键: M1 SW2 -> 动作: 下
void Action_M2_Step_CW(void)
{
    if (!Motor_IsRunning(MOTOR_2)) 
    {
        Motor_SetDirection(MOTOR_2, MOTOR_DIR_CW);
        Motor_Step(MOTOR_2, 1, 100);
        
        // 向下移动，Y坐标增加
        if(g_ctrl.state == SYS_CALIBRATING)
            g_manual_step_y += 1;
    }
}

// ------------------------------------------------------------
//  处理来自上位机的 UART 数据包
//  根据 mode 字段决定是更新坐标还是切换控制模式
// ------------------------------------------------------------
// ------------------------------------------------------------
//  处理来自上位机的 UART 数据包
// ------------------------------------------------------------
static void Handle_UART_Packet(UART_Packet *pkt) {
    if (!pkt->isValid) return;

    //tft180_printf(0, 9*16,"md:%d,x:%d,y:%d",pkt->mode,pkt->x,pkt->y);
    // 【新增】只要收到有效包，就清零超时计数器
    uart_rx_timeout = 0;
    
    switch (pkt->mode) {
        // 0x01：接收绝对实时坐标
        case 0x01:
            g_ctrl.current.x = pkt->x;
            g_ctrl.current.y = pkt->y;
            // 只有在非相对追踪模式下，收到0x01包才触发PID更新。
            // 防止在第三题丢目标时，0x01绝对坐标包不断触发旧的相对误差导致电机狂跑。
            if (g_ctrl.state != SYS_TRACKING_REL) {
                g_ctrl.new_data  = true;
            }
            break;

        // 0x02：接收相对实时坐标
        case 0x02:
            //给绿激光一个追踪点,加一点扰动
            g_ctrl.rel_err_x = pkt->x + Generate_Pixel_Perturbation();
            g_ctrl.rel_err_y = pkt->y + Generate_Pixel_Perturbation();
            // 相对追踪模式下，只有收到0x02相对误差包才真正触发PID运算
            if (g_ctrl.state == SYS_TRACKING_REL) {
                g_ctrl.new_data  = true;
            }
            //UART_Printf(UART_0_INST,"%d,%d,%d\n",pkt->mode,pkt->x,pkt->y);
            break;

        // 0x03：接收校准坐标
        case 0x03:
            if (g_ctrl.state == SYS_CALIBRATING) {
                g_ctrl.current.x = pkt->x;
                g_ctrl.current.y = pkt->y;
                PID_SaveCalibrationPoint(g_calib_select_idx);
                g_calib_step_points[g_calib_select_idx].x = g_manual_step_x;
                g_calib_step_points[g_calib_select_idx].y = g_manual_step_y;
            }
            break;

        default:
            break;
    }
}


int main(void) {
    SYSCFG_DL_init();
    Motor_Timer_Init();
    Motor_Init();
    PID_Init();
    
    tft180_init();
    lcd_ui_init();
    TFT180_BL(1);
    tft180_clear();

    //开启uart中断
    UART_Interrupt_Init();
    
    while (1) {

        // --- 示例：使用中断方式处理 UART0 数据 ---
        if (g_uart0_rx_complete) {
            // 处理完毕后重置标志位，准备接收下一个包
            g_uart0_rx_complete = false; 

            //处理步骤...
            // 收到 UART0 包后，转发给 UART1
            //UART_SendPacket(UART_1_INST, g_uart0_packet.mode, g_uart0_packet.x, g_uart0_packet.y);
            //UART_Printf(UART_0_INST, "UART0 Msg Forwarded\r\n");
            Handle_UART_Packet(&g_uart0_packet);
        }

        // --- 示例：使用中断方式处理 UART1 数据 ---
        if (g_uart1_rx_complete) {
            // 处理完毕后重置标志位，准备接收下一个包
            g_uart1_rx_complete = false;
            // 收到 UART1 回传包后，打印解析结果
            UART_Printf(UART_0_INST, "Recv Mode:%d, X:%d, Y:%d\r\n", g_uart1_packet.mode, g_uart1_packet.x, g_uart1_packet.y);
            Handle_UART_Packet(&g_uart1_packet);
            //tft180_show_int(0,16,g_uart1_packet.mode,3);
            //tft180_show_int(0,32,g_uart1_packet.x,3);
            //tft180_show_int(0,48,g_uart1_packet.y,3);
            }
        
        // 校准按键调用
        
        Button_Process(GPIO_MOTOR1_PIN_SW_PORT,  GPIO_MOTOR1_PIN_SW_PIN,  "Cursw:m1_sw1    ", Action_M2_Step_CCW);
        Button_Process(GPIO_MOTOR1_PIN_SW2_PORT, GPIO_MOTOR1_PIN_SW2_PIN, "Cursw:m1_sw2    ", Action_M2_Step_CW);
        Button_Process(GPIO_MOTOR2_PIN2_SW1_PORT,GPIO_MOTOR2_PIN2_SW1_PIN,"Cursw:m2_sw1    ", Action_M1_Step_CW);
        Button_Process(GPIO_MOTOR2_PIN2_SW2_PORT,GPIO_MOTOR2_PIN2_SW2_PIN,"Cursw:m2_sw2    ", Action_M1_Step_CCW);

        //控制区按键调用

        Button_Process(GPIO_CONTROL_PIN_STATE_SWITCH_PORT,GPIO_CONTROL_PIN_STATE_SWITCH_PIN,"Cursw:ctl_state ", PID_Handle_SW1_StateTransform);
        Button_Process(GPIO_CONTROL_PIN_CHANGE_INDEX_PORT,GPIO_CONTROL_PIN_CHANGE_INDEX_PIN,"Cursw:ctl_calib ", PID_Handle_SW2_SwitchCalibIdx);
        //Button_Process(GPIO_CONTROL_PIN_RESET_PORT,GPIO_CONTROL_PIN_RESET_PIN,"ctrl_sw2 pressed", PID_Handle_SW3_ResetPress);
        Button_Process(GPIO_CONTROL_PIN_STOP_PORT,GPIO_CONTROL_PIN_STOP_PIN,"Cursw:ctl_stop  ", PID_Handle_SW4_Stop);

        //ctrl_sw3的单独实现
        static uint8_t reset_btn_pressed = 0; // 记录按键当前状态

        if (DL_GPIO_readPins(GPIO_CONTROL_PIN_RESET_PORT, GPIO_CONTROL_PIN_RESET_PIN) == 0) {
            // 按下瞬间触发
            if (!reset_btn_pressed) {
                delay_cycles(320000); // 简单消抖
                if (DL_GPIO_readPins(GPIO_CONTROL_PIN_RESET_PORT, GPIO_CONTROL_PIN_RESET_PIN) == 0) {
                    reset_btn_pressed = 1;
                    tft180_printf(0, 9*16, "Cursw:res_press ");
                    PID_Handle_SW3_ResetPress();
                }
            }
        } else {
            // 松开瞬间触发
            if (reset_btn_pressed) {
                delay_cycles(320000); // 简单消抖
                if (DL_GPIO_readPins(GPIO_CONTROL_PIN_RESET_PORT, GPIO_CONTROL_PIN_RESET_PIN) != 0) {
                    reset_btn_pressed = 0;
                    tft180_printf(0, 9*16, "Cursw:res_relese");
                    PID_Handle_SW3_ResetRelease();
                }
            }
        }
        // ---- PID 更新：有新坐标则计算并驱动电机 ----
        PID_Update();

            // 【新增】超时检测机制
        if (g_ctrl.state == SYS_TRACKING_REL) {
            uart_rx_timeout++;
            // 这里的 50000 需要根据你 while(1) 的执行速度调整（大概调到 0.2~0.5秒超时即可）
            if (uart_rx_timeout > 50000) { 
                g_ctrl.rel_err_x = 0;
                g_ctrl.rel_err_y = 0;
                
                // 可选：直接清理积分项，防止电机因为之前的积分残留继续缓慢移动
                g_ctrl.pid_x.integral = 0;
                g_ctrl.pid_y.integral = 0;
                
                // 锁定在原位，防止跑飞
                uart_rx_timeout = 50000; // 防止数值溢出
            }
        }

        // // ---- TFT 实时显示当前状态（可选，降低刷新频率避免影响控制） ----
        // // 建议加计时器控制刷新频率，此处简化为每次主循环刷新
        // static uint32_t display_cnt = 0;
        // if (++display_cnt >= 5000) {
        //     display_cnt = 0;

        //     // 1. 显示系统状态
        //     const char *state_str[] = {
        //         [SYS_IDLE]         = "IDLE   ",
        //         [SYS_CALIBRATING]  = "CALIB  ",
        //         [SYS_TRACKING_ABS] = "TRK_ABS",
        //         [SYS_TRACKING_REL] = "TRK_REL",
        //         [SYS_MOVING]       = "MOVING ",
        //         [SYS_ARRIVED]      = "ARRIVED",
        //         [SYS_RESET]        = "RESET  "
        //     };

        //     if (g_ctrl.state >= SYS_IDLE && g_ctrl.state <= SYS_RESET) {
        //         tft180_printf(0, 0, "Mode:%s", state_str[g_ctrl.state]);
        //     } else {
        //         tft180_printf(0, 0, "UNKNOWN");
        //     }
            
        //     //显示校准的index
        //     if(g_ctrl.state == SYS_CALIBRATING)
        //     {
        //         tft180_printf(0,1*16,"Cur idx:%d     ",g_calib_select_idx);
        //     }else{
        //         tft180_printf(0,1*16,"Calib mode off");
        //     }
        //     // 2. 显示当前坐标 (末尾加空格用于清空旧数据位)
        //     tft180_printf(0, 32, "Cur:%3d,%3d    ", 
        //                 g_ctrl.current.x, g_ctrl.current.y);

        //     // 3. 显示目标坐标
        //     tft180_printf(0, 48, "Tgt:%3d,%3d    ", 
        //                 g_ctrl.target.x, g_ctrl.target.y);

        //     // 4. 显示误差 (根据不同模式智能输出)
        //     if (g_ctrl.state == SYS_TRACKING_REL) {
        //         tft180_printf(0, 64, "Err:%3d,%3d    ", 
        //                     (int)g_ctrl.rel_err_x, 
        //                     (int)g_ctrl.rel_err_y);
        //     } else {
        //         // 第一、二题：使用绝对坐标差值计算误差
        //         tft180_printf(0, 64, "Err:%3d,%3d    ", 
        //                     g_ctrl.target.x - g_ctrl.current.x,
        //                     g_ctrl.target.y - g_ctrl.current.y);
        //     }

        //     // 5. 显示当前追踪坐标和下标
        //     tft180_printf(0,80,"index=%d",g_ctrl.target_idx);
        // }
        // ---- TFT 实时显示当前状态（可选，降低刷新频率避免影响控制） ----
        // 建议加计时器控制刷新频率，此处简化为每次主循环刷新
        static uint32_t display_cnt = 0;
        if (++display_cnt >= 5000) {
            display_cnt = 0;

            // ==========================================
            // 1. 始终固定在第一行 (Y=0) 显示系统状态
            // ==========================================
            const char *state_str[] = {
                [SYS_IDLE]         = "IDLE   ",
                [SYS_CALIBRATING]  = "CALIB  ",
                [SYS_TRACKING_ABS] = "TRK_ABS",
                [SYS_TRACKING_REL] = "TRK_REL",
                [SYS_MOVING]       = "MOVING ",
                [SYS_ARRIVED]      = "ARRIVED",
                [SYS_RESET]        = "RESET  "
            };

            if (g_ctrl.state >= SYS_IDLE && g_ctrl.state <= SYS_RESET) {
                tft180_printf(0, 0, "Mode: %s", state_str[g_ctrl.state]);
            } else {
                tft180_printf(0, 0, "Mode: UNKNOWN");
            }

            // ==========================================
            // 2. 根据不同系统状态，分屏显示特定内容
            // 注意：不显示的行统一用空格填满，防止旧字符残留
            // ==========================================
            switch (g_ctrl.state) 
            {
                case SYS_CALIBRATING:
                    // 【校准模式】重点显示：校准点编号、当前电机的开环步数
                    tft180_printf(0, 16, "Calib Idx: %d     ", g_calib_select_idx);
                    tft180_printf(0, 32, "Cur X: %-3d      ", g_ctrl.current.x); //当前坐标
                    tft180_printf(0, 48, "Cur Y: %-3d      ", g_ctrl.current.y);
                    tft180_printf(0, 64, "StepX: %-3d      ", g_manual_step_x); //手动记步
                    tft180_printf(0, 80, "StepY: %-3d      ", g_manual_step_y); 
                    tft180_printf(0, 6*16, "tgtpt:(% 3d,% 3d)",g_target_list[g_calib_select_idx].x,g_target_list[g_calib_select_idx].y); //目标坐标
                    tft180_printf(0, 7*16, "tgtst:(% 3d,% 3d)",g_calib_step_points[g_calib_select_idx].x,g_calib_step_points[g_calib_select_idx].y); 
                    tft180_printf(0, 8*16, "                  "); 
                    
                    break;

                case SYS_TRACKING_ABS:
                case SYS_MOVING:
                case SYS_ARRIVED:
                    // 【绝对走点模式】(第一题) 重点显示：当前PID坐标、目标、误差、目标下标
                    tft180_printf(0, 16, "Cur: %4d,%4d   ", g_ctrl.current.x, g_ctrl.current.y);
                    tft180_printf(0, 32, "Tgt: %4d,%4d   ", g_ctrl.target.x, g_ctrl.target.y);
                    tft180_printf(0, 48, "Err: %4d,%4d   ", g_ctrl.target.x - g_ctrl.current.x, 
                                                              g_ctrl.target.y - g_ctrl.current.y);
                    tft180_printf(0, 64, "Tgt Idx: %d      ", g_ctrl.target_idx);
                    tft180_printf(0, 80, "                  "); // 清空无用行
                    tft180_printf(0, 6*16, "                  "); 
                    tft180_printf(0, 7*16, "                  "); 
                    tft180_printf(0, 8*16, "                  "); 
                    break;

                case SYS_TRACKING_REL:
                    // 【相对追踪模式】(第二题) 重点显示：摄像头传来的相对误差
                    tft180_printf(0, 16, "Cur: %4d,%4d   ", g_ctrl.current.x, g_ctrl.current.y);
                    tft180_printf(0, 32, "Rel Err:%4d,%4d", (int)g_ctrl.rel_err_x, (int)g_ctrl.rel_err_y);
                    tft180_printf(0, 48, "                  "); 
                    tft180_printf(0, 64, "                  "); 
                    tft180_printf(0, 80, "                  "); 
                    tft180_printf(0, 6*16, "                  "); 
                    tft180_printf(0, 7*16, "                  "); 
                    tft180_printf(0, 8*16, "                  "); 
                    break;

                case SYS_IDLE:
                case SYS_RESET:
                default:
                    // 【待机/复位模式】简单显示当前坐标即可，其余清空
                    tft180_printf(0, 16, "Cur: %4d,%4d   ", g_ctrl.current.x, g_ctrl.current.y);
                    tft180_printf(0, 32, "Waiting...        ");
                    tft180_printf(0, 48, "                  ");
                    tft180_printf(0, 64, "                  ");
                    tft180_printf(0, 80, "                  ");
                    tft180_printf(0, 6*16, "                  "); 
                    tft180_printf(0, 7*16, "                  "); 
                    tft180_printf(0, 8*16, "                  "); 
                    break;
            }
        }
    }
}