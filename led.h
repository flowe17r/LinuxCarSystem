#ifndef LED_H
#define LED_H

#include <QMainWindow>

class Led : public QMainWindow
{
    Q_OBJECT
public:
    explicit Led(QWidget *parent = nullptr);
    int led_write(char* led_buf);
signals:

private:
    QString led_device = "/dev/myled";
    int led_fd;
};

#endif // LED_H
