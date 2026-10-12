#include "camera.h"
#include "v4l2capture.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const char *kToolBtnStyle =
    "QPushButton { min-width:110px; min-height:52px; font-size:20px; font-weight:bold;"
    " color:white; background-color:#2d6cdf; border:2px solid #8eb6ff; border-radius:6px; }"
    "QPushButton:pressed { background-color:#1a4a9e; }"
    "QPushButton:disabled { background-color:#4a5568; border-color:#718096; color:#cbd5e0; }";
const char *kBackBtnStyle =
    "QPushButton { min-width:110px; min-height:52px; font-size:20px; font-weight:bold;"
    " color:white; background-color:#c0392b; border:2px solid #ff8a80; border-radius:6px; }"
    "QPushButton:pressed { background-color:#922b21; }";

} // namespace

Camera::Camera(QWidget *parent)
    : QWidget(parent)
{
    m_capture = new V4L2Capture(this);
    connect(m_capture, &V4L2Capture::frameReady, this, &Camera::onFrame);
    connect(m_capture, &V4L2Capture::captureStarted, this, &Camera::onCaptureStarted);
    connect(m_capture, &V4L2Capture::errorOccurred, this, &Camera::onCaptureError);
    connect(m_capture, &QThread::finished, this, [this]() {
        m_running = false;
        setPreviewEnabled(false);
        m_capture->releaseFrame();
    });
    initUi();
}

Camera::~Camera()
{
    stopPreview();
}

void Camera::initUi()
{
    setFixedSize(1024, 600);
    setWindowFlag(Qt::FramelessWindowHint);

    m_preview = new QLabel(this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet("background-color: #101418; color: #9aa4b2; font-size: 22px;");
    m_preview->setText(QStringLiteral("等待打开摄像头…"));
    m_preview->setMinimumSize(1, 1);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setStyleSheet("color:white; font-size:16px;");
    updateStatus(QStringLiteral("倒车影像  ·  USB / V4L2"));

    m_startBtn = new QPushButton(QStringLiteral("开始"), this);
    m_stopBtn = new QPushButton(QStringLiteral("停止"), this);
    m_snapBtn = new QPushButton(QStringLiteral("拍照"), this);
    m_backBtn = new QPushButton(QStringLiteral("返回"), this);
    m_startBtn->setStyleSheet(kToolBtnStyle);
    m_stopBtn->setStyleSheet(kToolBtnStyle);
    m_snapBtn->setStyleSheet(kToolBtnStyle);
    m_backBtn->setStyleSheet(kBackBtnStyle);
    setPreviewEnabled(false);

    auto *barLayout = new QHBoxLayout;
    barLayout->setContentsMargins(8, 4, 8, 4);
    barLayout->setSpacing(10);
    barLayout->addWidget(m_status, 1);
    barLayout->addWidget(m_startBtn);
    barLayout->addWidget(m_stopBtn);
    barLayout->addWidget(m_snapBtn);
    barLayout->addWidget(m_backBtn);

    auto *bar = new QWidget(this);
    bar->setFixedHeight(64);
    bar->setStyleSheet("background-color: rgba(20, 24, 32, 230);");
    bar->setLayout(barLayout);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_preview, 1);
    mainLayout->addWidget(bar, 0);

    connect(m_startBtn, &QPushButton::clicked, this, &Camera::onStartClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &Camera::onStopClicked);
    connect(m_snapBtn, &QPushButton::clicked, this, &Camera::onSnapshotClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &Camera::onBackClicked);
}

void Camera::setPreviewEnabled(bool running)
{
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    m_snapBtn->setEnabled(running);
}

void Camera::updateStatus(const QString &text)
{
    if (m_status)
        m_status->setText(text);
}

void Camera::stopPreview()
{
    if (m_capture && m_capture->isRunning())
        m_capture->stopCapture();
    m_running = false;
    setPreviewEnabled(false);
}

void Camera::camera_working()
{
    setGeometry(0, 0, 1024, 600);
    raise();
    show();
    onStartClicked();
}

void Camera::onStartClicked()
{
    if (m_capture->isRunning())
        return;
    m_lastFrame = QImage();
    m_preview->setPixmap(QPixmap());
    m_preview->setText(QStringLiteral("正在打开摄像头…"));
    updateStatus(QStringLiteral("正在打开摄像头…"));
    m_running = true;
    setPreviewEnabled(true);
    m_capture->start();
}

void Camera::onStopClicked()
{
    stopPreview();
    m_preview->setPixmap(QPixmap());
    m_preview->setText(QStringLiteral("预览已停止"));
    updateStatus(QStringLiteral("预览已停止"));
}

void Camera::onSnapshotClicked()
{
    if (m_lastFrame.isNull()) {
        updateStatus(QStringLiteral("还没有画面，无法拍照"));
        return;
    }
    const QString dirPath = QCoreApplication::applicationDirPath() + QStringLiteral("/Photos");
    if (!QDir().mkpath(dirPath)) {
        updateStatus(QStringLiteral("无法创建 Photos 目录"));
        return;
    }
    const QString fileName = QStringLiteral("cam_%1.jpg")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    const QString filePath = dirPath + QLatin1Char('/') + fileName;
    if (m_lastFrame.save(filePath, "JPG", 85))
        updateStatus(QStringLiteral("已保存 ") + fileName);
    else
        updateStatus(QStringLiteral("保存失败"));
}

void Camera::onBackClicked()
{
    stopPreview();
    m_preview->setPixmap(QPixmap());
    m_preview->setText(QStringLiteral("等待打开摄像头…"));
    updateStatus(QStringLiteral("倒车影像  ·  USB / V4L2"));
    emit back();
}

void Camera::onFrame(const QImage &image)
{
    if (!m_running) {
        m_capture->releaseFrame();
        return;
    }
    m_lastFrame = image;
    const QSize target = m_preview->size();
    if (target.width() > 2 && target.height() > 2) {
        m_preview->setPixmap(QPixmap::fromImage(
            image.scaled(target, Qt::KeepAspectRatio, Qt::FastTransformation)));
    }
    m_capture->releaseFrame();
}

void Camera::onCaptureStarted(int width, int height, const QString &format)
{
    m_preview->setText(QString());
    updateStatus(QStringLiteral("%1  %2x%3  %4  预览中")
                     .arg(m_capture->device())
                     .arg(width)
                     .arg(height)
                     .arg(format));
}

void Camera::onCaptureError(const QString &message)
{
    qDebug() << "camera error:" << message;
    m_preview->setPixmap(QPixmap());
    m_preview->setText(message);
    updateStatus(message);
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
    m_snapBtn->setEnabled(false);
    m_running = false;
}
