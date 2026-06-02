#include "buzzer.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
Buzzer::Buzzer(QWidget *parent) : QMainWindow(parent)
{
    buzzer_fd = open(buzzer_device.toStdString().c_str(),O_RDWR);
    if(buzzer_fd < 0)
    {
        qDebug() << "fail to open /dev/mybuzzer\n";
        return;
    }
    else
    {
        qDebug() << "open mybuzzer success" << endl;
    }
}

int Buzzer::buzzer_write(char buf)
{
    int ret = write(buzzer_fd,&buf,1);
    return ret;
}
