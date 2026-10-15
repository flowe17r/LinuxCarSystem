#include "map.h"

#include "simplemessagebox.h"
#include "commonhelper.h"

#include <QSslConfiguration>
#include <QSslSocket>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QMouseEvent>
#include <QtMath>

namespace {
// GPS 每秒一条 GGA，限流避免弱 CPU + 移动网络被请求打满
const qint64 kRefreshIntervalMs = 5000;
const int kRequestTimeoutMs = 10000;

// NMEA 的 ddmm.mmmm 转十进制度
double toDecimalDegree(double raw)
{
    int deg = int(raw / 100);
    return deg + (raw - deg * 100) / 60.0;
}

// "lon,lat" 解析；静态图中心点运算用
bool parseCenter(const QString &s, double &lon, double &lat)
{
    const QStringList parts = s.split(',');
    if (parts.count() != 2)
        return false;
    bool okLon = false, okLat = false;
    lon = parts.at(0).toDouble(&okLon);
    lat = parts.at(1).toDouble(&okLat);
    return okLon && okLat;
}
}

Map::Map(QWidget *parent) : QDialog(parent)
{
    initUi();

    m_cachePath = QCoreApplication::applicationDirPath() + "/map_cache.png";
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        if (!m_reply)
            return;
        m_errorText = QStringLiteral("请求超时(10s)，请检查板子网络与 DNS");
        m_reply->abort();
    });
}

Map::~Map()
{
    if (m_port.isOpen())
        m_port.close();
}

void Map::initUi()
{
    this->setFixedSize(1024, 600);
    m_logoLabel.setObjectName("logoLabel");
    setStatusText();
    m_back = new QPushButton(tr("返回"), this);
    m_zoomIn = new QPushButton(QStringLiteral("+"), this);
    m_zoomOut = new QPushButton(QStringLiteral("-"), this);
    m_recenter = new QPushButton(tr("回定位"), this);
    m_zoomIn->setFixedSize(56, 44);
    m_zoomOut->setFixedSize(56, 44);
    m_recenter->setFixedSize(88, 44);
    connect(m_zoomIn, &QPushButton::clicked, this, [this]() { zoomBy(1); });
    connect(m_zoomOut, &QPushButton::clicked, this, [this]() { zoomBy(-1); });
    connect(m_recenter, &QPushButton::clicked, this, &Map::recenter);

    // QLabel 接管 QMovie 生命周期（setPixmap/clear 时会销毁它），故不持有裸指针
    auto pMovie = new QMovie(QStringLiteral(":/images/loading.gif"));
    m_iconLabel.setMovie(pMovie);
    m_iconLabel.setAlignment(Qt::AlignCenter);
    m_logoLabel.setAlignment(Qt::AlignRight);

    auto *pTopRow = new QHBoxLayout;
    pTopRow->addStretch();
    pTopRow->addWidget(m_zoomIn);
    pTopRow->addWidget(m_zoomOut);
    pTopRow->addWidget(m_recenter);
    auto *pVLayout = new QVBoxLayout;
    pVLayout->addLayout(pTopRow);
    pVLayout->addStretch();
    pVLayout->addWidget(&m_logoLabel);
    pVLayout->addWidget(m_back);
    m_iconLabel.setLayout(pVLayout);
    connect(m_back, &QPushButton::clicked, this, &Map::on_back_clicked);

    auto *pMainLayout = new QVBoxLayout;
    pMainLayout->addWidget(&m_iconLabel);
    pMainLayout->setMargin(0);
    setLayout(pMainLayout);

    // 请求期间的浮层提示：单核 A7 解码 1024x600 PNG 有约半秒冻结，
    // 有提示后用户感知为"加载"而不是"卡死"
    m_busyLabel = new QLabel(&m_iconLabel);
    m_busyLabel->setAlignment(Qt::AlignCenter);
    m_busyLabel->setGeometry(362, 260, 300, 80);
    m_busyLabel->setStyleSheet("background: rgba(0,0,0,120); color: white;"
                               " font-size: 24px; border-radius: 8px;");
    m_busyLabel->setText(QStringLiteral("加载地图…"));
    m_busyLabel->hide();

    setObjectName("map_widget");
    CommonHelper::setStyleSheet(":/style/default.qss", this);
}

