#include "rotatablewidget.h"
#include "ui_rotatablewidget.h"
#include <QPainter>
// 创建可旋转部件
RotatableWidget::RotatableWidget(QWidget *parent, const QPixmap &pixmap,
                                 const QPointF &rotationCenter)
    : QWidget(parent), m_rotation(0), m_pixmap(pixmap),
      m_rotationCenter(rotationCenter)
{
    if (m_rotationCenter.isNull()) { // 如果没有指定旋转中心，则默认为控件中心
        m_rotationCenter = QPointF(width() / 2.0, height() / 2.0);
    }
}
// 获取当前部件的旋转角度
qreal RotatableWidget::rotation() const {
    return m_rotation;
}
// 设置新的旋转角度
void RotatableWidget::setRotation(qreal newRotation) {
    if (m_rotation == newRotation)
        return;

    m_rotation = newRotation;
    emit rotationChanged(m_rotation);
    update(); // 重绘控件
}
// 重绘事件处理函数，当部件需要重绘时会调用此函数
void RotatableWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿
    painter.translate(m_rotationCenter.x(), m_rotationCenter.y()); // 将旋转中心移动到设定的点
    painter.rotate(m_rotation); // 应用旋转
    painter.translate(-m_rotationCenter.x(), -m_rotationCenter.y()); // 移回旋转中心
    if (!m_pixmap.isNull()) {
        // 在部件的中心位置绘制图像，通过计算使图像在部件中居中显示
        // (width() - m_pixmap.width()) / 2：计算图像在水平方向上的偏移量，使其水平居中
        // (height() - m_pixmap.height()) / 2：计算图像在垂直方向上的偏移量，使其垂直居中
        painter.drawPixmap((width() - m_pixmap.width()) / 2, (height() - m_pixmap.height()) / 2, m_pixmap);
    }
}


// 析构函数：用于释放资源
RotatableWidget::~RotatableWidget()
{
    delete ui;
}
