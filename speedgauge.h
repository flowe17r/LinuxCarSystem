#ifndef SPEEDGAUGE_H
#define SPEEDGAUGE_H

#include <QWidget>

class SpeedGauge : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedGauge(QWidget *parent = nullptr);
    void setSpeed(int kmh);
    int speed() const { return m_speed; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_speed = 85;
    int m_maxSpeed = 180;
};

#endif // SPEEDGAUGE_H
