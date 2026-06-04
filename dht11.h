#ifndef DHT11_H
#define DHT11_H

#include <QObject>

class DHT11 : public QObject
{
    Q_OBJECT
public:
    explicit DHT11(QObject *parent = nullptr);
    ~DHT11();
    void start();
    int dht11_read(char *buf);

public slots:
    void timeto_read_dht11data(void);

signals:
    void readyto_read_dht11data(void);

private:
    QString dht11_device = "/dev/mydht11";
    int dht11_fd = -1;
    class QTimer *dht11_timer = nullptr;
};

#endif // DHT11_H
