#include "led.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
Led::Led(QWidget *parent) : QMainWindow(parent)
{
    led_fd = open(led_device.toStdString().c_str(),O_RDWR);
    if(led_fd < 0)
    {
        qDebug() << "fail to open /dev/myled\n";
        return;
    }
    else
    {
        qDebug() << "open myled success" << endl;
    }
}

int Led::led_write(char* led_buf)
{
    int ret = write(led_fd,led_buf,2);
    return ret;
}
