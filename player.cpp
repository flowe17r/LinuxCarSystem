#include "player.h"
#include "playercontrols.h"
#include "playlistmodel.h"
#include "videowidget.h"

#include <QMediaService>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QAudioProbe>
#include <QMediaMetaData>
#include <QtWidgets>

// 数据库操作相关头文件
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDebug>
#include <QVariant>
#include <QList>

/**
 * @brief Player 构造函数：初始化视频播放器界面、播放引擎、数据库和信号连接
 * @param parent 父窗口指针
 */
Player::Player(QWidget *parent)
    : QWidget(parent)
{
    // ------ 1. 设置窗口基础样式 ------
    // 忽略大小策略，让视频控件自动适应父布局大小变化
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    // 设置背景色为深灰 (RGB 51,51,51)，使视频区域背景不刺眼
    QPalette p = palette();
    QColor backgroundColor(51,51,51);
    p.setColor(QPalette::Window, backgroundColor);
    setPalette(p);

    // ------ 2. 创建媒体播放器和播放列表 ------
    m_player = new QMediaPlayer(this);
    // 设置音频角色为视频角色（让系统按视频场景优化音频）
    m_player->setAudioRole(QAudio::VideoRole);

    // 创建播放列表并关联到播放器
    m_playlist = new QMediaPlaylist();
    m_player->setPlaylist(m_playlist);

    // ------ 3. 创建视频显示控件 ------
    m_videoWidget = new VideoWidget(this);
    m_player->setVideoOutput(m_videoWidget);

    // 全屏浮层退出按钮：作为视频控件的子控件叠在画面上，默认隐藏，单击视频唤出
    m_exitFullButton = new QPushButton(tr("退出全屏"), m_videoWidget);
    m_exitFullButton->setStyleSheet(
        "QPushButton { background-color: rgba(40, 40, 40, 200); color: white;"
        " border: 1px solid white; padding: 8px 16px; font-size: 16px; }"
        "QPushButton:pressed { background-color: rgba(90, 90, 90, 220); }"
    );
    m_exitFullButton->adjustSize();
    m_exitFullButton->hide();

    // ------ 4. 创建播放列表模型和视图 ------
    m_playlistModel = new PlaylistModel(this);
    m_playlistModel->setPlaylist(m_playlist);

    m_playlistView = new QListView(this);
    m_playlistView->setModel(m_playlistModel);
    // 高亮当前播放项
    m_playlistView->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));
    // 透明背景，白色文字（适应深色背景）
    m_playlistView->setStyleSheet("background-color: transparent; color:white");

    // ------ 5. 创建进度条和时间标签 ------
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, m_player->duration() / 1000); // 单位：秒
    m_labelDuration = new QLabel(this);

    // ------ 6. 创建控制按钮（打开、返回、全屏） ------
    openButton = new QPushButton(tr("Open"), this);
    openButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: 1px solid white; width: 50px; }"
        "QPushButton:hover { background-color: lightgray; }"
    );

    backButton = new QPushButton(tr("Back"), this);
    backButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: 1px solid white; width: 50px; }"
        "QPushButton:hover { background-color: lightgray; }"
    );

    m_fullScreenButton = new QPushButton(tr("FullScreen"), this);
    m_fullScreenButton->setCheckable(true);
    m_fullScreenButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: 1px solid white; width: 100px; }"
        "QPushButton:hover { background-color: lightgray; }"
    );

    // ------ 7. 创建自定义播放控制面板（包含播放/暂停/音量等） ------
    controls = new PlayerControls(this);
    // 初始状态同步
    controls->setState(m_player->state());
    controls->setVolume(m_player->volume());

    // ------ 8. 布局：视频 + 列表（水平），进度条，控制按钮行 ------
    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(m_videoWidget, 2);   // 视频占 2/3
    displayLayout->addWidget(m_playlistView);     // 列表占 1/3

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(backButton);
    controlLayout->addStretch(1);
    controlLayout->addStretch(1);
    controlLayout->addWidget(controls);
    controlLayout->addStretch(1);
    controlLayout->addWidget(m_fullScreenButton);

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_slider);
    hLayout->addWidget(m_labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);

    // QNX 系统无标题栏，额外增加状态栏显示信息
    #if defined(Q_OS_QNX)
    m_statusLabel = new QLabel;
    m_statusBar = new QStatusBar;
    m_statusBar->addPermanentWidget(m_statusLabel);
    m_statusBar->setSizeGripEnabled(false);
    layout->addWidget(m_statusBar);
    #endif

    setLayout(layout);

    // ------ 9. 检查媒体服务是否可用 ------
    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"
                                "Please check the media service plugins are installed."));
        controls->setEnabled(false);
        m_playlistView->setEnabled(false);
        openButton->setEnabled(false);
        m_fullScreenButton->setEnabled(false);
    }

    // ------ 10. 初始化 SQLite 数据库 ------
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(QCoreApplication::applicationDirPath() + "/media_player.db");
    if (!m_database.open()) {
        qDebug() << "无法打开数据库：" << m_database.lastError().text();
        return;
    }

    // 创建表 video（若不存在）
    QSqlQuery createTableQuery;
    createTableQuery.prepare(
        "CREATE TABLE IF NOT EXISTS video ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "video_name TEXT UNIQUE)"
    );
    if (!createTableQuery.exec()) {
        qDebug() << "创建表失败：" << createTableQuery.lastError().text();
    }

    // 从数据库加载历史播放列表
    loadPlaylistFromDatabase();

    // ------ 11. 初始化元数据显示 ------
    metaDataChanged();

    // 固定窗口大小（适配 1024x600 屏幕，如嵌入式设备）
    setFixedSize(1024, 600);

    // ------ 12. 连接所有信号与槽 ------
    connect(m_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &Player::metaDataChanged);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &Player::playlistPositionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &Player::statusChanged);
    connect(m_player, &QMediaPlayer::bufferStatusChanged, this, &Player::bufferingProgress);
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &Player::displayErrorMessage);
    connect(m_playlistView, &QAbstractItemView::activated, this, &Player::jump);
    // 拖动过程中不 seek：IMX6ULL 纯软解，连续 seek 会卡死播放；松手才跳转一次
    connect(m_slider, &QSlider::sliderReleased, this, [this]() { seek(m_slider->value()); });
    connect(openButton, &QPushButton::clicked, this, &Player::open);
    connect(backButton, &QPushButton::clicked, this, &Player::backout);

    // 全屏：自管布局级切换，不用 QVideoWidget::setFullScreen（顶层窗口在 linuxfb 上回不来）
    connect(m_fullScreenButton, &QPushButton::clicked, this, &Player::toggleFullScreen);
    connect(m_videoWidget, &VideoWidget::doubleClicked, this, &Player::toggleFullScreen);
    // 全屏下单击视频 → 浮出退出按钮
    connect(m_videoWidget, &VideoWidget::clicked, this, [this]() {
        if (m_isFullScreen)
            showFullOverlay();
    });
    connect(m_exitFullButton, &QPushButton::clicked, this, [this]() {
        setFullScreenUi(false);
    });

    // 播放控制面板的信号 → 播放器操作
    connect(controls, &PlayerControls::play, m_player, &QMediaPlayer::play);
    connect(controls, &PlayerControls::pause, m_player, &QMediaPlayer::pause);
    connect(controls, &PlayerControls::stop, m_player, &QMediaPlayer::stop);
    connect(controls, &PlayerControls::next, m_playlist, &QMediaPlaylist::next);
    connect(controls, &PlayerControls::previous, this, &Player::previousClicked);
    connect(controls, &PlayerControls::changeVolume, m_player, &QMediaPlayer::setVolume);
    connect(controls, &PlayerControls::changeMuting, m_player, &QMediaPlayer::setMuted);
    connect(controls, &PlayerControls::changeRate, m_player, &QMediaPlayer::setPlaybackRate);
    connect(controls, &PlayerControls::stop, m_videoWidget, QOverload<>::of(&QVideoWidget::update));

    // 播放器状态变化 → 同步控制面板状态
    connect(m_player, &QMediaPlayer::stateChanged, controls, &PlayerControls::setState);
    connect(m_player, &QMediaPlayer::volumeChanged, controls, &PlayerControls::setVolume);
    connect(m_player, &QMediaPlayer::mutedChanged, controls, &PlayerControls::setMuted);
}

