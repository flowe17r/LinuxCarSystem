#ifndef DHT11_H
#define DHT11_H

#include <QMainWindow>

class DHT11 : public QMainWindow
{
    Q_OBJECT
public:
    explicit DHT11(QWidget *parent = nullptr);
    void dht11_read(char *buf);

public slots:
    void timeto_read_dht11data(void);
signals:
    void readyto_read_dht11data(void);

private:
    QString dht11_device = "/dev/mydht11";
    int dht11_fd;
    QTimer * dht11_timer;
};

#endif // DHT11_H
