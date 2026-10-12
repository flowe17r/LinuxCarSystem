#include "mq135.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <QTimer>

Mq135::Mq135(QObject *parent) : QObject(parent)
{
    mq135_timer = new QTimer(this);
    mq135_timer->setInterval(2000);
    connect(mq135_timer, &QTimer::timeout, this, &Mq135::timeto_read_mq135data);
}

Mq135::~Mq135()
{
    if (raw_fd >= 0) {
        close(raw_fd);
        qDebug() << "close in_voltage3_raw";
    }
}

void Mq135::start()
{
    if (raw_fd < 0) {
        raw_fd = open(in_voltage3_raw.toStdString().c_str(), O_RDONLY);
        if (raw_fd < 0) {
            qDebug() << "fail to open in_voltage3_raw";
            return;
        }
        qDebug() << "open in_voltage3_raw success";
    }
    
    if (!mq135_timer->isActive()) {
        mq135_timer->start();
    }
}

void Mq135::mq135_read(char *buf)
{
    int len;
    
    if (raw_fd < 0) {
        qDebug() << "mq135 device not opened";
        return;
    }
    
    lseek(raw_fd, 0, SEEK_SET);
    len = read(raw_fd, buf, sizeof(buf));
    
    if (len < 0) {
        qDebug() << "mq135 read error";
    }
}

void Mq135::timeto_read_mq135data()
{
    emit readyto_read_mq135data();
}