/**
 * @brief 析构函数：关闭数据库连接
 */
Player::~Player()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

// ==================== 公共接口 ====================

/**
 * @brief 检查媒体播放器是否可用（服务插件是否加载成功）
 */
bool Player::isPlayerAvailable() const
{
    return m_player->isAvailable();
}

/**
 * @brief 打开文件对话框，选择视频文件，将文件名存入数据库并添加到播放列表
 *
 * 流程：
 * 1. 弹出文件对话框，支持按 MIME 类型过滤。
 * 2. 对每个选中的文件，提取基础名（不含扩展名）。
 * 3. 检查该名称是否已存在于数据库（去重）。
 * 4. 若不存在，插入数据库并添加到播放列表。
 */
void Player::open()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setFileMode(QFileDialog::ExistingFiles);

    // 用后缀名过滤，不用 supportedMimeTypes()：
    // SDK 的 ffmpeg 插件上报的 MIME 表经常不全，用它做过滤器会把 mp4 隐藏掉
    fileDialog.setNameFilters(QStringList()
        << tr("视频 (*.mp4 *.mkv *.avi *.mov *.ts)")
        << tr("所有文件 (*)"));
    fileDialog.selectNameFilter(tr("视频 (*.mp4 *.mkv *.avi *.mov *.ts)"));

    // 默认目录：程序旁的 Videos，不存在则退回用户主目录
    QString startDir = QCoreApplication::applicationDirPath() + "/Videos";
    if (!QFileInfo(startDir).isDir())
        startDir = QDir::homePath();
    fileDialog.setDirectory(startDir);

    if (fileDialog.exec() == QDialog::Accepted) {
        QList<QUrl> selectedUrls = fileDialog.selectedUrls();
        if (selectedUrls.isEmpty())
            qDebug() << "Open 对话框未选中任何文件，目录：" << startDir;

        QSqlQuery insertQuery;
        for (const QUrl& url : selectedUrls) {
            QFileInfo fileInfo(url.toLocalFile());
            QString videoNameWithoutExtension = fileInfo.completeBaseName();

            // 检查是否已存在
            if (!isVideoAlreadyExists(videoNameWithoutExtension)) {
                insertQuery.prepare("INSERT INTO video (video_name) VALUES (:video_name)");
                insertQuery.bindValue(":video_name", videoNameWithoutExtension);
                if (!insertQuery.exec()) {
                    qDebug() << "插入数据失败：" << insertQuery.lastError().text();
                }
                m_playlist->addMedia(url);
            } else {
                qDebug() << "文件已存在于数据库中，不重复添加：" + videoNameWithoutExtension;
            }
        }
    }
}