void Map::initCtrl()
{
    initSerialPort();
    m_httpRetried = false;
    m_userMoved = false;
    m_location = m_fixLocation;
    requestMapImage(m_location);
}

// 串口初始化
void Map::initSerialPort()
{
    if (m_port.isOpen())
        return;

    // 设置串口名称 (UART6)
    m_port.setPortName("ttymxc5");
    // 以读写方式打开串口设备
    if (!m_port.open(QIODevice::ReadWrite)) {
        m_noGpsDevice = true;
        m_hasFix = false;
        if (!m_gpsWarned) {
            m_gpsWarned = true;
            SimpleMessageBox::infomationMessageBox("未检测到GPS设备，使用默认位置");
        }
        setStatusText();
        return;
    }

    m_noGpsDevice = false;
    m_port.setBaudRate(QSerialPort::Baud9600, QSerialPort::AllDirections);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);

    // 只连一次，避免重复进入页面时同一信号触发多次槽
    if (!m_portConnected) {
        connect(&m_port, &QSerialPort::readyRead, this, &Map::portReadReady);
        m_portConnected = true;
    }
    setStatusText();
}

// 请求地图照片
void Map::requestMapImage(const QString &center)
{
    // 板端 Qt 若没编进 OpenSSL，https 会直接以 "Protocol not available" 失败，
    // 表现为一直卡在 loading；此时降级为 http（天地图两种协议均可用）
    startRequest(buildUrl(center, QSslSocket::supportsSsl()));
}

QString Map::buildUrl(const QString &center, bool https) const
{
    QString url = tdtStaticUrl.arg(tdtToken).arg(center).arg(m_zoom).arg(m_fixLocation);
    if (!https)
        url.replace("https://", "http://");
    return url;
}

void Map::startRequest(const QString &url)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_array.clear();
    m_errorText.clear();

    QNetworkRequest networkRequest;
    m_lastUrlWasHttps = url.startsWith("https");
    if (m_lastUrlWasHttps) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        // 板端无 CA 证书 / 系统时间不对时避免握手直接失败
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
        networkRequest.setSslConfiguration(config);
    }
    networkRequest.setUrl(url);
    // 天地图 WAF 会拦浏览器型 UA（Qt5 默认是 "Mozilla/5.0 ... Qt/x.x"），返回 403；
    // 换成普通应用名 UA 才能拿到 200
    networkRequest.setRawHeader("User-Agent", "SmartCarIVI/1.0");

    qDebug() << "[Map] request:" << url;
    m_busyLabel->show();
    m_busyLabel->raise();
    m_reply = m_networkAccessManager.get(networkRequest);
    m_timeout.start(kRequestTimeoutMs);
    connect(m_reply, &QNetworkReply::readyRead, this, &Map::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &Map::onFinished);
}

void Map::onReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply)
        m_array.append(reply->readAll());
}

void Map::onFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    m_timeout.stop();
    if (reply != m_reply) {
        reply->deleteLater();
        return;
    }
    m_reply = nullptr;

    m_array.append(reply->readAll());
    const QNetworkReply::NetworkError err = reply->error();
    const QString errString = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    QPixmap pixmap;
    if (err == QNetworkReply::NoError)
        pixmap.loadFromData(m_array);

    if (err != QNetworkReply::NoError || pixmap.isNull()) {
        qDebug() << "[Map] failed:" << int(err) << errString;

        // 板端老 Qt 的 TLS 经常握手失败：https 挂了自动用 http 再试一次
        if (err != QNetworkReply::NoError && m_lastUrlWasHttps && !m_httpRetried) {
            m_httpRetried = true;
            m_errorText.clear();
            retryWithHttp();
            return;
        }

        QString reason = m_errorText;
        m_errorText.clear();
        if (reason.isEmpty()) {
            if (err != QNetworkReply::NoError)
                reason = QStringLiteral("[%1] %2").arg(int(err)).arg(shortErrorText(err));
            else
                reason = QStringLiteral("返回数据无法解码（缺少 PNG 图像插件）");
        }
        showFallback(reason);
        return;
    }

    // 服务端已按 markers 参数（定位点）画出定位针
    QFile file(m_cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(m_array);
        file.close();
    }
    showMap(pixmap, QString());
}

