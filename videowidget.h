#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H


#include <QLabel>
#include <QVideoWidget>


class VideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    // 这个构造函数会调用父类QVideoWidget的构造函数来完成基本的初始化工作
    explicit VideoWidget(QWidget *parent = nullptr);

protected:

    // 用于处理鼠标双击事件
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    // 用于处理鼠标按下事件
    void mousePressEvent(QMouseEvent *event) override;

signals:
    // 双击视频区域：由 Player 决定进入/退出全屏，不在这里动 QVideoWidget 的顶层窗口全屏
    void doubleClicked();
    // 全屏时单击视频区域唤出退出按钮
    void clicked();

};

#endif // VIDEOWIDGET_H
