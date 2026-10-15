# 车载娱乐系统（Smart_car）项目说明

本文档描述**当前仓库真实实现**，供阅读代码、继续开发和 AI 助手（如 Claude）作上下文。请以本文件与源码为准；根目录 `README.md` 可能含过时内容或 git 冲突标记。

---

## 1. 项目是什么

基于 **Qt5 Widgets** 的嵌入式 Linux **车载信息娱乐终端（IVI）**。

- 目标板：**100ASK IMX6ULL Pro**（ARM Cortex-A7）
- 屏幕：**1024 × 600** 触摸屏，平台插件一般为 `linuxfb` + tslib
- 语言：C++11
- 工程文件：`Smart_in-car_entertainment.pro`
- 可执行文件名：`Smart_in-car_entertainment`

定位：学生/求职作品。把多媒体、网络、串口 GPS、字符设备传感器、USB V4L2 摄像头做到同一套 HMI 里。

---

## 2. 功能一览（以主页按钮为准）

主页为浅色仪表盘风格（`mainwindow.ui`），不是早期街景大图标布局。

| 主页入口 | 界面对象名 | 打开的模块 | 实现文件 |
|----------|------------|------------|----------|
| 音乐播放 | `pushButton_2` | `Audio` | `Audio.*` |
| 视频播放 | `pushButton` | `Player` | `player.*`、`playercontrols.*`、`videowidget.*`、`playlistmodel.*` |
| 天气预报 | `pushButton_3` | `Weather` | `weather.*`、`day.*`、`citycodeutils.*` |
| 地图导航 | `pushButton_5` | `Map` | `map.*` |
| 环境监测 | `pushButton_4` | `Hardware` | `hardware.*`、`dht11.*`、`sr04.*`、`mq135.*`、`led.*`、`buzzer.*` |
| 系统设置 | `btnSettings` | `SettingsPage` | `settingspage.*`（音量/亮度演示占位） |
| 倒车影像 | `btnCamera` | `Camera` | `camera.*`、`v4l2capture.*` |

主页额外展示（演示数据，未接整车 CAN）：

- 系统状态、当前速度 85 km/h、剩余电量 65%、总里程
- 中间车速表：`SpeedGauge`（`speedgauge.*`）
- 底栏日期时间

子页面（天气/音乐/视频/地图/环境）**尚未**改成仪表盘浅色风格，仍是原 UI。

---

## 3. 架构

```
main.cpp
  └── MainWindow（1024×600 无边框主页）
        ├── 点击入口 → show 对应 QWidget / QDialog，hide 主页
        ├── 子页 emit back / back1 / backout → 再 show 主页
        ├── Weather / Audio 曾 moveToThread（含界面的 QWidget 移线程并不规范；Player 已于 2026-10-12 移除线程化）
        ├── Camera、SettingsPage 懒加载（第一次点击才 new）
        └── 倒车影像采集在 V4L2Capture(QThread)，禁止在 UI 线程里 ioctl/DQBUF
```

导航约定：

- 进入：`MainWindow::openModule()` 或 `Camera::camera_working()`
- 返回：各模块 `back` 类信号 → `MainWindow::backToHome()`

---

## 4. 各模块说明

### 4.1 天气 `Weather`

- 易客天气 HTTP JSON，解析 7 日数据，绘制高低温曲线
- 入口槽：`weather_working()`
- 返回信号：`back()`
- 需开发板能上网、DNS 可用

### 4.2 音乐 `Audio`

- 扫描可执行文件目录下 `myMusic/*.mp3`，同名 `.lrc` 作歌词
- Qt Multimedia 播放，唱臂/唱片旋转动画（`rotatablewidget.*`）
- 入口：`audio_working()`；返回：`back()`

### 4.3 视频 `Player`

