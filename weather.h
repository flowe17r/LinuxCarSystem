#ifndef WEATHER_H
#define WEATHER_H

#include <QLabel>
#include <QMenu>
#include <QNetworkReply>
#include <QWidget>
#include "citycodeutils.h"
#include "day.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Weather; }
QT_END_NAMESPACE

class Weather : public QWidget
{
    Q_OBJECT

public:
    Day days[7];
    QList<QLabel *> mDateList;
    QList<QLabel *> mWeekList;
    QList<QLabel *> mIconList;
    QList<QLabel *> mWeaTypeList;
    QList<QLabel *> mAirqList;
    QList<QLabel *> mFxList;
    QList<QLabel *> mFlList;
    Weather(QWidget *parent = nullptr);
    ~Weather();
    void weather_working();
protected:
    void mousePressEvent(QMouseEvent *event);
    //void mouseMoveEvent(QMouseEvent *event);
    bool eventFilter(QObject *watched,QEvent *event);

public slots:
    void readHttpRply(QNetworkReply *reply);
private slots:
    //void on_lineEditCity_returnPressed();

    void on_back_clicked();

signals:
    void back();
private:
    Ui::Weather *ui;
    QMenu *menuQuit;
    QPoint mOffset; // 偏移值
    QNetworkReply *reply;
    void parseWeatherJsonData(QByteArray rawData);
    QString strUrl;
    CityCodeUtils cityCodeUtils;
    QMap<QString,QString> mTypeMap;
    QNetworkAccessManager *manager;
    void parseWeatherJsonDataNew(QByteArray rawData);
    void updateUI();
    void drawTempLineHigh();
    void drawTempLineLow();
};
#endif // WIDGET_H
