#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "player.h"
#include "Audio.h"
#include "weather.h"
#include "hardware.h"
#include "map.h"
#include "camera.h"
#include "settingspage.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void weather_starting();
    void audio_starting();
    void hardware_starting();
    void map_starting();

private:
    void openModule(QWidget *page);
    void backToHome();

    Ui::MainWindow *ui;
    Player *player;
    Audio *audio;
    Weather *weather;
    Hardware *hardware;
    Map *map;
    Camera *camera;
    SettingsPage *settingsPage;
    QThread *t1;
    QThread *t2;
    QThread *t3;
};

#endif // MAINWINDOW_H