- 本地视频，默认目录 `Videos/`（`.mp4` / `.mkv` / `.avi`）
- 播放列表写入 SQLite：`media_player.db` 表 `video`（只存**不含扩展名的文件名**，查找时按 `.mp4`→`.mkv`→`.avi` 顺序拼路径）
- `Open` 按钮用后缀名过滤（`setNameFilters`），**不要用 `m_player->supportedMimeTypes()` 做过滤器**：SDK 的 ffmpeg 插件上报的 MIME 表不全，会把 mp4 隐藏掉导致"点了没反应"
- 全屏为**布局级自管**（`toggleFullScreen`/`setFullScreenUi`）：隐藏播放列表、Open/Back、控制面板、进度条行，并把 `layout()->setContentsMargins(0,0,0,0)`+`setSpacing(0)`（进入前把原边距存进 `m_savedMargins`，退出时恢复），使视频铺满整 1024×600。**不用 `QVideoWidget::setFullScreen`**（顶层窗口在 linuxfb 上退不回来）
- 全屏退出入口三条：① **单击视频** → 右上角浮出子控件按钮 `m_exitFullButton`（父控件是 `m_videoWidget`，`raise()` 叠在画面上，3s `QTimer::singleShot` 自动隐藏）；② 双击视频；③ ESC（`Player::keyPressEvent`）。非全屏时单击被 `m_isFullScreen` 判断挡掉，无副作用
- 单击/双击信号来自 `videowidget.*`：`VideoWidget::clicked()`、`VideoWidget::doubleClicked()`，全部由 `Player` 决定行为；`mouseDoubleClickEvent` **不得**再调 `setFullScreen`
- 进度条/音量均**松手才提交**（`sliderReleased`），避免软解板上连续 seek 卡顿；副作用：全屏时进度条隐藏，要拖进度得先退出全屏
- 无 `player_working()`，主页点击后直接 `show`（Player 不再 moveToThread）
- 返回：`back()`；`backout()` 会先退出全屏再停播归零
- **性能上限**（IMX6ULL 单核 A7、无 VPU，ffmpeg 纯软解）：板端只能流畅播 ≤640×360、H.264 **baseline**、≤30fps 的片子。1080p 素材在板上是"PPT 级"卡顿，属硬件上限，不是代码 bug。转码模板见 §7.1

### 4.4 环境监控 `Hardware`

- DHT11：`/dev/mydht11`，内核驱动 `dht11_drv.c`，DATA 接 GPIO4_IO19（Linux GPIO 115）
- 可选（代码在，硬件可能未接，open 失败可忽略）：
  - SR04 `/dev/mysr04`
  - MQ135 `/sys/bus/iio/devices/iio:device0/in_voltage3_raw`
  - LED `/dev/myled`
  - 蜂鸣器 `/dev/mybuzzer`
- 入口：`hardware_working()`；返回：`back1()`

### 4.5 GPS 地图 `Map`

- 串口 **`/dev/ttymxc5`（UART6）**，9600 8N1，解析 NMEA `$GPGGA`
- 模块：ATGM336H，TX→板 RX，RX→板 TX
- 联网拉**天地图**静态地图（Key `tk` 写在 `map.h`；百度 AK 已停用故废弃）
- 天地图为 CGCS2000（≈WGS84），与 NMEA 解析输出同源，**无需 GCJ-02/BD-09 转换**
- 室内无定位时用默认坐标 **上海市浦东新区公元新村**（WGS84 `121.5722919,31.1241443`，天地图直接用）
- 静态图 URL：`center=<视图中心>`、`markers=<定位点>`、`zoom=<级别>`；定位针由天地图服务端绘制，平移后针留在定位点而非屏幕中心
- 请求带 10s 超时与错误处理；板端 Qt 若无 OpenSSL（`QSslSocket::supportsSsl()` 为假）自动降级 `http://`
- 成功后把图片缓存到 `应用程序目录/map_cache.png`；再次失败时显示上次地图并在底栏给出原因，不会卡在 loading
- 定位成功后按 5s 节流刷新地图，避免 GGA 每秒一条打满弱 CPU
- 交互：触摸屏拖动平移（松手按 Web Mercator 像素→经纬度换算重新请求）、右上 `+`/`-` 缩放（zoom 3–18）、`回定位` 恢复跟随 GPS/默认点；用户拖动后 GPS 不再拽走视图
- 入口：`map_working()`；返回：`backout()`（返回时停请求并关闭串口）
- `Map` 是 `QDialog`

### 4.6 倒车影像 `Camera` + `V4L2Capture`

这是独立模块，不要和视频播放器 `Player` 混用。

**界面 `camera.cpp`**

