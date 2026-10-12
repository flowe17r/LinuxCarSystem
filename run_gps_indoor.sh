#!/bin/bash
# 室内 GPS 地图测试：直接运行主程序，进入 GPS 模块即可
# （Map 模块会在无有效卫星信号时自动使用默认位置：上海市浦东新区公元新村）
#
# 用法: chmod +x run_gps_indoor.sh && ./run_gps_indoor.sh

export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export QT_QPA_FONTDIR=/usr/lib/fonts/
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_CALIBFILE=/etc/pointercal
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/usr/lib/ts
export TSLIB_FBDEVICE=/dev/fb0

cd "$(dirname "$0")"
BIN="./Smart_in-car_entertainment"
[ -f "/root/Smart_in-car_entertainment" ] && BIN="/root/Smart_in-car_entertainment"

echo "启动主程序，请点击 GPS 地图 进入模块"
exec "$BIN"
