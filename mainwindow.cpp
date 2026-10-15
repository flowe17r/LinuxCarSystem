#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "speedgauge.h"

#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , camera(nullptr)
    , settingsPage(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    menuBar()->hide();
    statusBar()->hide();
    setFixedSize(1024, 600);

    const QList<QToolButton *> navBtns = {
        ui->pushButton, ui->pushButton_2, ui->pushButton_3,
        ui->pushButton_4, ui->pushButton_5, ui->btnSettings, ui->btnCamera
    };
    for (QToolButton *btn : navBtns) {
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setCursor(Qt::PointingHandCursor);
    }

    ui->speedGauge->setSpeed(85);
    // 预留最宽文本（180 km/h）的宽度：否则 99→100 时标签变宽会挤压布局，
    // 仪表盘随 side 变化整体缩放，拖动时出现"边缘变大变小"的跳动
    ui->labelSpeed->setMinimumWidth(
        ui->labelSpeed->fontMetrics().horizontalAdvance(QStringLiteral("180 km/h")));
    connect(ui->speedGauge, &SpeedGauge::speedChanged, this, [this](int kmh) {
        ui->labelSpeed->setText(QStringLiteral("%1 km/h").arg(kmh));
    });

    t1 = new QThread(this);
    t3 = new QThread(this);

    weather = new Weather;
    player = new Player;
    audio = new Audio;
    hardware = new Hardware;
    map = new Map;

    weather->moveToThread(t1);
    audio->moveToThread(t3);

    connect(this, &MainWindow::weather_starting, weather, &Weather::weather_working);
    connect(this, &MainWindow::audio_starting, audio, &Audio::audio_working);
    connect(this, &MainWindow::hardware_starting, hardware, &Hardware::hardware_working);
    connect(this, &MainWindow::map_starting, map, &Map::map_working);

    connect(ui->pushButton_3, &QToolButton::clicked, this, [=]() {
        emit weather_starting();
        t1->start();
        openModule(weather);
    });
    connect(ui->pushButton, &QToolButton::clicked, this, [=]() {
        openModule(player);
    });
    connect(ui->pushButton_2, &QToolButton::clicked, this, [=]() {
        emit audio_starting();
        t3->start();
        openModule(audio);
    });
    connect(ui->pushButton_4, &QToolButton::clicked, this, [=]() {
        emit hardware_starting();
        openModule(hardware);
    });
    connect(ui->pushButton_5, &QToolButton::clicked, this, [=]() {
        emit map_starting();
        openModule(map);
    });
    connect(ui->btnSettings, &QToolButton::clicked, this, [this]() {
        if (!settingsPage) {
            settingsPage = new SettingsPage;
            connect(settingsPage, &SettingsPage::back, this, [this]() {
                settingsPage->hide();
                backToHome();
            });
        }
        openModule(settingsPage);
    });
    connect(ui->btnCamera, &QToolButton::clicked, this, [this]() {
        if (!camera) {
            camera = new Camera;
            connect(camera, &Camera::back, this, [this]() {
                camera->hide();
                backToHome();
            });
        }
        hide();
        camera->camera_working();
    });

    connect(weather, &Weather::back, this, [=]() {
        t1->quit();
        weather->hide();
        backToHome();
    });
    connect(player, &Player::back, this, [=]() {
        player->hide();
        backToHome();
    });
    connect(audio, &Audio::back, this, [=]() {
        t3->quit();
        audio->hide();
        backToHome();
    });
    connect(hardware, &Hardware::back1, this, [=]() {
        hardware->hide();
        backToHome();
    });
    connect(map, &Map::backout, this, [=]() {
        map->hide();
        backToHome();
    });

    auto *clock = new QTimer(this);
    clock->setInterval(1000);
    connect(clock, &QTimer::timeout, this, [this]() {
        const QDateTime now = QDateTime::currentDateTime();
        ui->datelable->setText(now.toString(QStringLiteral("yyyy年MM月dd日 ddd")));
        ui->timelable->setText(now.toString(QStringLiteral("hh:mm:ss")));
    });
    clock->start();
    ui->datelable->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy年MM月dd日 ddd")));
    ui->timelable->setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")));

    m_netMgr = new QNetworkAccessManager(this);
    checkNetwork();
    auto *netCheck = new QTimer(this);
    netCheck->setInterval(30000);
    connect(netCheck, &QTimer::timeout, this, &MainWindow::checkNetwork);
    netCheck->start();
}

// 真实探测外网：能收到任何 HTTP 响应（含 403）即视为已连接，超时/无路由为未连接
void MainWindow::checkNetwork()
{
    QNetworkRequest req{QUrl(QStringLiteral("http://www.baidu.com"))};
    req.setRawHeader("User-Agent", "SmartCarIVI/1.0");
    QNetworkReply *reply = m_netMgr->get(req);

    auto *abortTimer = new QTimer(this);
    abortTimer->setSingleShot(true);
    connect(abortTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
    abortTimer->start(5000);

    connect(reply, &QNetworkReply::finished, this, [this, reply, abortTimer]() {
        abortTimer->stop();
        abortTimer->deleteLater();
        const bool replyFinished = reply->isFinished();
        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool connected = reply->error() == QNetworkReply::NoError
            || (replyFinished && httpStatus > 0);
        reply->deleteLater();

        ui->titleNet->setText(connected ? QStringLiteral("网络：已连接")
                                        : QStringLiteral("网络：未连接"));
        ui->titleNet->setStyleSheet(connected ? QStringLiteral("color:#4ade80; font-size:12px;")
                                              : QStringLiteral("color:#f87171; font-size:12px;"));
    });
}

void MainWindow::openModule(QWidget *page)
{
    page->show();
    page->raise();
    hide();
}

void MainWindow::backToHome()
{
    show();
    raise();
}

MainWindow::~MainWindow()
{
    if (camera)
        delete camera;
    if (settingsPage)
        delete settingsPage;
    t1->quit();
    t1->wait(500);
    t3->quit();
    t3->wait(500);
    delete ui;
}
