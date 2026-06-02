#include "Audio.h"
#include "ui_Audio.h"

Audio::Audio(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Audio)
{
    ui->setupUi(this);
    /* 音臂 */
    // 加载旋转图片
    QPixmap rotatingPixmap(":/images/styli.png");
    // 指定控件并初始化坐标用作旋转中心点
    baseWidget = ui->widget_2;
    QPointF rotationCenter(34, 54);
    // 创建RotatableWidget对象，将图片和旋转中心点传递给构造函数
    RotatableWidget *rotatableWidget = new RotatableWidget(baseWidget, rotatingPixmap, rotationCenter);
    rotatableWidget->resize(baseWidget->size()); // 确保RotatableWidget的大小与baseWidget相同
    // 创建动画来旋转 rotatableWidget
    animation = new QPropertyAnimation(rotatableWidget, "rotation");
    animation->setDuration(200); // 动画持续200毫秒

    /* 唱片*/
    // 设置专辑封面
    setAlbumArt(QPixmap(":/images/fengshen.jpg"));

    QPixmap tmpPixmap(":/images/disc.png");
    QPixmap rotatingPixmap2 = tmpPixmap.scaled(240, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    baseWidget2 = ui->widget_3;
    QPointF rotationCenter2(baseWidget2->width()/2, baseWidget2->height()/2);
    RotatableWidget *rotatableWidget2 = new RotatableWidget(baseWidget2, rotatingPixmap2, rotationCenter2);
    rotatableWidget2->resize(baseWidget2->size()); // 确保RotatableWidget的大小与baseWidget相同
    // 创建动画来旋转 rotatableWidget
    animation2 = new QPropertyAnimation(rotatableWidget2, "rotation");
    animation2->setStartValue(0); // 开始角度为0
    animation2->setEndValue(360); // 结束角度为360
    animation2->setDuration(4000); // 动画持续4000毫秒
    animation2->setLoopCount(-1); // 设置动画无限循环


    /* 媒体播放器初始化 */
    mediaPlayerInit();
    /* 扫描歌曲 */
    scanSongs();

//    ui->listWidget->setStyleSheet(R"(
//        QListWidget::verticalScrollBar {
//            border:none;
//            background:transparent;
//        }
//        QListWidget::item {
//            color:white;
//            height:60px;
//        }
//        QListWidget::verticalScrollBar:handle {
//            border: none;
//            background-color:#aaaaaa;
//            width: 20px; /* 设置滚动条宽度 */
//        }
//        QListWidget::verticalScrollBar:add-page,
//        QListWidget::verticalScrollBar:sub-page {
//            height: 0px; /* 隐藏按钮 */
//            border: none;
//            background: none;
//        }
//    )");






}
void Audio::audio_working()
{
    /* 媒体信号槽连接 */
    connect(musicPlayer,
            SIGNAL(stateChanged(QMediaPlayer::State)),
            this,
            SLOT(mediaPlayerStateChanged(QMediaPlayer::State)));
    connect(mediaPlaylist,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(mediaPlaylistCurrentIndexChanged(int)));
    connect(musicPlayer, SIGNAL(durationChanged(qint64)),
            this,
            SLOT(musicPlayerDurationChanged(qint64)));
    connect(musicPlayer,
            SIGNAL(positionChanged(qint64)),
            this,
            SLOT(mediaPlayerPositionChanged(qint64)));
    connect(musicPlayer, &QMediaPlayer::metaDataAvailableChanged,
            this, &Audio::onMetaDataAvailableChanged);

    // 在播放器的播放进度更新的槽函数中调用updateLyrics()
    connect(musicPlayer, &QMediaPlayer::positionChanged,
            this, &Audio::updateLyrics);
    //
    connect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged,
            this, &Audio::mediaPlaylistCurrentIndexChanged);
    /* 列表信号槽连接 */
    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(listWidgetCliked(QListWidgetItem*)));

    /* slider信号槽连接 */
    connect(ui->horizontalSlider, SIGNAL(sliderReleased()),
            this, SLOT(durationSliderReleased()));
}
void Audio::clearConnections()
{
    // 断开音乐播放器状态改变信号的连接
    disconnect(musicPlayer,
               SIGNAL(stateChanged(QMediaPlayer::State)),
               this,
               SLOT(mediaPlayerStateChanged(QMediaPlayer::State)));

    // 断开媒体播放列表当前索引改变信号的连接
    disconnect(mediaPlaylist,
               SIGNAL(currentIndexChanged(int)),
               this,
               SLOT(mediaPlaylistCurrentIndexChanged(int)));

    // 断开音乐播放器时长改变信号的连接
    disconnect(musicPlayer, SIGNAL(durationChanged(qint64)),
               this,
               SLOT(musicPlayerDurationChanged(qint64)));

    // 断开音乐播放器播放位置改变信号的连接
    disconnect(musicPlayer,
               SIGNAL(positionChanged(qint64)),
               this,
               SLOT(mediaPlayerPositionChanged(qint64)));

    // 断开音乐播放器元数据可用改变信号的连接
    disconnect(musicPlayer, &QMediaPlayer::metaDataAvailableChanged,
               this, &Audio::onMetaDataAvailableChanged);

    // 断开音乐播放器播放位置改变信号与更新歌词槽函数的连接
    disconnect(musicPlayer, &QMediaPlayer::positionChanged,
               this, &Audio::updateLyrics);

    // 断开媒体播放列表当前索引改变信号与另一个同名槽函数的连接（如果有重复连接情况需要注意）
    disconnect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged,
               this, &Audio::mediaPlaylistCurrentIndexChanged);

    // 断开列表部件的项目点击信号的连接
    disconnect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)),
               this, SLOT(listWidgetCliked(QListWidgetItem*)));

    // 断开水平滑块的滑块释放信号的连接
    disconnect(ui->horizontalSlider, SIGNAL(sliderReleased()),
               this, SLOT(durationSliderReleased()));
}
Audio::~Audio()
{
    delete ui;
}


