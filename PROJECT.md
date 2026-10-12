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
        ├── Weather / Player / Audio 曾 moveToThread（含界面的 QWidget 移线程并不规范，改动需谨慎）
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
- 播放列表写入 SQLite：`media_player.db` 表 `video`
- 无 `player_working()`，主页点击后直接 `show`
- 返回：`back()`

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
- 静态图 URL 带 `markers=<中心坐标>`，定位针由天地图服务端绘制（已实测返回 1024×600 PNG 含红针）
- 请求带 10s 超时与错误处理；板端 Qt 若无 OpenSSL（`QSslSocket::supportsSsl()` 为假）自动降级 `http://`
- 成功后把图片缓存到 `应用程序目录/map_cache.png`；再次失败时显示上次地图并在底栏给出原因，不会卡在 loading
- 定位成功后按 5s 节流刷新地图，避免 GGA 每秒一条打满弱 CPU
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

---

## 8. 给后续改代码的约束

1. 分辨率固定 **1024×600**，主页用 layout，不要再堆绝对坐标大图。
2. 倒车影像只改 `camera.*` / `v4l2capture.*`；本地视频播放只改 `player.*`。
3. 采集必须在 `V4L2Capture` 线程；离开倒车页必须停流、close。
4. 子页面风格尚未统一，第一轮不要大改 `weather.ui` / `Audio.ui` 等，除非需求明确要改。
5. IMX6ULL CPU 弱：预览建议 640×480；若加录像，优先 MJPEG 封装，不要软件 H.264。
6. 含界面的类不要 `moveToThread`（历史代码里 Weather/Player/Audio 有此问题）。
7. API Key（天气、天地图）目前写在源码中。

---

## 9. 已知状态 / 未完成

- 主页仪表盘 UI 已改；子页 UI 未改。
- 倒车影像：预览 + 拍照已写；**录像未做**。
- 车速/电量/里程为写死演示值。
- `README.md` 可能过时或含冲突标记，以本文件为准。
- 工程曾引用 `mapview.cpp`，当前地图实现是 `map.cpp` 天地图静态图，无 `mapview` 源文件。