/**
 * @brief 返回上一级界面（退出全屏、停止播放、重置进度，发送 back 信号）
 */
void Player::backout()
{
    if (m_isFullScreen)
        setFullScreenUi(false);
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    }
    m_player->setPosition(0);
    m_slider->setValue(0);
    emit this->back();
}

// ==================== 数据库辅助函数 ====================

/**
 * @brief 检查指定视频名称是否已存在于 video 表中
 */
bool Player::isVideoAlreadyExists(const QString &videoName)
{
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM video WHERE video_name = :video_name");
    checkQuery.bindValue(":video_name", videoName);
    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        return count > 0;
    }
    return false;
}

/**
 * @brief 从数据库加载所有视频名称，查找对应文件并添加到播放列表
 *
 * 数据库只存名称，实际文件在 ./Videos/ 目录下，依次尝试 .mp4, .mkv, .avi
 */
void Player::loadPlaylistFromDatabase()
{
    QSqlQuery selectQuery("SELECT * FROM video");
    while (selectQuery.next()) {
        QString videoName = selectQuery.value(1).toString();
        QString videoPath = findVideoPath(videoName);
        if (!videoPath.isEmpty()) {
            QUrl videoUrl = QUrl::fromLocalFile(videoPath);
            m_playlist->addMedia(videoUrl);
        }
    }
}

