#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMovie>
#include <QSerialPort>
#include <QByteArray>
#include <QPushButton>
#include <QString>
#include <QTimer>

class Map : public QDialog
{
    Q_OBJECT

    // 天地图 Key，在 https://console.tianditu.gov.cn 申请（百度 AK 已停用，改用天地图）
    const QString tdtToken = "83beac5e0b4fe9d015773d84adc0aaaa";
    // 天地图静态图接口：%1 = tk，%2 = "经度,纬度"（同时作为中心点与标记点）
    // markers 由服务端画出定位针；若板端 Qt 无 OpenSSL，会降级成 http 请求
    const QString tdtStaticUrl =
        "https://api.tianditu.gov.cn/staticimage?tk=%1&center=%2&width=1024&height=600"
        "&zoom=16&markers=%2";

public:
    explicit Map(QWidget *parent = nullptr);
    ~Map();

private:
    void initUi();
    void initCtrl();
    void initSerialPort();
    void requestMapImage(const QString &center);
    void startRequest(const QString &url);
    void showMap(const QPixmap &pixmap, const QString &note);
    void showFallback(const QString &reason);
    void setStatusText();
    QString parseGpsData(const QString &data);

private slots:
    void onReadyRead();
    void onFinished();
    void portReadReady();
    void on_back_clicked();
public slots:
    void map_working();

signals:
    void backout();

private:
    QLabel m_iconLabel;
    QLabel m_logoLabel;
    QPushButton* m_back;
    QByteArray m_array;
    QNetworkAccessManager m_networkAccessManager;
    QNetworkReply *m_reply = nullptr;
    QTimer m_timeout;
    // 请求失败时由超时等逻辑填入，用于给用户看具体原因
    QString m_errorText;
    QString m_cachePath;
    qint64 m_lastRequestMs = 0;
    bool m_gpsWarned = false;
    bool m_portConnected = false;
    bool m_hasFix = false;
    bool m_noGpsDevice = false;
    // 默认定位（室内收不到卫星时使用）：上海市浦东新区周浦镇 公元新村
    // 天地图用 CGCS2000（≈WGS84），故直接使用 WGS84 坐标，无需 GCJ-02/BD-09 转换
    QString m_location = QString("121.5722919,31.1241443");
    const QString m_defaultPlace = QStringLiteral("上海市浦东新区公元新村");
    QSerialPort m_port;
};

#endif // MAPWIDGET_H
