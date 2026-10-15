#include "videowidget.h"

#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedLayout>
#include <QVBoxLayout>

// VideoWidget类继承自QVideoWidget，用于创建一个自定义的视频显示部件，可处理一些特定的用户交互事件
VideoWidget::VideoWidget(QWidget *parent)
    : QVideoWidget(parent)
{

   setStyleSheet("background-color: transparent;");

}




// 重写鼠标双击事件处理函数：只发信号，由 Player 统一做布局级全屏切换。
// 之前直接调 QVideoWidget::setFullScreen 会变成独立顶层窗口，退出入口不可见
void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit doubleClicked();

    // 接受该事件，表示该事件已被处理，不再向上传递给父部件或其他处理函数
    event->accept();
}

// 重写鼠标按下事件处理函数：发单击信号（全屏时用于唤出浮层按钮），
// 再交给父类保持默认行为
void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked();
    QVideoWidget::mousePressEvent(event);
}