- 预览 `QLabel`，按钮：开始 / 停止 / 拍照 / 返回
- 拍照保存：`应用程序目录/Photos/cam_时间.jpg`
- `camera_working()`：进入页面并自动开始采集
- 返回前 `STREAMOFF`、关设备

**采集 `v4l2capture.cpp`（Linux）**

- 扫描 `/dev/video0`～`video7`，选带 `V4L2_CAP_VIDEO_CAPTURE` 的节点
- 优先 MJPEG 640×480，其次 YUYV；mmap + poll + DQBUF
- 目标约 15fps；UI 忙则丢帧（`m_frameBusy`）
- 非 Linux 编译为桩，提示不支持 V4L2
- 面向 **USB UVC 免驱摄像头**，不写 USB 驱动，不改 CSI 设备树

录像（H.264/AVI）**尚未实现**，目前是预览 + 拍照。

### 4.7 系统设置 `SettingsPage`

- 媒体音量、屏幕亮度滑条为演示，不写板端配置
- 返回：`back()`

---

## 5. 关键文件地图

| 路径 | 角色 |
|------|------|
| `Smart_in-car_entertainment.pro` | qmake 工程，QT 模块：widgets network xml multimedia multimediawidgets sql serialport |
| `main.cpp` / `mainwindow.*` | 程序入口与主页 |
| `speedgauge.*` | 主页车速表绘制 |
| `camera.*` / `v4l2capture.*` | 倒车影像 |
| `settingspage.*` | 设置占位页 |
| `weather.*` `Audio.*` `player.*` `hardware.*` `map.*` | 五大业务子页 |
| `dht11_drv.c` | DHT11 内核驱动（单独 `insmod`，不进 Qt 链接） |
| `res.qrc` / `images/` | 图标与背景 |
| `style/*.qss` | 部分旧样式 |
| `myMusic/` | 示例音乐与歌词 |

主页按钮图标仍用旧资源：`Music.png`、`Video1.png`、`Weather.png`、`gps.png`、`environment.png`；设置暂用 `sunrise.png`，倒车影像暂用 `car.png`。

---

## 6. 硬件与设备节点

| 外设 | 节点 / 接口 | 备注 |
|------|-------------|------|
| 触摸屏 | `/dev/input/eventX` + tslib | 需 `ts_calibrate` |
| DHT11 | `/dev/mydht11` | 自定义驱动 |
| GPS | `/dev/ttymxc5` | UART6，9600 |
| USB 摄像头 | `/dev/videoX` | UVC，内核 `uvcvideo` |
| SR04 / LED / 蜂鸣器 / MQ135 | 见 4.4 | 可选 |

---

## 7. 交叉编译与运行

必须在 **Linux 虚拟机（百问网 Ubuntu）** 用 SDK 的 qmake，不要用系统 Qt4 的 `qmake`。

```bash
cd ~/Desktop/Smart_car
mkdir -p build && cd build
/home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/bin/qmake ../Smart_in-car_entertainment.pro
make -j4
```

板端示例：

```bash
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export QT_QPA_FONTDIR=/usr/lib/fonts/
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
# TSLIB_* 按板子实际 event 号配置
cd /root
./Smart_in-car_entertainment
```

部署建议同目录：可执行文件、`myMusic/`、`Videos/`，首次运行生成 `media_player.db`；拍照生成 `Photos/`。

### 7.1 视频性能调优（板端实测结论，2026-10-12）

> 原理：视频文件是压缩数据，**每次播放都在板端现场逐帧软解码**，转码不等于预解码。IMX6ULL 无 VPU，
> 解码成本只能靠素材规格压低：分辨率小、帧率低、baseline（无 B 帧）、或改用帧内编码的 MJPEG。

1. **锁频**（板端执行；原理：往 cpufreq 的 `scaling_governor` 虚拟文件写入策略名，`performance` = CPU 恒定 792MHz 满频，默认策略降频后升频不及时会加剧软解卡顿。**重启失效**）：
   ```sh
   echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
   ```
   持久化（一次执行，写入 `/etc/profile`，串口/SSH 登录时自动生效；若程序改为开机自启则应放 `/etc/init.d/` 启动脚本）：
   ```sh
   echo 'echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' >> /etc/profile
   ```
   验证：
   ```sh
   cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor   # performance
   cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq   # 792000 (kHz)
   ```
   瓶颈定位：播放时 `top -b -n 1 | head -15` 看进程 `%CPU`（2026-10-12 板端实测：原 1080p 素材满载变 PPT，低配素材流畅）。
