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
#include <QPixmap>

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

    // QLabel 接管 QMovie 生命周期（setPixmap/clear 时会销毁它），故不持有裸指针
    auto pMovie = new QMovie(QStringLiteral(":/images/loading.gif"));
    m_iconLabel.setMovie(pMovie);
    m_iconLabel.setAlignment(Qt::AlignCenter);
    m_logoLabel.setAlignment(Qt::AlignRight);

    auto *pVLayout = new QVBoxLayout;
    pVLayout->addStretch();
    pVLayout->addWidget(&m_logoLabel);
    pVLayout->addWidget(m_back);
    m_iconLabel.setLayout(pVLayout);
    connect(m_back, &QPushButton::clicked, this, &Map::on_back_clicked);

    auto *pMainLayout = new QVBoxLayout;
    pMainLayout->addWidget(&m_iconLabel);
    pMainLayout->setMargin(0);
    setLayout(pMainLayout);
    setObjectName("map_widget");
    CommonHelper::setStyleSheet(":/style/default.qss", this);
}

void Map::initCtrl()
{
    initSerialPort();
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
    QString url = tdtStaticUrl.arg(tdtToken).arg(center);

    // 板端 Qt 若没编进 OpenSSL，https 会直接以 "Protocol not available" 失败，
    // 表现为一直卡在 loading；此时降级为 http（天地图两种协议均可用）
    if (!QSslSocket::supportsSsl())
        url.replace("https://", "http://");

    startRequest(url);
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
    if (url.startsWith("https")) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        // 板端无 CA 证书 / 系统时间不对时避免握手直接失败
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
        networkRequest.setSslConfiguration(config);
    }
    networkRequest.setUrl(url);

    qDebug() << "[Map] request:" << url;
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
    reply->deleteLater();

    QPixmap pixmap;
    if (err == QNetworkReply::NoError)
        pixmap.loadFromData(m_array);

    if (err != QNetworkReply::NoError || pixmap.isNull()) {
        QString reason = m_errorText;
        m_errorText.clear();
        if (reason.isEmpty()) {
            if (err != QNetworkReply::NoError) {
                if (err == QNetworkReply::SslHandshakeFailedError)
                    reason = QStringLiteral("HTTPS 握手失败（证书/系统时间问题）");
                else if (err == QNetworkReply::ProtocolUnknownError
                         || err == QNetworkReply::ContentNotAvailableError
                         || errString.contains("ssl", Qt::CaseInsensitive))
                    reason = QStringLiteral("Qt 不支持 HTTPS（缺少 OpenSSL 运行库）");
                else
                    reason = errString;
            } else {
                reason = QStringLiteral("返回数据无法解码（缺少 PNG 图像插件）");
            }
        }
        qDebug() << "[Map] failed:" << int(err) << errString;
        showFallback(reason);
        return;
    }

    // 服务端已按 markers 参数在中心画出定位针
    QFile file(m_cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(m_array);
        file.close();
    }
    showMap(pixmap, QString());
}

void Map::showMap(const QPixmap &pixmap, const QString &note)
{
    m_iconLabel.setPixmap(pixmap);
    if (!note.isEmpty())
        m_logoLabel.setText(note);
    else
        setStatusText();
}

// 网络失败时的兜底：优先显示上次成功缓存的地图，否则给出明确原因，绝不卡在 loading
void Map::showFallback(const QString &reason)
{
    QPixmap cached;
    if (cached.load(m_cachePath)) {
        QString note = QStringLiteral("网络异常：%1（显示上次地图）").arg(reason);
        showMap(cached, note);
        return;
    }

    m_iconLabel.clear();  // 销毁 loading 动画
    m_iconLabel.setText(QStringLiteral("地图加载失败：%1").arg(reason));
    m_logoLabel.setText(QStringLiteral("%1  [%2]").arg(m_defaultPlace).arg(m_location));
}

void Map::setStatusText()
{
    QString text;
    if (m_hasFix)
        text = QStringLiteral("定位成功：") + m_location;
    else if (m_noGpsDevice)
        text = QStringLiteral("未检测到GPS设备，展示默认位置：") + m_defaultPlace
               + QStringLiteral("  [") + m_location + QStringLiteral("]");
    else
        text = QStringLiteral("GPS定位中，移动至空旷处；当前展示默认位置：") + m_defaultPlace
               + QStringLiteral("  [") + m_location + QStringLiteral("]");
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
        const bool moved = (location != m_location);
        m_location = location;
        setStatusText();

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