/**
 * @brief 根据视频名称查找文件路径（按扩展名顺序查找）
 */
QString Player::findVideoPath(const QString &videoName)
{
    QString basePath = QCoreApplication::applicationDirPath() + "/Videos";
    // 依次尝试 .mp4, .mkv, .avi
    QStringList extensions = {".mp4", ".mkv", ".avi"};
    for (const QString &ext : extensions) {
        QString fullPath = basePath + QDir::separator() + videoName + ext;
        if (QFile::exists(fullPath))
            return fullPath;
    }
    return QString();
}

// ==================== 播放列表辅助 ====================

/**
 * @brief 判断 URL 是否指向 .m3u 播放列表文件
 */
static bool isPlaylist(const QUrl &url)
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists() && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

/**
 * @brief 将 URL 列表添加到播放列表（若为 .m3u 则加载其内容）
 */
void Player::addToPlaylist(const QList<QUrl> &urls)
{
    for (auto &url : urls) {
        if (isPlaylist(url))
            m_playlist->load(url);
        else
            m_playlist->addMedia(url);
    }
}

// ==================== 播放控制槽函数 ====================

/**
 * @brief 设置自定义音频角色（由外部调用）
 */
void Player::setCustomAudioRole(const QString &role)
{
    m_player->setCustomAudioRole(role);
}

/**
 * @brief 更新进度条最大值为媒体总时长（秒）
 */
void Player::durationChanged(qint64 duration)
{
    m_duration = duration / 1000;
    m_slider->setMaximum(m_duration);
}

/**
 * @brief 播放位置变化时更新进度条和时间显示
 *
 * 如果用户没有拖动滑块，则自动更新滑块位置；
 * 否则不干扰用户拖动操作，仅在释放时跳转。
 */
void Player::positionChanged(qint64 progress)
{
    if (!m_slider->isSliderDown())
        m_slider->setValue(progress / 1000);
    updateDurationInfo(progress / 1000);
}

/**
 * @brief 元数据可用时，更新轨道信息和封面图
 *
 * 读取 AlbumArtist 和 Title 拼接显示；
 * 封面图通过 CoverArtUrlLarge 加载。
 */
void Player::metaDataChanged()
{
    if (m_player->isMetaDataAvailable()) {
        setTrackInfo(QString("%1 - %2")
            .arg(m_player->metaData(QMediaMetaData::AlbumArtist).toString())
            .arg(m_player->metaData(QMediaMetaData::Title).toString()));

        if (m_coverLabel) {
            QUrl url = m_player->metaData(QMediaMetaData::CoverArtUrlLarge).value<QUrl>();
            m_coverLabel->setPixmap(!url.isEmpty()
                ? QPixmap(url.toString())
                : QPixmap());
        }
    }
}

/**
 * @brief 上一曲按钮点击处理：若播放位置 ≤5 秒则切换到上一首，否则跳转到开头
 */
void Player::previousClicked()
{
    if (m_player->position() <= 5000)
        m_playlist->previous();
    else
        m_player->setPosition(0);
}

/**
 * @brief 双击播放列表项时切换并播放
 */
void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
        m_player->play();
    }
}

/**
 * @brief 播放列表当前索引变化时，同步高亮视图中的对应项
 */
void Player::playlistPositionChanged(int currentItem)
{
    m_playlistView->setCurrentIndex(m_playlistModel->index(currentItem, 0));
}

/**
 * @brief 进度条拖动释放后跳转到对应位置（秒→毫秒）
 */
void Player::seek(int seconds)
{
    m_player->setPosition(seconds * 1000);
}

// ==================== 状态与错误处理 ====================

/**
 * @brief 媒体状态变化时更新状态信息，并控制鼠标光标样式
 */