void Map::retryWithHttp()
{
    startRequest(buildUrl(m_location, false));
}

// 拖动平移：像素偏移按 Web Mercator 在当前 zoom 下换算成经纬度偏移后重新请求
void Map::panBy(const QPoint &pixelDelta)
{
    double lon = 0, lat = 0;
    if (!parseCenter(m_location, lon, lat))
        return;

    const double degPerPx = 360.0 / (256.0 * double(1 << m_zoom));
    lon -= pixelDelta.x() * degPerPx;
    const double cosLat = qCos(qDegreesToRadians(lat));
    lat += pixelDelta.y() * degPerPx * (cosLat > 0.05 ? cosLat : 0.05);
    lat = qBound(-85.0, lat, 85.0);
    if (lon > 180.0)
        lon -= 360.0;
    else if (lon < -180.0)
        lon += 360.0;

    m_location = QString::number(lon, 'f', 6) + ',' + QString::number(lat, 'f', 6);
    m_userMoved = true;
    setStatusText();
    requestMapImage(m_location);
}

void Map::zoomBy(int step)
{
    const int zoom = qBound(3, m_zoom + step, 18);
    if (zoom == m_zoom)
        return;
    m_zoom = zoom;
    m_userMoved = true;
    requestMapImage(m_location);
}

void Map::recenter()
{
    m_userMoved = false;
    m_location = m_fixLocation;
    setStatusText();
    requestMapImage(m_location);
}

void Map::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStart = event->pos();
        m_dragging = true;
    }
    QDialog::mousePressEvent(event);
}

void Map::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void Map::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        const QPoint delta = event->pos() - m_dragStart;
        // 小于阈值视为点按，不触发平移
        if (delta.manhattanLength() > 30)
            panBy(delta);
    }
    QDialog::mouseReleaseEvent(event);
}

// errorString 会把完整 URL 拼进去（"Error transferring <url> - ..."），屏上放不下，
// 这里只给短描述，完整串走 qDebug
QString Map::shortErrorText(QNetworkReply::NetworkError err) const
{
    switch (err) {
    case QNetworkReply::HostNotFoundError:
        return QStringLiteral("DNS 解析失败，检查板子 DNS");
    case QNetworkReply::ConnectionRefusedError:
        return QStringLiteral("服务器拒绝连接");
    case QNetworkReply::TimeoutError:
        return QStringLiteral("请求超时");
    case QNetworkReply::OperationCanceledError:
        return QStringLiteral("请求被取消");
    case QNetworkReply::SslHandshakeFailedError:
        return QStringLiteral("TLS 握手失败（已尝试 http 仍失败）");
    case QNetworkReply::TemporaryNetworkFailureError:
        return QStringLiteral("网络临时故障");
    case QNetworkReply::NetworkSessionFailedError:
        return QStringLiteral("网络会话失败");
    case QNetworkReply::ProtocolUnknownError:
        return QStringLiteral("Qt 不支持该协议");
    case QNetworkReply::ProtocolInvalidOperationError:
        return QStringLiteral("协议操作无效");
    case QNetworkReply::UnknownNetworkError:
        return QStringLiteral("网络不可达");
    default:
        break;
    }
    // content(201-299) / server(401+) 段枚举在部分旧 Qt 头文件里缺失，用数值区间兜底
    const int code = int(err);
    if (code >= 201 && code <= 299)
        return QStringLiteral("服务器拒绝内容(%1)").arg(code);
    if (code >= 401)
        return QStringLiteral("服务器错误(%1)").arg(code);
    return QStringLiteral("未知网络错误(%1)").arg(code);
}

