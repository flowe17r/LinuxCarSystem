#ifndef Hardware_H
#define Hardware_H

#include <QMainWindow>
#include "buzzer.h"
#include "dht11.h"
#include "led.h"
#include "mq135.h"
#include "sr04.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Hardware; }
QT_END_NAMESPACE

class Hardware : public QMainWindow
{
    Q_OBJECT

public:
    Hardware(QWidget *parent = nullptr);
    ~Hardware();
public slots:
    void dht11_handler(void);
    void sr04_handler(void);
    void mq135_handler(void);
    void hardware_working(void);

signals:
    void back1();
private slots:
    void on_back1_clicked();

private:
    Ui::Hardware *ui;
    DHT11* dht11;
    Sr04* sr04;
    Buzzer* buzzer;
    Led* led;
    Mq135* mq135;

};
#endif // MAINWINDOW_H