// 扫描歌曲目录，将歌曲添加到媒体列表中，并为有歌词的歌曲保存歌词路径
void Audio::scanSongs()
{
    // 获取当前程序路径下的myMusic目录
    QDir dir(QCoreApplication::applicationDirPath()
             + "/myMusic");

    // 获取歌曲目录绝对路径
    QDir dirbsolutePath(dir.absolutePath());
    qDebug() << dirbsolutePath.path() <<endl;

    /* 如果目录存在 */
    if (dirbsolutePath.exists()) {
        /* 定义过滤器 */
        QStringList filter;
        /* 获取所有以.mp3结尾的文件 */
        filter << "*.mp3";
        /* 获取符合条件文件的信息 */
        QFileInfoList files =
                dirbsolutePath.entryInfoList(filter, QDir::Files);

        // 歌词映射：歌曲文件名 -> 歌词文件完整路径
        QHash<QString, QString> songToLyricsMap;
        // 添加歌词文件的搜索
        filter.clear(); // 清空过滤器
        filter << "*.lrc";
        QFileInfoList lrcFiles = dirbsolutePath.entryInfoList(filter, QDir::Files);
        // 将去除扩展名的文件以及文件绝对路径插入songToLyricsMap
        for (const QFileInfo &lrcFileInfo : lrcFiles) {
            QString baseName = lrcFileInfo.completeBaseName(); // 完整的基础文件名，不含扩展名
            songToLyricsMap.insert(baseName, lrcFileInfo.absoluteFilePath());
        }

        /* 遍历 */
        for (int i = 0; i < files.count(); i++) {
            MediaObjectInfo info;
            QString baseName = files.at(i).completeBaseName(); // 去除拓展名
            QString fileName = baseName;
            // 获取文件名的第二个字符串并拼接 info.fileName: "阿拉斯加海湾 - 蓝心羽\n 蓝心羽"
            info.fileName = fileName + "\n"
                    + fileName.split("-").at(1);
            // 获取对应索引下的文件的绝对路径
            info.filePath = QString::fromUtf8(files.at(i)
                                              .filePath()
                                              .toUtf8()
                                              .data());

            // 获取歌词路径
            QString lyricsPath = songToLyricsMap.value(baseName);
            /* 媒体列表添加歌曲 */
            if (mediaPlaylist->addMedia(
                        QUrl::fromLocalFile(info.filePath))) {
                // 如果歌词文件存在，则保存其路径
                if (!lyricsPath.isEmpty()) {
                    info.lyricsPath = lyricsPath;
                }
                /* 添加到容器数组里储存 */
                mediaObjectInfo.append(info);
                /* 添加歌曲名字至列表 */
                ui->listWidget->addItem(info.fileName);
            } else {
                qDebug()<<
                           mediaPlaylist->errorString()
                           .toUtf8().data()
                        << endl;
                qDebug()<< "  Error number:"
                         << mediaPlaylist->error()
                         << endl;
            }
        }
    }
    else
        qDebug() << "dir is no exists" <<endl;

}
// 根据播放进度更新歌词显示
void Audio::updateLyrics(qint64 currentTime) {
    if (!currentLyricsPath.isEmpty()) {
        QFile file(currentLyricsPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) { // 以只读方式打开文本
            QTextStream stream(&file);

            // PC端使用不会乱码，开发板则去掉
            stream.setCodec("UTF-8");

            while (!stream.atEnd()) {
                QString line = stream.readLine();
                // 解析每一句歌词的时间
                // 歌词格式通常是如下形式：[00:12.00]歌词内容
                QRegExp timeRegex("\\[(\\d+):(\\d+\\.\\d+)\\]");
                if (timeRegex.indexIn(line) != -1) {
                    // 将分钟数和秒数相加得到歌词播放时间，精确到小数点后两位
                    qint64 startTime = timeRegex.cap(1).toInt() * 60000 + // 提取分钟数
                                       timeRegex.cap(2).toFloat() * 1000; // 提取秒数
                    QString currentLyric = line.remove(timeRegex);
                    // 开始时间小于等于现在播放时间
                    if (startTime <= currentTime) {
                        ui->label_4->setText(currentLyric); // 显示
                    } else {
                        break;
                    }
                }
            }
        }
    }
    else
        qDebug() << "currentLyricsPath isEmpty " << endl;
}
// 初始化媒体播放器
void Audio::mediaPlayerInit()
{
    musicPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);
    /* 确保列表是空的 */
    mediaPlaylist->clear();
    /* 设置音乐播放器的列表为mediaPlaylist */
    musicPlayer->setPlaylist(mediaPlaylist);
    /* 设置播放模式，Loop是列循环 */
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
}

