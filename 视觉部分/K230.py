import time, os, gc, image, math, struct
from media.sensor import Sensor, CAM_CHN_ID_0
from media.display import Display
from media.media import MediaManager
from ybUtils.YbUart import YbUart
from machine import FPIOA, Pin

DISPLAY_WIDTH=640           #LCD显示宽度/LCDdisplaywidth
DISPLAY_HEIGHT=480          #LCD显示高度/LCDdisplayheight
MAX_RED_SIZE = 1000
ARRIVAL_THRESHOLD = 5        # 像素阈值：激光点距离目标点多近算“到达”
flag_cont_send = False       # 连续发送开关
flag_tracking = False        # 误差追踪开关
flag_rect_detecting = False  # 是否正在识别矩形
saved_outer = None           # 锁定的外框顶点
saved_inner = None           # 锁定的内框顶点
target_path = []             # 存放射周长一圈的插值目标点
current_target_idx = 0       # 当前正在追踪的航点索引

# 激光阈值
red_threshold = (60, 100, 16, 45, -23, 22)
redB_threshold = (100, 0, 20, 127, -39, 42)

# ==================== 按键初始化定义 ====================
class YbKey:
    def __init__(self, pin_num):
        self._fpioa = FPIOA()
        self._pin_num = pin_num
        # 设置 FPIOA 映射
        self._fpioa.set_function(self._pin_num, FPIOA.GPIO0 + self._pin_num, ie=1, oe=0)
        # 初始化 Pin 为上拉输入
        self._key = Pin(self._pin_num, Pin.IN, pull=Pin.PULL_UP, drive=7)

    def is_pressed(self):
        return self._key.value() == 0
# ==================== 激光红点识别 ====================
def find_red_blob(img):
    global last_red_pos
    red_blobs = img.find_blobs([red_threshold, redB_threshold], pixels_threshold=3, area_threshold=3)
    for blob in red_blobs:
        w, h = blob.w(), blob.h()
        if (w*h < MAX_RED_SIZE and w > 2 and h > 2 and w < 2*h and h < 2*w):
            current_pos = (blob.cx(), blob.cy())
            img.draw_cross(current_pos[0], current_pos[1], color=(0, 0, 255), size=10, thickness=2)
            return current_pos
    last_red_pos = None
    return None
# ==================== 矩形识别 ====================
# 模組 1: 幾何計算工具類
class GeometryUtils:
    @staticmethod
    def sort_corners(corners):
        # 强制将所有传入的角点解包并转换为整形 List，彻底断绝 tuple 报错
        if not corners or len(corners) != 4:
            return corners

        pts = [[int(p[0]), int(p[1])] for p in corners]

        # 计算几何中心 (使用 float 避免精度丢失)
        cx = sum(pt[0] for pt in pts) / 4.0
        cy = sum(pt[1] for pt in pts) / 4.0

        # 极角排序
        pts.sort(key=lambda pt: math.atan2(pt[1] - cy, pt[0] - cx))

        # 锁定左上角 (x+y 最小)
        tl_idx = min(range(4), key=lambda i: pts[i][0] + pts[i][1])
        return pts[tl_idx:] + pts[:tl_idx]

    @staticmethod
    def get_shrinked_rect(corners, ratio_x, ratio_y):
        """計算內側縮放矩形座標"""
        pts = GeometryUtils.sort_corners([list(p) for p in corners])
        cx = sum(p[0] for p in pts) / 4.0
        cy = sum(p[1] for p in pts) / 4.0
        return [[int(cx + (p[0]-cx)*ratio_x), int(cy + (p[1]-cy)*ratio_y)] for p in pts]
# 模組 2: 座標穩定器（防止抖動）
class RectStabilizer:
    def __init__(self, buffer_size=5):
        self.buffer = []
        self.buffer_size = buffer_size

    def update(self, corners):
        """輸入當前幀角點，返回平滑後的平均角點"""
        self.buffer.append(corners)
        if len(self.buffer) > self.buffer_size:
            self.buffer.pop(0)

        avg_corners = []
        for i in range(4):
            x = sum(f[i][0] for f in self.buffer) / len(self.buffer)
            y = sum(f[i][1] for f in self.buffer) / len(self.buffer)
            avg_corners.append([int(x), int(y)])
        return avg_corners
