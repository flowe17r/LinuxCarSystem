#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "speedgauge.h"

#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QToolButton>

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

    t1 = new QThread(this);
    t2 = new QThread(this);
    t3 = new QThread(this);

    weather = new Weather;
    player = new Player;
    audio = new Audio;
    hardware = new Hardware;
    map = new Map;

    weather->moveToThread(t1);
    player->moveToThread(t2);
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
        t2->start();
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
    t2->quit();
    t2->wait(500);
    t3->quit();
    t3->wait(500);
    delete ui;
}