// 设置专辑封面
void Audio::setAlbumArt(QPixmap pixmap)
{
    int cornerRadius = 86;  // 圆角大小
    // 图像缩放 将输入的pixmap缩放到 172 * 172 大小，同时保持宽高比，并使用平滑变换
    QPixmap Pixmap2 = pixmap.scaled(172, 172, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap roundedPixmap(Pixmap2.size()); // 创建一个跟Pixmap2等大的对象
    roundedPixmap.fill(Qt::transparent); // 颜色填充为透明
    // 将roundedPixmap传入painter对象 以后的操作都是对roundedPixmap操作
    QPainter painter(&roundedPixmap);
    // 开启抗锯齿和平滑图像变换
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(Pixmap2));
    painter.setPen(Qt::NoPen); // 不绘制边框
    QPainterPath path; // 定义绘制路径
    // 将Pixmap2.rect()绘制成带有圆角的矩形区域添加到path中
    path.addRoundedRect(Pixmap2.rect(), cornerRadius, cornerRadius);
    painter.setClipPath(path); // 设置裁剪路径
    painter.drawPixmap(Pixmap2.rect(), Pixmap2); // 将 Pixmap2 按照 Pixmap2.rect() 所确定的位置和大小进行绘制到 roundedPixmap
    painter.end();
    // 获取当前程序的路径
    QDir dir(QCoreApplication::applicationDirPath());
    QString filePath = dir.filePath("pixmapTmp.jpg");  // 正确拼接文件的完整路径
    bool saved = roundedPixmap.save(filePath);
    // 设置widget属性
    if(saved) {
        ui->widget->setStyleSheet("QWidget#widget{\n"
                                  "border:none;\n"
                                  "border-radius:120px;\n"
                                  "image: url(" + filePath + ");}");
    }
    else {
        ui->widget->setStyleSheet("QWidget#widget{\n"
                                  "border:none;\n"
                                  "border-radius:120px;\n"
                                  "image: url(:/images/fengshen.jpg);}");
    }
}
// 播放/暂停按钮点击事件处理
void Audio::on_btn_play_clicked(bool checked)
{
    if(checked) {
        animation->setStartValue(0); // 开始角度
        animation->setEndValue(30); // 结束角度
        animation->start(); // 开始动画
        animation2->start(); // 开始动画
        if(animation3 != nullptr)
            animation3->start();
    }
    else {
        animation->setStartValue(30); // 开始角度为30
        animation->setEndValue(0); // 结束角度为0
        //    animation->setLoopCount(-1); // 设置动画无限循环
        animation->start(); // 开始动画
        animation2->stop(); // 停止动画(disc)
        if(animation3 != nullptr)
            animation3->stop();
    }

    int state = musicPlayer->state();

    switch (state) {
    case QMediaPlayer::StoppedState:
        /* 媒体播放 */
        musicPlayer->play();
        break;
    case QMediaPlayer::PlayingState:
        /* 媒体暂停 */
        musicPlayer->pause();
        break;
    case QMediaPlayer::PausedState:
        musicPlayer->play();
        break;
    }
}
// 上一曲按钮点击事件处理
void Audio::on_btn_pre_clicked()
{
    musicPlayer->stop();
    int count = mediaPlaylist->mediaCount();
    if (0 == count)
        return;

    /* 列表上一个 */
    mediaPlaylist->previous();
//    musicPlayer->play();
    emit ui->btn_play->clicked(true);

}
// 下一曲按钮点击事件处理
void Audio::on_btn_next_clicked()
{
    musicPlayer->stop();
    int count = mediaPlaylist->mediaCount();
    if (0 == count) {
        qDebug() << "mediaPlaylist->mediaCount = 0" << endl;
        return;
    }

    /* 列表下一个 */
    mediaPlaylist->next();
//    musicPlayer->play();
    emit ui->btn_play->clicked(true);

}
// 循环模式按钮点击事件处理
void Audio::on_btn_loop_clicked(bool checked)
{
    if(checked) {
        // 当前项循环
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    }
    else {
        // 列表循环
        mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    }
}
// 媒体播放器状态改变时的槽函数
void Audio::mediaPlayerStateChanged(QMediaPlayer::State state)
{
    //click和check相互独立
    //click用于实现槽函数；check用于改变图片
    switch (state) {
    case QMediaPlayer::StoppedState: // 停止
        ui->btn_play->setChecked(false);
        break;

    case QMediaPlayer::PlayingState: // 播放
        ui->btn_play->setChecked(true);
        break;

    case QMediaPlayer::PausedState: // 暂停
        ui->btn_play->setChecked(false);
        break;
    }
}
// 媒体播放列表当前索引改变时的槽函数
void Audio::mediaPlaylistCurrentIndexChanged(int index)
{
    if (-1 == index)
        return;

    /* 设置列表正在播放的项 */
    ui->listWidget->setCurrentRow(index);

    // 检查索引有效性
    if (index >= 0 && index < mediaObjectInfo.size()) {
        // 更新当前歌词路径
        currentLyricsPath = mediaObjectInfo.at(index).lyricsPath;
        qDebug() << currentLyricsPath << endl;
        // 如果需要，你可以在这里调用的函数
//        updateLyrics();
    } else {
        // 如果索引无效，清除当前歌词路径
        currentLyricsPath.clear();
        qDebug() << "currentLyricsPath.clear" << endl;
    }
}
// 媒体播放时长改变时的槽函数
void Audio::musicPlayerDurationChanged(qint64 duration)
{
    // 设置歌曲播放时间范围 duration / 1000 ：毫秒转换为秒
    ui->horizontalSlider->setRange(0, duration / 1000);
    int second  = duration / 1000; // 获取秒
    int minute = second / 60; // 获取分
    second %= 60;

    QString mediaDuration;
    mediaDuration.clear();
    // 分、秒小于10则前面补0，否则正常显示
    if (minute >= 10)
        mediaDuration = QString::number(minute, 10);
    else
        mediaDuration = "0" + QString::number(minute, 10);

    if (second >= 10)
        mediaDuration = mediaDuration
                + ":" + QString::number(second, 10);
    else
        mediaDuration = mediaDuration
                + ":0" + QString::number(second, 10);

    /* 显示媒体总长度时间 */
    ui->label_3->setText(mediaDuration);
}
// 媒体播放位置改变时的槽函数 拖动进度条
void Audio::mediaPlayerPositionChanged(qint64 position)
{
    // 判断滑动条是否被按下
    if (!ui->horizontalSlider->isSliderDown())
        ui->horizontalSlider->setValue(position/1000);

    int second  = position / 1000; // 获取总的秒数
    int minute = second / 60; // 算出分
    second %= 60; // 得到剩余的秒

    QString mediaPosition;
    mediaPosition.clear();

    if (minute >= 10)
        mediaPosition = QString::number(minute, 10);
    else
        mediaPosition = "0" + QString::number(minute, 10);

    if (second >= 10)
        mediaPosition = mediaPosition
                + ":" + QString::number(second, 10);
    else
        mediaPosition = mediaPosition
                + ":0" + QString::number(second, 10);

    /* 显示现在播放的时间 */
    ui->label_2->setText(mediaPosition);
}
// 媒体元数据可用状态改变时的槽函数
void Audio::onMetaDataAvailableChanged(bool available) {
    if (available) {
        // 获取元数据的图片
        QVariant coverArtVariant = musicPlayer->metaData(QMediaMetaData::CoverArtImage);
        if (coverArtVariant.isValid()) {
            QImage coverArtImage = qvariant_cast<QImage>(coverArtVariant); // 转换成QImage后保存
            if (!coverArtImage.isNull()) {
//                coverArtImage.save("/home/book/Desktop/Program/qt/musicMedia/DescoverArtImage.jpg");
                setAlbumArt(QPixmap::fromImage(coverArtImage));
            } else {
                // 如果没有封面图像，则可以设置一个默认图片
                setAlbumArt(QPixmap(":/images/fengshen.jpg"));
            }
        }
        else
            setAlbumArt(QPixmap(":/images/fengshen.jpg"));
    }
}

