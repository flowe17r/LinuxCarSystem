#ifndef V4L2CAPTURE_H
#define V4L2CAPTURE_H

#include <QAtomicInt>
#include <QImage>
#include <QString>
#include <QThread>
#include <cstddef>

/**
 * @brief USB/CSI 摄像头采集线程（V4L2 mmap）
 *
 * 在独立线程中打开 /dev/videoX、协商 MJPEG/YUYV、循环 DQBUF，
 * 将每一帧转成 QImage 后通过 frameReady 发给界面线程。
 * 界面来不及显示时丢弃中间帧，避免预览队列堆积卡死。
 */
class V4L2Capture : public QThread
{
    Q_OBJECT

public:
    explicit V4L2Capture(QObject *parent = nullptr);
    ~V4L2Capture() override;

    void setDevice(const QString &device);
    QString device() const { return m_device; }

    void stopCapture();
    void releaseFrame();

signals:
    void frameReady(const QImage &image);
    void captureStarted(int width, int height, const QString &format);
    void errorOccurred(const QString &message);

protected:
    void run() override;

private:
    struct Buffer {
        void *start = nullptr;
        size_t length = 0;
    };

    bool openDevice();
    bool initFormat();
    bool trySetFormat(quint32 fourcc, int width, int height);
    bool initMmap();
    bool startStreaming();
    void cleanup();
    QImage makeImage(const uchar *data, size_t bytes) const;
    QImage yuyvToRgb(const uchar *src, int width, int height) const;
    static QString fourccName(quint32 fourcc);

    QString m_device = QStringLiteral("/dev/video0");
    QAtomicInt m_abort;
    QAtomicInt m_frameBusy;
    int m_fd = -1;
    int m_width = 640;
    int m_height = 480;
    quint32 m_pixelformat = 0;
    Buffer *m_buffers = nullptr;
    unsigned int m_bufferCount = 0;
};

#endif // V4L2CAPTURE_H
