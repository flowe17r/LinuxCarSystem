#include "v4l2capture.h"

#include <QDebug>

#if defined(Q_OS_LINUX)
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>
#endif

namespace {

#if defined(Q_OS_LINUX)
int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

bool isCaptureDevice(int fd)
{
    v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
        return false;

    const unsigned caps = (cap.device_caps != 0) ? cap.device_caps : cap.capabilities;
    return (caps & V4L2_CAP_VIDEO_CAPTURE) != 0;
}
#endif

} // namespace

V4L2Capture::V4L2Capture(QObject *parent)
    : QThread(parent)
{
}

V4L2Capture::~V4L2Capture()
{
    stopCapture();
}

void V4L2Capture::setDevice(const QString &device)
{
    if (!isRunning())
        m_device = device;
}

void V4L2Capture::stopCapture()
{
    m_abort.storeRelease(1);
    if (isRunning() && !wait(4000))
        qWarning() << "V4L2 capture thread did not stop in time";
}

void V4L2Capture::releaseFrame()
{
    m_frameBusy.storeRelease(0);
}

QString V4L2Capture::fourccName(quint32 fourcc)
{
    char name[5] = {
        char(fourcc & 0xFF),
        char((fourcc >> 8) & 0xFF),
        char((fourcc >> 16) & 0xFF),
        char((fourcc >> 24) & 0xFF),
        0
    };
    return QString::fromLatin1(name);
}

#if defined(Q_OS_LINUX)

bool V4L2Capture::openDevice()
{
    QStringList candidates;
    if (!m_device.isEmpty())
        candidates << m_device;
    for (int i = 0; i < 2; ++i) {
        const QString path = QStringLiteral("/dev/video%1").arg(i);
        if (!candidates.contains(path))
            candidates << path;
    }

    for (const QString &path : candidates) {
        const int fd = ::open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK, 0);
        if (fd < 0)
            continue;
        if (!isCaptureDevice(fd)) {
            ::close(fd);
            continue;
        }
        m_fd = fd;
        m_device = path;
        qDebug() << "V4L2 open OK:" << path;
        return true;
    }

    emit errorOccurred(QStringLiteral("无法打开摄像头设备，请检查 /dev/video0 是否存在"));
    return false;
}

bool V4L2Capture::trySetFormat(quint32 fourcc, int width, int height)
{
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = static_cast<unsigned>(width);
    fmt.fmt.pix.height = static_cast<unsigned>(height);
    fmt.fmt.pix.pixelformat = fourcc;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0)
        return false;
    if (fmt.fmt.pix.pixelformat != fourcc)
        return false;

    m_width = static_cast<int>(fmt.fmt.pix.width);
    m_height = static_cast<int>(fmt.fmt.pix.height);
    m_pixelformat = fmt.fmt.pix.pixelformat;
    return true;
}

bool V4L2Capture::initFormat()
{
    static const struct {
        quint32 fourcc;
        int w;
        int h;
    } kPrefs[] = {
        {V4L2_PIX_FMT_MJPEG, 640, 480},
        {V4L2_PIX_FMT_MJPEG, 320, 240},
        {V4L2_PIX_FMT_YUYV, 640, 480},
        {V4L2_PIX_FMT_YUYV, 320, 240},
    };

    for (const auto &pref : kPrefs) {
        if (trySetFormat(pref.fourcc, pref.w, pref.h)) {
            qDebug() << "V4L2 format:" << fourccName(m_pixelformat) << m_width << "x" << m_height;
            return true;
        }
    }

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_G_FMT, &fmt) < 0) {
        emit errorOccurred(QStringLiteral("摄像头不支持 MJPEG/YUYV 预览格式"));
        return false;
    }

    m_width = static_cast<int>(fmt.fmt.pix.width);
    m_height = static_cast<int>(fmt.fmt.pix.height);
    m_pixelformat = fmt.fmt.pix.pixelformat;
    qDebug() << "V4L2 fallback format:" << fourccName(m_pixelformat) << m_width << "x" << m_height;
    return m_width > 0 && m_height > 0;
}

bool V4L2Capture::initMmap()
{
    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
        emit errorOccurred(QStringLiteral("VIDIOC_REQBUFS 失败，设备可能不支持 mmap"));
        return false;
    }

    m_bufferCount = req.count;
    m_buffers = new Buffer[m_bufferCount];

    for (unsigned int i = 0; i < m_bufferCount; ++i) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            emit errorOccurred(QStringLiteral("VIDIOC_QUERYBUF 失败"));
            return false;
        }

        m_buffers[i].length = buf.length;
        m_buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, m_fd, buf.m.offset);
        if (m_buffers[i].start == MAP_FAILED) {
            m_buffers[i].start = nullptr;
            emit errorOccurred(QStringLiteral("mmap 失败"));
            return false;
        }
    }
    return true;
}

