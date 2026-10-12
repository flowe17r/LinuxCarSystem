#include "settingspage.h"

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(1024, 600);
    setStyleSheet(
        "SettingsPage { background-color: #eef6ff; }"
        "QLabel { color: #1e293b; }"
        "QPushButton { min-height:52px; min-width:120px; font-size:20px; font-weight:bold;"
        " color:white; background-color:#3b82f6; border:none; border-radius:10px; }"
        "QPushButton:pressed { background-color:#1d4ed8; }");

    auto *title = new QLabel(QStringLiteral("系统设置"), this);
    title->setAlignment(Qt::AlignCenter);
    QFont tf = title->font();
    tf.setPixelSize(28);
    tf.setBold(true);
    title->setFont(tf);

    auto *volLabel = new QLabel(QStringLiteral("媒体音量"), this);
    auto *vol = new QSlider(Qt::Horizontal, this);
    vol->setRange(0, 100);
    vol->setValue(70);
    vol->setMinimumHeight(36);

    auto *briLabel = new QLabel(QStringLiteral("屏幕亮度（演示）"), this);
    auto *bri = new QSlider(Qt::Horizontal, this);
    bri->setRange(20, 100);
    bri->setValue(80);
    bri->setMinimumHeight(36);

    auto *about = new QLabel(
        QStringLiteral("车载娱乐终端  ·  1024×600\n摄像头：USB UVC / V4L2\n设置项当前为演示，不写入板端配置"),
        this);
    about->setAlignment(Qt::AlignCenter);
    about->setWordWrap(true);

    auto *backBtn = new QPushButton(QStringLiteral("返回"), this);
    connect(backBtn, &QPushButton::clicked, this, &SettingsPage::back);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(80, 30, 80, 30);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addSpacing(10);
    layout->addWidget(volLabel);
    layout->addWidget(vol);
    layout->addWidget(briLabel);
    layout->addWidget(bri);
    layout->addSpacing(12);
    layout->addWidget(about);
    layout->addStretch(1);
    layout->addWidget(backBtn, 0, Qt::AlignHCenter);
}