2. **在 PC/虚拟机转码**（板子跑不动转码），放 `Videos/` 替换。基线模板：
   ```sh
   ffmpeg -i 输入.mp4 -vf scale=640:360 -r 20 -c:v libx264 -profile:v baseline -level 3.0 \
     -preset veryfast -crf 28 -g 20 -c:a aac -b:a 64k 输出.mp4
   ```
   若觉得 20fps 观感顿挫，用降分辨率换帧率：`scale=512:288 -r 30 -g 30 -crf 30`。
3. **MJPEG 备选**（帧内编码、无帧间预测，单帧解码最便宜；缺点是体积大，短演示片段适用）：
   ```sh
   ffmpeg -i 输入.mp4 -vf scale=512:288 -r 20 -c:v mjpeg -q:v 8 \
     -c:a pcm_s16le -ar 22050 -ac 1 输出.avi
   ```
   `-q:v` 质量 2–31 越小越清晰；输出 `.avi` 恰好被 `findVideoPath` 支持。优先级：先试模板 2，仍不流畅再换 3。
4. **替换文件名规则**：数据库按主文件名找文件，覆盖同名 `Videos/xxx.mp4` 即生效，无需动 `media_player.db`。
5. **判断瓶颈**：播放时板端 `top`——CPU 满载 = 素材规格超上限（继续降规格）；CPU 不忙仍卡 = 渲染/代码问题再查。

---

## 8. 给后续改代码的约束

1. 分辨率固定 **1024×600**，主页用 layout，不要再堆绝对坐标大图。
2. 倒车影像只改 `camera.*` / `v4l2capture.*`；本地视频播放只改 `player.*`。
3. 采集必须在 `V4L2Capture` 线程；离开倒车页必须停流、close。
4. 子页面风格尚未统一，第一轮不要大改 `weather.ui` / `Audio.ui` 等，除非需求明确要改。
5. IMX6ULL CPU 弱：预览建议 640×480；若加录像，优先 MJPEG 封装，不要软件 H.264。本地视频同样受软解上限约束（见 §4.3 性能上限与 §7.1）。
6. 含界面的类不要 `moveToThread`（Player 已修复；历史代码里 Weather/Audio 仍有此问题）。
7. API Key（天气、天地图）目前写在源码中。
8. **SDK 的 Qt 版本偏旧**：`qHypot`（Qt5.10+）、部分 `QNetworkReply::NetworkError` 新枚举名都不存在。写数学/枚举时优先用老 API（`qSqrt(dx*dx+dy*dy)`、数字错误码区间），编译报错即换等价写法。

---

## 9. 已知状态 / 未完成

- 主页 UI 已改为深色卡片风（2026-10-12，见 §10）；子页 UI 未改。
- 倒车影像：预览 + 拍照已写；**录像未做**。
- 车速/电量/里程为写死演示值。
- `README.md` 可能过时或含冲突标记，以本文件为准。
- 工程曾引用 `mapview.cpp`，当前地图实现是 `map.cpp` 天地图静态图，无 `mapview` 源文件。

---

## 10. 改动记录（供交接，新条目放最上）

### 2026-10-12 首页 UI 深色卡片化（`mainwindow.ui` `speedgauge.cpp`）
- 首页整体改为深色车载风：深蓝紫渐变背景；顶栏 = 标题「智能车载终端」+ 状态点 + 右对齐日期/大号时钟。
- 主体两栏：左列 = 仪表盘卡（`speedGauge`）+ 速度/电量/里程三张数据条；右列 = **2×4 功能卡网格**（音乐/视频/天气/地图/环境/设置/倒车影像 7 个入口 + 系统状态卡，状态卡内含 GPS/网络占位行）。
- 功能卡统一深色卡样式，底部 3px 彩色描边区分模块（`:pressed` 变亮作按压反馈），样式集中在 `MainWindow.styleSheet`，不再逐按钮内联。
- 所有 objectName 保持不变（`pushButton`~`pushButton_5`、`btnSettings`、`btnCamera`、`datelable`、`timelable`、`speedGauge` 等），`mainwindow.cpp` **无需改动**。
- `speedgauge.cpp` 配色适配深色卡：底盘环 `#2d3e5c`、中央数字 `#f1f5f9`、单位/标题 `#94a3b8`；若以后换回浅色背景需同步改回。
- 待办：板端编译看整体效果；主页照片里日期是 1970 年 = 板子 RTC 未设置（`date -s` + `hwclock -w` 可修，与 UI 无关）。