# 模組 3: 矩形檢測核心類
class RectangleDetector:
    def __init__(self, threshold=20000, min_area=40000):
        self.threshold = threshold
        self.min_area = min_area
        self.stabilizer = RectStabilizer(buffer_size=3) # 緩衝3幀數據

    def process_frame(self, img):
        """處理單幀圖像，返回 (外框角點, 内框角點)"""
        img.gaussian(1) # 預處理平滑噪點
        rects = img.find_rects(threshold=self.threshold)

        best_rect = None
        max_mag = 0

        for r in rects:
            mag = r.magnitude()
            if mag > self.min_area and mag > max_mag:
                max_mag = mag
                best_rect = r

        if best_rect:
            # 排序 -> 平滑 -> 缩放计算
            raw_corners = GeometryUtils.sort_corners(best_rect.corners())
            stable_outer = self.stabilizer.update(raw_corners)
            stable_inner = GeometryUtils.get_shrinked_rect(stable_outer, 0.93, 0.93)
            return stable_outer, stable_inner

        return None, None

    def draw_result(self, img, outer, inner):
        if not outer: return

        # 绘制外框（红色）
        for i in range(4):
            # 取出坐标，严格转为整数
            x1, y1 = int(outer[i][0]), int(outer[i][1])
            x2, y2 = int(outer[(i+1)%4][0]), int(outer[(i+1)%4][1])

            img.draw_line(x1, y1, x2, y2, color=(255, 0, 0), thickness=3)

            # 在角点附近标出排序序号：绿色字，Scale=2放大显示
            # 将文字偏移10个像素，防止被边框挡住
            img.draw_string(x1 + 10, y1 + 10, str(i), color=(0, 255, 0), scale=2)
            # 加个小圆点标注真正角点的位置
            img.draw_circle(x1, y1, 4, color=(255, 255, 0), fill=True)

            # 绘制内框（青色）
            if inner:
                for i in range(4):
                    x1, y1 = int(inner[i][0]), int(inner[i][1])
                    x2, y2 = int(inner[(i+1)%4][0]), int(inner[(i+1)%4][1])
                    img.draw_line(x1, y1, x2, y2, color=(0, 255, 255), thickness=2)

# ==================== 串口发送 ====================
def send_packet(x, y, mode=0x01):
    """
    打包并发送串口数据 (7字节)
    格式: 帧头(0xAA) + 模式 + X(short) + Y(short) + 帧尾(0xBB)
    """
    try:
        # <BBhhB 表示：小端字节序, 两个1字节无符号, 两个2字节有符号短整型, 一个1字节无符号
        data = struct.pack("<BBhhB", 0xAA, mode, int(x), int(y), 0xBB)
        uart.send(data)
    except Exception as e:
        print(f"UART Send Error: {e}")

# ==================== 矩形路径插点 ====================
def generate_path(corners, steps_long, steps_short): # <--- 修改：传入两个步数参数
    """
    在矩形的四个顶点之间插值，自动识别长短边并分配点数
    """
    path = []

    # 1. 先计算出四条边的长度，存入列表
    edge_lengths = []
    for i in range(4):
        p1 = corners[i]
        p2 = corners[(i + 1) % 4]
        # 使用欧几里得距离公式: $d = \sqrt{(x_2 - x_1)^2 + (y_2 - y_1)^2}$
        dist = math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)
        edge_lengths.append(dist)

    # 2. 找到长边和短边的基准值（用于比较）
    max_len = max(edge_lengths)
    min_len = min(edge_lengths)

    # 3. 开始插值
    for i in range(4):
        p_start = corners[i]
        p_end = corners[(i + 1) % 4]
        current_len = edge_lengths[i]

        # --- 核心逻辑：判断当前边是长是短 ---
        # 如果当前边长度更接近最大长度，则判定为长边
        if (max_len - current_len) < (current_len - min_len):
            current_steps = steps_long
        else:
            current_steps = steps_short

        # 线性插值生成点
        for step in range(current_steps):
            ratio = step / current_steps
            x = int(p_start[0] + (p_end[0] - p_start[0]) * ratio)
            y = int(p_start[1] + (p_end[1] - p_start[1]) * ratio)
            path.append((x, y))

    return path
# ==================== 主程序逻辑 ====================

