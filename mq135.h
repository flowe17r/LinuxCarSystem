#ifndef MQ135_H
#define MQ135_H

#include <QObject>
#include <QTimer>

class Mq135 : public QObject
{
    Q_OBJECT
public:
    explicit Mq135(QObject *parent = nullptr);
    ~Mq135();
    
    void start();
    void mq135_read(char *buf);
    
public slots:
    void timeto_read_mq135data(void);
    
signals:
    void readyto_read_mq135data();
    
private:
    QString in_voltage3_raw = "/sys/bus/iio/devices/iio:device0/in_voltage3_raw";
    int raw_fd = -1;
    QTimer * mq135_timer;
};

#endif // MQ135_H