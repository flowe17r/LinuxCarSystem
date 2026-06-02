#ifndef SR04_H
#define SR04_H

#include <QMainWindow>

class Sr04 : public QMainWindow
{
    Q_OBJECT
public:
    explicit Sr04(QWidget *parent = nullptr);
    void sr04_read(int *buf);

public slots:
    void timeto_read_sr04data(void);
signals:
    void readyto_read_sr04data(void);
private:
    int sr04_fd;
    QString sr04_device = "/dev/mysr04";
    QTimer * sr04_timer;
};

#endif // SR04_H
