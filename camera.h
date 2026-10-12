#ifndef CAMERA_H
#define CAMERA_H

#include <QImage>
#include <QWidget>

class QLabel;
class QPushButton;
class V4L2Capture;

class Camera : public QWidget
{
    Q_OBJECT

public:
    explicit Camera(QWidget *parent = nullptr);
    ~Camera() override;

public slots:
    void camera_working();

signals:
    void back();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onSnapshotClicked();
    void onBackClicked();
    void onFrame(const QImage &image);
    void onCaptureStarted(int width, int height, const QString &format);
    void onCaptureError(const QString &message);

private:
    void initUi();
    void stopPreview();
    void updateStatus(const QString &text);
    void setPreviewEnabled(bool running);

    QLabel *m_preview = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_snapBtn = nullptr;
    QPushButton *m_backBtn = nullptr;
    V4L2Capture *m_capture = nullptr;
    QImage m_lastFrame;
    bool m_running = false;
};

#endif // CAMERA_H
