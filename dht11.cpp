#include "dht11.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <QTimer>

DHT11::DHT11(QObject *parent) : QObject(parent)
{
    // 构造函数不做任何操作，设备打开延迟到 start()
}

DHT11::~DHT11()
{
    if (dht11_fd >= 0) {
        close(dht11_fd);
    }
    if (dht11_timer) {
        dht11_timer->stop();
        delete dht11_timer;
    }
}

void DHT11::start()
{
    if (dht11_fd >= 0) {
        qDebug() << "DHT11 already started";
        return;
    }

    dht11_fd = open(dht11_device.toStdString().c_str(), O_RDONLY);
    if (dht11_fd < 0) {
        qDebug() << "fail to open /dev/mydht11";
        return;
    } else {
        qDebug() << "open dht11 success";
    }

    dht11_timer = new QTimer(this);
    dht11_timer->setInterval(2000);
    connect(dht11_timer, &QTimer::timeout, this, &DHT11::timeto_read_dht11data);
    dht11_timer->start();
}

int DHT11::dht11_read(char *buf)
{
    int len = -1;

    if (dht11_fd < 0) {
        qDebug() << "DHT11 device not opened";
        return len;
    }

    len = read(dht11_fd, buf, 4);
    if (len != 4) {
        qDebug() << "DHT11 read failed:" << len;
    }
    return len;
}

void DHT11::timeto_read_dht11data()
{
    emit readyto_read_dht11data();
}
