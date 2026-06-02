#include "dht11.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <QTimer>
DHT11::DHT11(QWidget *parent) : QMainWindow(parent)
{
    dht11_fd = open(dht11_device.toStdString().c_str(),O_RDONLY);
    if(dht11_fd < 0)
    {
        qDebug() << "fail to open /dev/mydht11\n";
        return;
    }
    else
    {
        qDebug() << "open dht11 success" << endl;
    }
    dht11_timer = new QTimer(this);
    dht11_timer->setInterval(2000);
    connect(dht11_timer,&QTimer::timeout,this,&DHT11::timeto_read_dht11data);
    dht11_timer->start();
}

void DHT11::dht11_read(char *buf)
{
    int len;
    len = read(dht11_fd, buf, 4);
}

void DHT11::timeto_read_dht11data()
{
    emit readyto_read_dht11data();
}
