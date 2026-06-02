# Smart_in-car_entertainment

基于 **Qt5** 的嵌入式 Linux 车载娱乐系统，面向 **1024×600** 触摸屏开发板（已在 **100ASK IMX6ULL Pro** 上验证）。主界面提供天气、音乐、视频、环境监控与 GPS 地图五个功能模块。

---

## 功能概览

| 模块 | 说明 |
|------|------|
| **主界面** | 显示当前日期与时间，五个功能入口 |
| **天气** | 通过易客天气 API 查询城市天气，展示 7 日预报与温度曲线 |
| **音乐** | 扫描本地 MP3，支持 LRC 歌词同步、唱片旋转动画、播放列表 |
| **视频** | 基于 Qt Multimedia 的本地视频播放，支持播放列表与 SQLite 记录 |
| **环境监控** | 读取温湿度（DHT11）；SR04 / MQ135 / LED / 蜂鸣器为可选，当前配置未使用 |
| **GPS 地图** | ATGM336H 经 `ttymxc5` 读取 NMEA，调用百度静态地图 API 显示当前位置 |

---

## 运行环境

### 硬件

- **开发板**：100ASK IMX6ULL Pro（或同类 ARM Linux 板）
- **屏幕**：7 寸 1024×600 电容/电阻触摸屏
- **网络**：WiFi 或有线（天气、地图功能需要联网）

#### 当前硬件配置

| 模块 | 型号 | 接口 / 设备节点 | 说明 |
|------|------|-----------------|------|
| **温湿度** | DHT11（百问配套） | `/dev/mydht11` | 需加载 `dht11_drv` 内核驱动；DATA 接 **GPIO4_IO19**（Linux GPIO **115**） |
| **GPS** | ATGM336H（百问配套，串口 TTL） | `/dev/ttymxc5`（UART5） | **9600** 8N1，NMEA **`$GPGGA`**；模块 **TX → 板子 RX**；室外或靠窗定位 |

接线请参考百问 **《嵌入式 Linux 应用开发完全手册》IMX6ULL Pro** 扩展板章节，或课程资料 `04_嵌入式Linux应用开发基础知识/doc_pic/10.UART/05_Linux串口应用编程.md`（§3.5.1 GPS 接线图）。

#### 未配置 / 可选外设

以下功能在工程中仍有代码，**当前未接硬件**，进入「环境监控」时可能打印 `fail to open /dev/...`，不影响主界面、天气、音乐、视频、GPS 与 DHT11：

| 模块 | 设备节点 |
|------|----------|
| HC-SR04 超声波 | `/dev/mysr04` |
| MQ135 空气质量 | `/sys/bus/iio/devices/iio:device0/in_voltage3_raw` |
| LED | `/dev/myled` |
| 蜂鸣器 | `/dev/mybuzzer` |

> 仅接 DHT11 + ATGM336H 时：先 `insmod dht11_drv.ko` 并确认 `/dev/mydht11`，GPS 用 `cat /dev/ttymxc5` 验证 NMEA 后再使用地图页。

### 软件依赖

- Qt 5（modules: `core gui widgets network xml multimedia multimediawidgets sql serialport`）
- 交叉工具链：`arm-buildroot-linux-gnueabihf-`（百问网 SDK 自带）
- 板端运行库：Qt5、tslib（触摸屏校准）

---

## 目录结构

```
Smart_car/
├── Smart_in-car_entertainment.pro   # Qt 工程文件
├── main.cpp / mainwindow.*          # 主程序与主界面
├── weather.*                        # 天气模块
├── Audio.*                          # 音乐播放器
├── player.* / playercontrols.*      # 视频播放器
├── hardware.*                       # 环境监控
├── map.*                            # GPS 地图
├── myMusic/                         # 示例音乐与 LRC 歌词
├── style/                           # QSS 样式
├── res.qrc                          # 图片等资源
└── build/                           # 编译输出目录（自行创建）
```

### 部署到板子后的目录建议

将可执行文件与资源放在同一目录，例如 `/root/`：

```
/root/
├── Smart_in-car_entertainment    # 可执行文件
├── myMusic/                      # MP3 + 同名 .lrc 歌词
├── Videos/                       # 本地视频（.mp4 / .mkv / .avi）
└── media_player.db               # 视频播放列表数据库（首次运行自动生成）
```

---

## 编译

> **必须在 Linux（百问网 Ubuntu 虚拟机）中交叉编译**，不要在 Windows 上直接用 Qt Creator 编译 ARM 版本。

### 1. 设置工具链