### 2026-10-12 仪表盘三处修正（`speedgauge.cpp` `mainwindow.*`）
- **拖动 99↔100 整表缩放跳动**：根因是 `labelSpeed` 文本变宽 → statsRow 最小宽度变化 → 左列被重新分配 → `SpeedGauge::paintEvent` 里 `side=qMin(w,h)` 跟着变，整个表盘缩放跳帧。修法：构造时用 `fontMetrics().horizontalAdvance("180 km/h")` 给 `labelSpeed` 预留最宽最小宽度。**教训：会动态变文本的标签要预留宽度，否则连锁改布局。**
- 中心轮毂 r11→r7（红心 2.5），速度大数字 / km/h / "拖动圆环调节"提示整体下移到圆心下方（y=10/56/74），不再被指针和轮毂压住。
- 首页"网络：已连接"原来是 .ui 写死的假状态。新增 `MainWindow::checkNetwork()`：GET `http://www.baidu.com`（UA 用 `SmartCarIVI/1.0`），**收到任何 HTTP 响应（含 403）即算已连接**，5s 超时/无响应算未连接；进入时探测一次 + 每 30s 复检；未连接时该行文字变红，初始"检测中…"。

### 2026-10-12 仪表盘可拖动 + 电量 100%（`speedgauge.*` `mainwindow.*`）
- `SpeedGauge` 重绘为运动仪表风：外细圈、270° 底环 + 85%–100% 红色警示区、青→紫渐变进度弧、每 20 km/h 主刻度带数字（环外）+ 每 10 细刻度（环内）、菱形红指针 + 轮毂、中央 46px 大数字。
- **交互**：按住圆环区拖动即改时速（角度→速度映射在 `setSpeedFromPos`，底部 90° 缺口取最近端点，中心数字区不响应）；新增 `speedChanged(int)` 信号，`mainwindow.cpp` 里连到 `labelSpeed` 同步"当前速度"卡。演示时可直接手拖当"车速调节旋钮"。
- 半径分配（280 逻辑坐标系）：数字刻度 115 / 外圈 124 / 环 100±6 / 刻度线 72–88 / 指针 70，改样式时注意别互相压字。
- 电量卡固定文案改为 `100%`（仍是演示值，未接真实电池数据源）。

### 2026-10-12 视频播放器（`player.*` `videowidget.*` `playercontrols.cpp` `mainwindow.*`）
- 删除 `player->moveToThread(t2)` 及 t2 全部代码：界面控件不能离开 GUI 线程（原因见 §8-6）。
- 废弃 `QVideoWidget::setFullScreen` 与 `videoAvailableChanged`（内有重复 connect，是"全屏退不回来"根因之一），改为 `toggleFullScreen`/`setFullScreenUi` 布局级全屏，细节见 §4.3。
- seek/音量改 `sliderReleased` 松手提交，解决拖动卡顿。
- 全屏改为真铺满（边距清零+隐藏进度条行），单击视频浮出退出按钮、3s 自动隐藏。
- `open()` 过滤器由 MIME 改为后缀名，修复板端"Open 添加不了文件"。
- 待办：板端复测全屏三入口与退出后布局恢复；视频素材需按 §7.1 转码。

### 2026-10-12 地图（`map.*`）
- 修复一直停在 loading：补 `QNetworkReply` 错误处理、10s 超时、https→http 自动降级、PNG 磁盘缓存兜底。
- 天地图 403 根因：Qt5 默认 UA 是浏览器样式被 WAF 拦，改发 `SmartCarIVI/1.0` 后 200（详见记忆文件 `smartcar-tianditu-ua.md`）。
- 平移/缩放/回定位交互；`markers` 与 `center` 分离，定位针固定在定位点不再漂到屏幕中心。
- 注意：SDK 的 Qt 头文件缺部分 `QNetworkReply::NetworkError` 新枚举名，错误码判断用数字区间（201–299 内容类、401+ 服务器类）。