try:
   # 硬件初始化
    sensor = Sensor(width=DISPLAY_WIDTH,height=DISPLAY_HEIGHT)
    sensor.reset()
    sensor.set_framesize(width=DISPLAY_WIDTH,height=DISPLAY_HEIGHT)
    sensor.set_pixformat(Sensor.RGB565)
    Display.init(Display.ST7701,to_ide = True)
    MediaManager.init()
    sensor.run()
    uart = YbUart(baudrate=115200)

    # 初始化三个按键：Pin 42, 33, 43
    key1 = YbKey(42) # 功能：单次发送激光坐标
    key2 = YbKey(33) # 功能：连续发送激光坐标
    key3 = YbKey(43) # 功能：矩形识别 + 误差追踪

    # 實例化檢測器
    detector = RectangleDetector(threshold=18000, min_area=35000)
    clock = time.clock()

    while True:
        clock.tick()
        os.exitpoint()
        img = sensor.snapshot()

        # --- 第一部分：全局激光红点识别 (始终运行) ---
        red_pos = find_red_blob(img)

        # --- 第二部分：按键逻辑检测 ---
        # 按键 1: 单次发送 (Mode 0x01)
        if key1.is_pressed():
            time.sleep(0.05) # 消抖
            flag_rect_detecting = not flag_rect_detecting
            flag_tracking = False
            flag_cont_send = False
            if flag_rect_detecting:
                print(">>> 开始识别矩形...")
                # 开始识别时，先清空旧的路径和数据，防止误操作
                target_path = []
            else:
                print(">>> 停止识别矩形！")
                # 停止识别时，如果刚刚有成功获取到轮廓，则生成并保存路径
                if saved_inner:
                    target_path = generate_path(saved_inner, 9, 6)
                    current_target_idx = 0
                    print("已锁定矩形顶点，并成功生成循迹路径！")
                else:
                    print("未锁定任何有效矩形！")
            if red_pos:
                send_packet(red_pos[0], red_pos[1], mode=0x03)
                print(f"单次发送绝对坐标: {red_pos}")
            while key1.is_pressed(): pass # 等待松开

        # 按键 2: 连续发送开关 (Mode 0x02)
        if key2.is_pressed():
            time.sleep(0.05)
            flag_cont_send = not flag_cont_send
            flag_tracking = False # 模式互斥
            flag_rect_detecting = False
            print(f"连续发送模式: {flag_cont_send}")
            while key2.is_pressed(): pass

        # 按键 3: 矩形识别并启动追踪 (Mode 0x03)
        if key3.is_pressed():
            time.sleep(0.05)
            flag_rect_detecting = False
            flag_cont_send = False
            if not target_path:
                print("错误:还没有识别并锁定路径!请先使用按键1完成矩形识别.")
            else:
                flag_tracking = not flag_tracking
                print(f">>> 误差追踪模式: {flag_tracking}")
            while key3.is_pressed(): pass

        # --- 第三部分：模式执行逻辑 ---
        # A. 矩形识别模式逻辑
        if flag_rect_detecting:
            outer, inner = detector.process_frame(img)
            # 只要识别到了，就实时更新暂存的数据
            if outer and inner:
                saved_outer = outer
                saved_inner = inner

            # 在屏幕上实时画出正在识别的轮廓
            if saved_outer:
                for i in range(4):
                    p1, p2 = saved_outer[i], saved_outer[(i+1)%4]
                    img.draw_line(p1[0], p1[1], p2[0], p2[1], color=(255, 0, 0), thickness=2)
                    img.draw_string_advanced(p1[0] + 10, p1[1] + 10, 25, str(i), color=(0, 255, 0))
            if saved_inner:
                for i in range(4):
                    p1, p2 = saved_inner[i], saved_inner[(i+1)%4]
                    img.draw_line(p1[0], p1[1], p2[0], p2[1], color=(0, 255, 0), thickness=2)

        # B. 连续发送激光点绝对坐标
        if flag_cont_send and red_pos:
            send_packet(red_pos[0], red_pos[1], mode=0x01)
            print(f"连续发送绝对坐标: {red_pos}")

        # C. 误差追踪模式逻辑
        if flag_tracking and target_path:
            # --- 显示部分 ---
            # 画出锁定的矩形轮廓（绿色外框）
            if saved_inner:
                for i in range(4):
                    p1, p2 = saved_inner[i], saved_inner[(i+1)%4]
                    img.draw_line(p1[0], p1[1], p2[0], p2[1], color=(0, 255, 0), thickness=1)

            # 画出所有的插入点（用青色小圆点表示）
            for pt in target_path:
                img.draw_circle(pt[0], pt[1], 2, color=(0, 255, 255), fill=True)

            # --- 追踪控制部分 ---
            if red_pos:
                # 取出当前真正要追踪的目标点
                target_pt = target_path[current_target_idx]

                # 视觉辅助：画出当前的目标点（黄色大圆点），方便你看出它追踪到哪了
                img.draw_circle(target_pt[0], target_pt[1], 6, color=(255, 255, 0), fill=True)

                # 计算欧氏距离与误差 (使用 target_pt 的 x 和 y)
                dx = target_pt[0] - red_pos[0]
                dy = target_pt[1] - red_pos[1]
                distance = math.sqrt(dx**2 + dy**2)

                # 判读是否“到达”当前点
                if distance < ARRIVAL_THRESHOLD:
                # 切换到下一个点
                    current_target_idx = (current_target_idx + 1) % len(target_path)
                else:
                # 未到达，发送当前误差给单片机
                    send_packet(dx, dy, mode=0x02)

        # ================= UI 显示状态 =================
        mode_text = ""
        if flag_rect_detecting: mode_text += "[DETECTING] "
        if flag_tracking: mode_text += "[TRACKING] "
        if flag_cont_send: mode_text += "[CONT SEND] "
        if not mode_text: mode_text = "[IDLE]"

        img.draw_string_advanced(10, 10, 20, f"FPS: {clock.fps():.1f} {mode_text}", color=(0, 255, 0))
        Display.show_image(img)
        gc.collect()
        time.sleep_ms(50)

except KeyboardInterrupt as e:
    print("User Stop: ", e)
except BaseException as e:
    print(f"Exception: {e}")
finally:
    # 清理资源 Cleanup resources
    if isinstance(image, Sensor):
        sensor.stop()
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()
