import sensor
import time
from pyb import UART, Pin, Timer
import struct
import display, image, math
# 1. 硬件初始化
uart = UART(3, 115200, timeout_char=2)  # 通常是 P4(TX), P5(RX)
# 2. 初始化按键 (P9 引脚，上拉模式，按下时引脚为低电平)
key1 = Pin('P9', Pin.IN, Pin.PULL_UP)
key2 = Pin('P8', Pin.IN, Pin.PULL_UP)
# 3. 全局标志位
send_flag = False
is_tracking1_enabled = False  # 用来记录按键状态（按下切换发送/停止）
is_tracking2_enabled = False  # 用来记录按键状态（按下切换发送/停止）


# --- 定时器中断回调函数 ---
def tick(timer):
    global send_flag
    # 仅仅改变标志位，不在这里写 uart.write
    send_flag = True


# 4. 配置定时器 (使用 Timer 4, 频率 10Hz = 每秒发送10次坐标)
tim = Timer(4, freq=10)
tim.callback(tick)
# 5. 传感器配置
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.VGA)
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
sensor.set_auto_exposure(False, exposure_us=17000)  # 曝光时间控制
lcd = display.SPIDisplay(vflip=True, hmirror=True)
clock = time.clock()
# 6. 参数控制
last_key1_state = 1
last_key2_state = 1  # 按键初始状态
MAX_RED_SIZE = 800  # 激光点最大面积限制 (w*h)
# 红色激光点阈值 (通常亮度L非常高，A值也很大)
red_threshold = (60, 100, 16, 45, -23, 22)
redB_threshold = (100, 0, 20, 127, -39, 42)
# 绿色激光点阈值 (a 通道（绿-红）具有极低的负值)
green_threshold = (60, 100, -120, -30, -10, 60)
HISTORY_LEN = 2       # 保存的红点历史坐标数量（数值越大，跟得越远，需根据FPS微调）
SAFE_DISTANCE = 5    # 红绿激光最小安全距离（像素），小于此距离绿光刹车防止重叠
red_history = []      # 用于存放红激光历史坐标的列表


def send_data_packet(x, y, mode=0x01):
    """打包并发送串口数据 (Mode 0x01:坐标, 0x02:误差)"""
    data = struct.pack("<BBhhB", 0xAA, mode, int(x), int(y), 0xBB)
    uart.write(data)


def find_red_blod():
    red_blobs = img.find_blobs([red_threshold, redB_threshold], pixels_threshold=3, area_threshold=3)
    red_pos = None
    for blob in red_blobs:
        # 几何形状过滤：确保它是“又小又方”的激光点
        if (blob[2]*blob[3] < MAX_RED_SIZE and (blob[2] > 2) and (blob[3] > 2) and (blob[2] < 2 * blob[3]) and (blob[3] < 2 * blob[2])):
            red_pos = (blob.cx(), blob.cy())
            # 【核心修改】：用蓝色十字标定激光中心点
            img.draw_cross(red_pos[0], red_pos[1], color=(0, 0, 255), size=10, thickness=2)
            return red_pos
    return None


def find_green_blob():
    # 使用绿色阈值查找色块
    green_blobs = img.find_blobs([green_threshold], pixels_threshold=5, area_threshold=5)
    green_pos = None

    for blob in green_blobs:
        # 几何形状过滤：确保它是“又小又方”的激光点
        # blob[2]是宽度，blob[3]是高度
        if (blob[2]*blob[3] < MAX_RED_SIZE and (blob[2] > 2) and (blob[3] > 2) and (blob[2] < 2 * blob[3]) and (blob[3] < 2 * blob[2])):
            green_pos = (blob.cx(), blob.cy())
            # 【可视化】：用绿色十字标定绿色激光中心点
            img.draw_cross(green_pos[0], green_pos[1], color=(0, 0, 255), size=10, thickness=2)
            # 找到第一个符合条件的就返回，避免干扰
            return green_pos
    return None


while (True):
    clock.tick()
    img = sensor.snapshot()  # 获取当前图像

    red_pos = find_red_blod()      # 寻找红色激光点
    green_pos = find_green_blob()  # 寻找绿色激光点

    if red_pos:
        red_history.append(red_pos)
        # 维持队列长度
        if len(red_history) > HISTORY_LEN:
            red_history.pop(0)

    if key1.value() == 0 and last_key1_state == 1:
        is_tracking1_enabled = not is_tracking1_enabled
        print("Transmission Blob:", "ON")
        time.sleep_ms(200)  # 简单消抖
    last_key1_state = key1.value()

    if key2.value() == 0 and last_key2_state == 1:
        is_tracking2_enabled = not is_tracking2_enabled
        print("Transmission err:", "ON" if is_tracking2_enabled else "OFF")
        time.sleep_ms(200)  # 简单消抖
    last_key2_state = key2.value()
    lcd.write(img, hint=image.CENTER | image.SCALE_ASPECT_KEEP)
    if send_flag:
        send_flag = False  # 重置定时器标志
        if is_tracking1_enabled and green_pos and len(red_history) > 0:

            # 安全地获取目标红点：取历史队列中最老的一个
            target_red = red_history[0]

            # 再次确认 target_red 不是 None (防止队列里混入了无效帧)
            if target_red is not None:
                # 计算当前真实距离（用于安全死区判断）
                current_dist = math.sqrt((red_pos[0]-green_pos[0])**2 + (red_pos[1]-green_pos[1])**2) if red_pos else 999

                if current_dist < SAFE_DISTANCE:
                    dx, dy = 0, 0
                else:
                    dx = target_red[0] - green_pos[0]
                    dy = target_red[1] - green_pos[1]

                send_data_packet(dx, dy, 0x02)

        elif red_pos:
            # 如果只看到红光，发绝对坐标让绿光快追
            send_data_packet(red_pos[0], red_pos[1], 0x01)
    #  输出当前运行帧率
    # print("FPS: %.2f" % clock.fps())
