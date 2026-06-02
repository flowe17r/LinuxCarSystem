#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "player.h"
#include "Audio.h"
#include "weather.h"

#include <QStackedLayout>
#include <QThread>
#include <QTimer>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // 创建线程对象
    t1 = new QThread;
    t2 = new QThread;
    t3 = new QThread;
    t4 = new QThread;
    t5 = new QThread;
    // 创建工作对象
    weather = new Weather;
    player = new Player;
    audio = new Audio;
    hardware = new Hardware;
    map = new Map;
    // 将工作对象移入线程中
    weather->moveToThread(t1);
    player->moveToThread(t2);
    audio->moveToThread(t3);
    hardware->moveToThread(t4);
    map->moveToThread(t5);
    // 当信号被触发执行相应的工作函数
    connect(this,&MainWindow::weather_starting,weather,&Weather::weather_working);
    connect(this,&MainWindow::player_starting,player,&Player::player_working);
    connect(this,&MainWindow::audio_starting,audio,&Audio::audio_working);
    connect(this,&MainWindow::hardware_starting,hardware,&Hardware::hardware_working);
    connect(this,&MainWindow::map_starting,map,&Map::map_working);
    // 当按钮被按下后触发信号，启动线程
    connect(ui->pushButton_3,&QPushButton::clicked,this,[=](){
       emit weather_starting();
       t1->start();
       weather->show();
       this->hide();
    });
    connect(ui->pushButton,&QPushButton::clicked,this,[=](){
        emit player_starting();
        t2->start();
        player->show();
        this->hide();
    });
    connect(ui->pushButton_2,&QPushButton::clicked,this,[=](){
       emit audio_starting();
       t3->start();
       audio->show();
       this->hide();
    });
    connect(ui->pushButton_4,&QPushButton::clicked,this,[=](){
       emit hardware_starting();
       t4->start();
       hardware->show();
       this->hide();
    });
    connect(ui->pushButton_5,&QPushButton::clicked,this,[=](){
       emit map_starting();
       t5->start();
       map->show();
       this->hide();
    });

    // 当返回按钮被按下后，终止线程
    connect(this->weather,&Weather::back,[=](){
        t1->terminate();
        this->weather->hide();
        this->show();
    });
    connect(this->player,&Player::back,[=](){
        t2->terminate();
        player->clearConnections();
        this->player->hide();
        this->show();
    });
    connect(this->audio,&Audio::back,[=](){
        t3->terminate();
        this->audio->hide();
        this->show();
    });
    connect(this->hardware,&Hardware::back1,[=](){
        t4->terminate();
        this->hardware->hide();
        this->show();
    });
    connect(this->map,&Map::backout,[=](){
        t5->terminate();
        this->map->hide();
        this->show();
    });
    // 获取当前的日期时间
    QDateTime *datatime=new QDateTime(QDateTime::currentDateTime());
    QString str = datatime->toString("    yyyy年MM月dd日 ddd"); //设置显示格式
    ui->datelable->setText(str); // 设置显示
    // 获取当前时间
    QTime currentTime = QTime::currentTime();
    ui->timelable->setText(QString(currentTime.toString()));
    // 设置定时器 每一秒触发并重新设置时间
    QTimer *myTimer = new QTimer(this);
    myTimer->start(1000);
    //每次定时器触发一次，就更改文本框的内容
    // connect(myTimer,&QTimer::timeout,this,[=](){
    //     QTime currentTime = QTime::currentTime();
    //     ui->timelable->setText(QString(currentTime.toString()));
    // });
    connect(myTimer,&QTimer::timeout,this,[=](){
        QDateTime now = QDateTime::currentDateTime();
        ui->datelable->setText(now.toString("    yyyy年MM月dd日 ddd"));
        ui->timelable->setText(now.toString("hh:mm:ss"));
    });

}

MainWindow::~MainWindow()
{
    // 销毁线程
    t1->quit();
    t1->exit();
    t2->quit();
    t2->exit();
    t3->quit();
    t3->exit();
    t4->quit();
    t4->exit();
    t5->quit();
    t5->exit();
    delete ui;
}


