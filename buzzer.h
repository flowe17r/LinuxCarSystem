#ifndef BUZZER_H
#define BUZZER_H

#include <QMainWindow>

class Buzzer : public QMainWindow
{
    Q_OBJECT
public:
    explicit Buzzer(QWidget *parent = nullptr);
    int buzzer_write(char buf);
signals:

private:
    QString buzzer_device = "/dev/mybuzzer";
    int buzzer_fd;
};

#endif // BUZZER_H