// 列表项点击事件处理
void Audio::on_listWidget_itemClicked(QListWidgetItem *item)
{
    musicPlayer->stop();
    mediaPlaylist->setCurrentIndex(ui->listWidget->row(item));
//    musicPlayer->play();
    emit ui->btn_play->clicked(true);

}
// 水平滑块释放事件处理
void Audio::on_horizontalSlider_sliderReleased()
{
    /* 当进度条被拖动释放后设置媒体播放的位置 */
    musicPlayer->setPosition(ui->horizontalSlider->value() * 1000);
}
// 关闭按钮点击事件处理
void Audio::on_pushButton_clicked()
{
    clearConnections();
    // 设置音乐播放器状态为停止状态
    musicPlayer->stop();
    // 播放/暂停按钮设置为未选中状态
    ui->btn_play->setChecked(false);

    // 重置播放列表索引为 -1
    mediaPlaylist->setCurrentIndex(-1);
    // 清除列表中的当前选中项
    ui->listWidget->setCurrentRow(-1);

    // 将水平滑块设置到初始位置（值为0）
    ui->horizontalSlider->setValue(0);
    // 清空显示的播放时间等信息
    ui->label_2->setText("");
    ui->label_3->setText("");

    // 设置专辑封面为初始默认图片
    setAlbumArt(QPixmap(":/images/fengshen.jpg"));

    // 重置当前歌词路径为空
    currentLyricsPath.clear();
    // 清除歌词显示标签的文本内容
    ui->label_4->setText("");
    emit this->back();
}
// 垂直滑块释放事件处理
void Audio::on_verticalSlider_sliderReleased()
{
    musicPlayer->setVolume(ui->verticalSlider->value());
}
