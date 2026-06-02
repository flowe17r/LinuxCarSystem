QT       += core gui network \
            xml \
            multimedia \
            multimediawidgets \
            widgets \
            sql \
            serialport


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Audio.cpp \
    buzzer.cpp \
    citycodeutils.cpp \
    day.cpp \
    dht11.cpp \
    hardware.cpp \
    led.cpp \
    main.cpp \
    mainwindow.cpp \
    map.cpp \
    mq135.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    rotatablewidget.cpp \
    simplemessagebox.cpp \
    sr04.cpp \
    videowidget.cpp \
    weather.cpp

HEADERS += \
    Audio.h \
    buzzer.h \
    citycodeutils.h \
    commonhelper.h \
    day.h \
    dht11.h \
    hardware.h \
    led.h \
    mainwindow.h \
    map.h \
    mq135.h \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    rotatablewidget.h \
    simplemessagebox.h \
    sr04.h \
    videowidget.h \
    weather.h

FORMS += \
    Audio.ui \
    hardware.ui \
    mainwindow.ui \
    map.ui \
    rotatablewidget.ui \
    weather.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