```bash
export ARCH=arm
export CROSS_COMPILE=arm-buildroot-linux-gnueabihf-
export PATH=$PATH:/home/book/100ask_imx6ull-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin
```

### 2. 使用 SDK 自带的 qmake

```bash
cd ~/Desktop/Smart_car
mkdir -p build && cd build

/home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/bin/qmake ../Smart_in-car_entertainment.pro
make -j4
```

编译成功后生成可执行文件：`build/Smart_in-car_entertainment`。

---

## 部署与运行

### 传输文件到开发板

WiFi 连通后，可用 **SCP**（不依赖 adb）：

```bash
scp Smart_in-car_entertainment root@板子IP:/root/
scp -r ../myMusic root@板子IP:/root/
ssh root@板子IP "chmod +x /root/Smart_in-car_entertainment"
```

### 关闭出厂界面（首次部署）

```bash
ssh root@板子IP
mv /etc/init.d/*hmi* /root/ 2>/dev/null
mv /etc/init.d/*lvgl* /root/ 2>/dev/null
reboot
```

### 手动运行

```bash
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export QT_QPA_FONTDIR=/usr/lib/fonts/

# 触摸屏（event 编号以板子为准，常见 event0 或 event1）
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/usr/lib/ts
export TSLIB_FBDEVICE=/dev/fb0

cd /root
./Smart_in-car_entertainment
```

触摸不准时，先执行 `ts_calibrate` 生成 `/etc/pointercal`，并确认 `event` 节点编号。

### 开机自启（可选）

可编写 `/etc/init.d/S107smart_car` 启动脚本，并在网络就绪（WiFi 或 eth1 配好 IP、DNS）后再启动程序，以便天气模块能正常联网、系统时间已校时。

---

## 各模块说明

### 天气

- 数据来源：[易客天气 API](http://v1.yiketianqi.com/)
- 需开发板能访问外网，并配置 DNS（如 `114.114.114.114`）
- 城市编码见 `citycodeutils.cpp`

### 音乐

- 自动扫描可执行文件同目录下 `myMusic/` 中的 `*.mp3`
- 同名 `*.lrc` 文件用于歌词显示
- 请将可播放的 MP3 放入该目录（部分加密格式无法播放）

### 视频

- 视频文件放在 `Videos/` 目录
- 支持 `.mp4`、`.mkv`、`.avi`
- 播放记录保存在 `media_player.db`

### 环境监控

- **当前硬件**：DHT11 → `/dev/mydht11`（需百问 `dht11_drv` 驱动）
- 温湿度在「硬件监控」页显示；未接 SR04 / MQ135 等时，距离与空气质量相关控件无效，终端可能有 open 失败日志

### GPS 地图

- **当前硬件**：ATGM336H → `/dev/ttymxc5`，9600 8N1
- 默认坐标：`118.804341,31.975024`（无定位时使用）
- 串口收到有效 `$GPGGA` 后自动更新位置；需联网加载百度静态地图（`map.h` 中配置 AK）
- 室内信号弱时可把模块放到窗外，或使用外置有源 GPS 天线

---

## 常见问题

| 现象 | 可能原因 | 处理 |
|------|----------|------|
| 编译缺少 multimedia / serialport | qmake 路径不对 | 使用 SDK Buildroot 目录下的 qmake |
| 黑屏或被占用 | 出厂 HMI/LVGL 未关闭 | 移走 `/etc/init.d/` 中相关脚本 |
| 触摸无反应 | 未校准或 event 号错误 | `ts_calibrate`，修改 `event0/event1` |
| 天气不更新 | 无网络或未校时 | 检查 WiFi/有线、DNS、`ntpdate` |
| `fail to open /dev/mysr04` 等 | 未接 SR04/MQ135/LED 等（当前配置） | 可忽略；仅使用 DHT11 + GPS 时属正常 |
| `fail to open /dev/mydht11` | DHT11 驱动未加载或接线错误 | `insmod dht11_drv.ko`，检查 GPIO 与扩展板 |
| GPS 无数据 / 地图不更新 | 室内无星号或未接 UART5 | `cat /dev/ttymxc5` 测 NMEA；板子需能上网 |
| adb 不可用 | USB gadget 启动失败 | 使用 `scp` / `ssh` 传文件 |

---

## 技术栈

- **语言**：C++11
- **框架**：Qt5 Widgets、Qt Multimedia、Qt Network、Qt SerialPort、Qt SQL
- **架构**：主界面 + 多子模块，各模块在独立 `QThread` 中初始化

---

## 许可证

本项目为开源车载娱乐系统示例代码，具体许可证以仓库说明为准。第三方 API（易客天气、百度地图）请遵守各自服务条款。
