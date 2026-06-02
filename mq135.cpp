#include "mq135.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <QTimer>
Mq135::Mq135(QWidget *parent) : QMainWindow(parent)
{

    mq135_timer = new QTimer(this);
    mq135_timer->setInterval(2000);
    connect(mq135_timer,&QTimer::timeout,this,&Mq135::timeto_read_mq135data);
    mq135_timer->start();

}

void Mq135::mq135_read(char *buf)
{
    int len;
    raw_fd = open(in_voltage3_raw.toStdString().c_str(),O_RDONLY);
    if(raw_fd < 0)
    {
        qDebug() << "fail to open in_voltage3_raw\n";
        return;
    }
    else
    {
        qDebug() << "open in_voltage3_raw success" << endl;
    }
    len = read(raw_fd, buf, sizeof(buf));
}



void Mq135::timeto_read_mq135data()
{
    emit readyto_read_mq135data();
}