void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(m_player->bufferStatus()));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(m_player->bufferStatus()));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);  // 闪一下任务栏/窗口
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

/**
 * @brief 根据媒体状态设置鼠标指针为忙碌或默认
 */
void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia) {
        setCursor(QCursor(Qt::BusyCursor));
    } else {
        unsetCursor();
    }
#endif
}

/**
 * @brief 缓冲进度变化时更新状态信息（区分卡顿和正常缓冲）
 */
void Player::bufferingProgress(int progress)
{
    if (m_player->mediaStatus() == QMediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(progress));
    else
        setStatusInfo(tr("Buffering %1%").arg(progress));
}

// ==================== 全屏 ====================

/**
 * @brief 全屏切换：按钮、双击视频、ESC 都收敛到这里
 */
void Player::toggleFullScreen()
{
    setFullScreenUi(!m_isFullScreen);
}

/**
 * @brief 布局级全屏：隐藏列表、控制行和进度条并清零布局边距，视频铺满整屏；
 *        退出入口为：单击视频浮出的按钮、双击视频、ESC，保证 linuxfb 下一定能退出
 */
void Player::setFullScreenUi(bool fullScreen)
{
    m_isFullScreen = fullScreen;
    m_fullScreenButton->setChecked(fullScreen);
    m_fullScreenButton->setText(fullScreen ? tr("退出全屏") : tr("FullScreen"));
    if (!fullScreen && !m_savedMargins.isNull()) {
        layout()->setContentsMargins(m_savedMargins);
        m_savedMargins = QMargins();
    }
    m_fullScreenButton->setVisible(!fullScreen);
    m_playlistView->setVisible(!fullScreen);
    openButton->setVisible(!fullScreen);
    backButton->setVisible(!fullScreen);
    controls->setVisible(!fullScreen);
    m_slider->setVisible(!fullScreen);
    m_labelDuration->setVisible(!fullScreen);

    if (!fullScreen) {
        m_exitFullButton->hide();
        emit fullScreenChanged(fullScreen);
        return;
    }

    // 清零外边距并压缩间距，让视频真正铺满整屏
    if (layout()) {
        m_savedMargins = layout()->contentsMargins();
        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setSpacing(0);
    }
    emit fullScreenChanged(fullScreen);
}

/**
 * @brief 全屏下单击视频区域：在右上角浮出"退出全屏"按钮，3 秒后自动收起
 */
void Player::showFullOverlay()
{
    m_exitFullButton->adjustSize();
    m_exitFullButton->move(m_videoWidget->width() - m_exitFullButton->width() - 16, 16);
    m_exitFullButton->show();
    m_exitFullButton->raise();

    // 重新点击时重置倒计时（旧定时器因 context 失效自动作废）
    QTimer::singleShot(3000, m_exitFullButton, [this]() {
        if (m_isFullScreen)
            m_exitFullButton->hide();
    });
}

void Player::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_isFullScreen) {
        setFullScreenUi(false);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

// ==================== 信息显示工具函数 ====================

/**
 * @brief 设置轨道信息（曲目标题/艺术家），显示在状态栏或窗口标题
 */
void Player::setTrackInfo(const QString &info)
{
    m_trackInfo = info;
    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

/**
 * @brief 设置状态信息（加载/缓冲/错误等），显示在状态栏或窗口标题
 */
void Player::setStatusInfo(const QString &info)
{
    m_statusInfo = info;
    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

/**
 * @brief 显示错误信息（从播放器获取错误字符串）
 */
void Player::displayErrorMessage()
{
    setStatusInfo(m_player->errorString());
}

/**
 * @brief 更新时间标签：当前时间 / 总时间（格式 mm:ss 或 hh:mm:ss）
 */
void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60,
                          currentInfo % 60, (currentInfo * 1000) % 1000);
        QTime totalTime((m_duration / 3600) % 60, (m_duration / 60) % 60,
                        m_duration % 60, (m_duration * 1000) % 1000);
        QString format = "mm:ss";
        if (m_duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    m_labelDuration->setText(tStr);
}
