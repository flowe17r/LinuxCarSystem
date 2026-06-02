#ifndef ROTATABLEWIDGET_H
#define ROTATABLEWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QPixmap>
#include <QPropertyAnimation>

namespace Ui {
class RotatableWidget;
}

class RotatableWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)

public:
    explicit RotatableWidget(QWidget *parent = nullptr,
                             const QPixmap &pixmap = QPixmap(),
                             const QPointF &rotationCenter = QPointF());
    ~RotatableWidget();
    qreal rotation() const;
    void setRotation(qreal newRotation);

signals:
    void rotationChanged(qreal);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::RotatableWidget *ui;
    qreal m_rotation;
    QPixmap m_pixmap;
    QPointF m_rotationCenter; // 新增成员变量用于存储旋转中心点坐标

};

#endif // ROTATABLEWIDGET_H
