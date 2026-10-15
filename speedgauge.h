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

signals:
    // 拖动或程序设置导致速度变化时发出
    void speedChanged(int kmh);

protected:
    void paintEvent(QPaintEvent *event) override;
    // 按住圆环拖动 → 指针跟手改变时速
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setSpeedFromPos(const QPoint &pos);

    int m_speed = 85;
    int m_maxSpeed = 180;
};

#endif // SPEEDGAUGE_H