bool V4L2Capture::startStreaming()
{
    for (unsigned int i = 0; i < m_bufferCount; ++i) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            emit errorOccurred(QStringLiteral("VIDIOC_QBUF 失败"));
            return false;
        }
    }

    v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_G_PARM, &parm) == 0) {
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = 15;
        xioctl(m_fd, VIDIOC_S_PARM, &parm);
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        emit errorOccurred(QStringLiteral("VIDIOC_STREAMON 失败"));
        return false;
    }
    return true;
}

void V4L2Capture::cleanup()
{
    if (m_fd >= 0 && m_buffers) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(m_fd, VIDIOC_STREAMOFF, &type);
    }

    if (m_buffers) {
        for (unsigned int i = 0; i < m_bufferCount; ++i) {
            if (m_buffers[i].start && m_buffers[i].start != MAP_FAILED)
                munmap(m_buffers[i].start, m_buffers[i].length);
        }
        delete[] m_buffers;
        m_buffers = nullptr;
        m_bufferCount = 0;
    }

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

QImage V4L2Capture::makeImage(const uchar *data, size_t bytes) const
{
    if (!data || bytes == 0)
        return QImage();

    if (m_pixelformat == V4L2_PIX_FMT_MJPEG || m_pixelformat == V4L2_PIX_FMT_JPEG) {
        QImage img;
        if (!img.loadFromData(data, static_cast<int>(bytes), "JPEG"))
            return QImage();
        return img;
    }

    if (m_pixelformat == V4L2_PIX_FMT_YUYV)
        return yuyvToRgb(data, m_width, m_height);

    return QImage();
}

QImage V4L2Capture::yuyvToRgb(const uchar *src, int width, int height) const
{
    if (!src || width <= 0 || height <= 0)
        return QImage();

    QImage img(width, height, QImage::Format_RGB888);
    if (img.isNull())
        return img;

    for (int y = 0; y < height; ++y) {
        uchar *dst = img.scanLine(y);
        const uchar *row = src + y * width * 2;
        for (int x = 0; x < width; x += 2) {
            const int y0 = row[0];
            const int u = row[1];
            const int y1 = row[2];
            const int v = row[3];
            row += 4;

            const int d = u - 128;
            const int e = v - 128;
            auto convert = [&](int yy, uchar *p) {
                const int c = yy - 16;
                int r = (298 * c + 409 * e + 128) >> 8;
                int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
                int b = (298 * c + 516 * d + 128) >> 8;
                p[0] = static_cast<uchar>(qBound(0, r, 255));
                p[1] = static_cast<uchar>(qBound(0, g, 255));
                p[2] = static_cast<uchar>(qBound(0, b, 255));
            };
            convert(y0, dst);
            dst += 3;
            convert(y1, dst);
            dst += 3;
        }
    }
    return img;
}

void V4L2Capture::run()
{
    m_abort.storeRelease(0);
    m_frameBusy.storeRelease(0);

    if (!openDevice())
        return;
    if (!initFormat() || !initMmap() || !startStreaming()) {
        cleanup();
        return;
    }

    emit captureStarted(m_width, m_height, fourccName(m_pixelformat));

    while (!m_abort.loadAcquire()) {
        pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        const int pr = poll(&pfd, 1, 400);
        if (pr < 0) {
            if (errno == EINTR)
                continue;
            emit errorOccurred(QStringLiteral("poll 失败: ") + QString::fromLocal8Bit(strerror(errno)));
            break;
        }
        if (pr == 0)
            continue;

        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (xioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
            if (errno == EAGAIN || errno == EIO)
                continue;
            emit errorOccurred(QStringLiteral("VIDIOC_DQBUF 失败"));
            break;
        }

        if (buf.index < m_bufferCount && !m_frameBusy.loadAcquire()) {
            const Buffer &b = m_buffers[buf.index];
            const QImage img = makeImage(static_cast<const uchar *>(b.start), buf.bytesused);
            if (!img.isNull()) {
                m_frameBusy.storeRelease(1);
                emit frameReady(img);
            }
        }

        if (xioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            emit errorOccurred(QStringLiteral("VIDIOC_QBUF 归还缓冲失败"));
            break;
        }
    }

    cleanup();
}

#else // !Q_OS_LINUX

bool V4L2Capture::openDevice() { return false; }
bool V4L2Capture::initFormat() { return false; }
bool V4L2Capture::trySetFormat(quint32, int, int) { return false; }
bool V4L2Capture::initMmap() { return false; }
bool V4L2Capture::startStreaming() { return false; }
void V4L2Capture::cleanup() {}
QImage V4L2Capture::makeImage(const uchar *, size_t) const { return QImage(); }
QImage V4L2Capture::yuyvToRgb(const uchar *, int, int) const { return QImage(); }

void V4L2Capture::run()
{
    emit errorOccurred(QStringLiteral("当前系统不支持 V4L2，请在 Linux 开发板上运行"));
}

#endif
