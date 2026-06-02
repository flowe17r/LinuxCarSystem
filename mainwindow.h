#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "player.h"
#include "Audio.h"
#include "weather.h"
#include "hardware.h"
#include "map.h"
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
    void player_starting();
    void audio_starting();
    void hardware_starting();
    void map_starting();


private:
    Ui::MainWindow *ui;
    Player* player;
    Audio* audio;
    Weather* weather;
    Hardware* hardware;
    Map* map;
    QThread* t1;
    QThread* t2;
    QThread* t3;
    QThread* t4;
    QThread* t5;
    QTimer *m_timer;
    QLabel* mylabel;
    QPushButton* m_button;
};
#endif // MAINWINDOW_H
