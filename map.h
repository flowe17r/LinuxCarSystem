#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMovie>
#include <QPoint>
#include <QSerialPort>
#include <QByteArray>
#include <QPushButton>
#include <QString>
#include <QTimer>

class QMouseEvent;

class Map : public QDialog
{
    Q_OBJECT

    // 天地图 Key，在 https://console.tianditu.gov.cn 申请（百度 AK 已停用，改用天地图）
    const QString tdtToken = "83beac5e0b4fe9d015773d84adc0aaaa";
    // 天地图静态图接口：%1 = tk，%2 = 视图中心"经度,纬度"，%3 = zoom，%4 = 定位标记点
    // markers 传定位点而非中心点：平移后红针仍留在用户所在位置；
    // 若板端 Qt 无 OpenSSL，会降级成 http 请求
    const QString tdtStaticUrl =
        "https://api.tianditu.gov.cn/staticimage?tk=%1&center=%2&width=1024&height=600"
        "&zoom=%3&markers=%4";

public:
    explicit Map(QWidget *parent = nullptr);
    ~Map();

private:
    void initUi();
    void initCtrl();
    void initSerialPort();
    void requestMapImage(const QString &center);
    void startRequest(const QString &url);
    void retryWithHttp();
    QString buildUrl(const QString &center, bool https) const;
    void panBy(const QPoint &pixelDelta);
    void zoomBy(int step);
    void recenter();
    QString shortErrorText(QNetworkReply::NetworkError err) const;
    void showMap(const QPixmap &pixmap, const QString &note);
    void showFallback(const QString &reason);
    void setStatusText();
    QString parseGpsData(const QString &data);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

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
    QPushButton* m_zoomIn;
    QPushButton* m_zoomOut;
    QPushButton* m_recenter;
    QLabel* m_busyLabel;
    QByteArray m_array;
    QNetworkAccessManager m_networkAccessManager;
    QNetworkReply *m_reply = nullptr;
    QTimer m_timeout;
    // 请求失败时由超时等逻辑填入，用于给用户看具体原因
    QString m_errorText;
    QString m_cachePath;
    qint64 m_lastRequestMs = 0;
    bool m_httpRetried = false;
    bool m_lastUrlWasHttps = false;
    // 交互状态：zoom 级别、是否被用户拖动/缩放偏离定位点、拖动起点
    int m_zoom = 16;
    bool m_userMoved = false;
    bool m_dragging = false;
    QPoint m_dragStart;
    // 最近一次 GPS 定位（或默认位置），用于"回定位"
    QString m_fixLocation = QString("121.5722919,31.1241443");
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