void Map::showMap(const QPixmap &pixmap, const QString &note)
{
    m_busyLabel->hide();
    m_iconLabel.setWordWrap(false);
    m_iconLabel.setPixmap(pixmap);
    if (!note.isEmpty())
        m_logoLabel.setText(note);
    else
        setStatusText();
}

// 网络失败时的兜底：优先显示上次成功缓存的地图，否则给出明确原因，绝不卡在 loading
void Map::showFallback(const QString &reason)
{
    m_busyLabel->hide();
    QPixmap cached;
    if (cached.load(m_cachePath)) {
        QString note = QStringLiteral("网络异常：%1（显示上次地图）").arg(reason);
        showMap(cached, note);
        return;
    }

    m_iconLabel.clear();  // 销毁 loading 动画
    m_iconLabel.setWordWrap(true);
    m_iconLabel.setText(QStringLiteral("地图加载失败：%1").arg(reason));
    m_logoLabel.setText(QStringLiteral("%1  [%2]").arg(m_defaultPlace).arg(m_fixLocation));
}

void Map::setStatusText()
{
    QString text;
    if (m_userMoved)
        text = QStringLiteral("地图已移动/缩放，中心：[") + m_location
               + QStringLiteral("]；按“回定位”返回当前位置");
    else if (m_hasFix)
        text = QStringLiteral("GPS定位成功：") + m_fixLocation;
    else if (m_noGpsDevice)
        text = QStringLiteral("未检测到GPS设备，展示默认位置：") + m_defaultPlace
               + QStringLiteral("  [") + m_fixLocation + QStringLiteral("]");
    else
        text = QStringLiteral("GPS定位中，移动至空旷处；当前展示默认位置：") + m_defaultPlace
               + QStringLiteral("  [") + m_fixLocation + QStringLiteral("]");
    m_logoLabel.setText(text);
}

// 读取串口数据
void Map::portReadReady()
{
    // 当串口仍有数据
    while (m_port.bytesAvailable()) {
        QString line = m_port.readLine(); // 读取一行
        if (line.isEmpty())
            break;

        auto location = parseGpsData(line); // 解析GPS数据
        if (location.isEmpty()) {
            if (!m_hasFix)
                setStatusText();
            continue;
        }

        m_hasFix = true;
        m_fixLocation = location;
        setStatusText();

        // 用户手动拖动/缩放后不再被 GPS 拽走，按"回定位"才恢复跟随
        if (m_userMoved)
            continue;

        const bool moved = (location != m_location);
        m_location = location;

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (moved && now - m_lastRequestMs >= kRefreshIntervalMs) {
            m_lastRequestMs = now;
            requestMapImage(location); // 根据经纬度定位
        }
    }
}

void Map::map_working()
{
    initCtrl();
    setModal(true);

    this->show();
}

void Map::on_back_clicked()
{
    m_timeout.stop();
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_port.isOpen())
        m_port.close();

    emit backout();
}

// 解析GPS数据
QString Map::parseGpsData(const QString &data)
{
    auto list = data.trimmed().split(',');
    // $GPGGA 至少要有到定位质量/经纬度的字段
    if (list.count() < 9)
        return QString();
    // 判断是否为 GGA 定位语句（支持 $GPGGA / $GNGGA / $BDGGA 等）
    if (!list.at(0).contains("GGA"))
        return QString();
    // 定位质量为 0 表示无效
    if (list.at(6).toInt() == 0)
        return QString();

    bool ok = false;
    double lon = list.at(4).toDouble(&ok); // 经度 ddmm.mmmm
    if (!ok)
        return QString();
    double lat = list.at(2).toDouble(&ok); // 纬度 ddmm.mmmm
    if (!ok)
        return QString();

    lon = toDecimalDegree(lon);
    lat = toDecimalDegree(lat);
    if (list.at(5).compare("W", Qt::CaseInsensitive) == 0)
        lon = -lon;
    if (list.at(7).compare("S", Qt::CaseInsensitive) == 0)
        lat = -lat;

    return QString::number(lon, 'f', 6) + ',' + QString::number(lat, 'f', 6);
}
