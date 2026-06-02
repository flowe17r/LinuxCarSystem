#ifndef Audio_H
#define Audio_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMediaMetaData>
#include <QFileInfoList>
#include <QDir>
#include <QPainter>
#include <QListWidgetItem>
#include "rotatablewidget.h"

namespace Ui {
class Audio;
}

/* 媒体信息结构体 */
struct MediaObjectInfo {
    // 用于保存歌曲文件名
    QString fileName;
    // 用于保存歌曲文件路径
    QString filePath;
    // 歌词文件路径
    QString lyricsPath;
};

class Audio : public QWidget
{
    Q_OBJECT

public:
    explicit Audio(QWidget *parent = 0);
    ~Audio();

    void audio_working();
    void clearConnections();
signals:
    void back();

private slots:
    // 播放
    void on_btn_play_clicked(bool checked);
    // 上一曲
    void on_btn_pre_clicked();
    // 下一曲
    void on_btn_next_clicked();
    // 循环播放
    void on_btn_loop_clicked(bool checked);
    /* 媒体状态改变 */
    void mediaPlayerStateChanged(QMediaPlayer::State);
    /* 媒体列表项改变 */
    void mediaPlaylistCurrentIndexChanged(int);
    /* 媒体总长度改变 */
    void musicPlayerDurationChanged(qint64);
    /* 媒体播放位置改变 */
    void mediaPlayerPositionChanged(qint64);

    void onMetaDataAvailableChanged(bool available);
    // 歌词更新
    void updateLyrics(qint64 currentTime);
    // 列表项被点击
    void on_listWidget_itemClicked(QListWidgetItem *item);
    // 水平滑块
    void on_horizontalSlider_sliderReleased();

    void on_pushButton_clicked();
    // 垂直滑块
    void on_verticalSlider_sliderReleased();

private:
    Ui::Audio *ui;
//    QPixmap rotatingPixmap;
    RotatableWidget *rotatableWidget;
    QWidget *baseWidget;
//    QPointF rotationCenter;
    QPropertyAnimation *animation;

    RotatableWidget *rotatableWidget2;
    QWidget *baseWidget2;
    QPropertyAnimation *animation2;

    RotatableWidget *rotatableWidget3 = nullptr;
    QWidget *baseWidget3;
    QPropertyAnimation *animation3 = nullptr;

    /* 媒体播放器，用于播放音乐 */
    QMediaPlayer *musicPlayer;
    /* 媒体列表 */
    QMediaPlaylist *mediaPlaylist;
    /* 媒体信息存储 */
    QVector<MediaObjectInfo> mediaObjectInfo;

    QString currentLyricsPath;

    /* 扫描歌曲 */
    void scanSongs();
    /* 媒体播放器类初始化 */
    void mediaPlayerInit();

    void setAlbumArt(QPixmap pixmap);
};

#endif // WIDGET_H
