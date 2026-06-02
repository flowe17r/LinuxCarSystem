#include "weather.h"
#include "ui_weather.h"

#include <QMouseEvent>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>

Weather::Weather(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Weather)
{
    ui->setupUi(this);
    // 设置字体格式
    this->setFont(QFont("Airal",15));
    // 固定窗口 防止拖拉
    setFixedSize(1024,600); // 设置窗口固定尺寸
    setWindowFlag(Qt::FramelessWindowHint);
    // 新建菜单组件
    menuQuit = new QMenu(this);
    // 设置菜单项文字颜色
    menuQuit->setStyleSheet("QMenu::item {color:white}");
    // 为菜单组件添加一个动作
    QAction *closeAct = new QAction(QIcon(":/images/close.png"),tr("退出"),this);
    menuQuit->addAction(closeAct);

    // 用户选择菜单项并点击后回调函数，退出程序
    connect(menuQuit,&QMenu::triggered,this,[=]{
        this->close();
    });



    mWeekList << ui->labelday0 << ui->labelday1
              << ui->labelday2 << ui->labelday3
              << ui->labelday4 << ui->labelday5;
    mDateList << ui->labelDate0 << ui->labelDate1
              << ui->labelDate2 << ui->labelDate3
              << ui->labelDate4 << ui->labelDate5;
    mIconList << ui->labelWeaterIcon0 << ui->labelWeaterIcon1
              << ui->labelWeaterIcon2 << ui->labelWeaterIcon3
              << ui->labelWeaterIcon4 << ui->labelWeaterIcon5;
    mWeaTypeList << ui->lbweatherTypeDate0 << ui->lbweatherTypeDate1
                 << ui->lbweatherTypeDate2 << ui->lbweatherTypeDate3
                 << ui->lbweatherTypeDate4 << ui->lbweatherTypeDate5;
    mAirqList << ui->labelairq0 << ui->labelairq1 << ui->labelairq2
              << ui->labelairq3 << ui->labelairq4 << ui->labelairq5;
    mFxList << ui->labelFX0 << ui->labelFX1 << ui->labelFX2
            << ui->labelFX3 << ui->labelFX4 << ui->labelFX5;
    mFlList << ui->labelFL0 << ui->labelFL1 << ui->labelFL2
            << ui->labelFL3 << ui->labelFL4 << ui->labelFL5;


    //根据keys,设置icon的路径
    mTypeMap.insert("暴雪",":/images/type/BaoXue.png");
    mTypeMap.insert("暴雨",":/images/type/BaoYu. png");
    mTypeMap.insert("暴雨到大暴雨",":/images/type/BaoYuDaoDaBaoYu.png");
    mTypeMap.insert("大暴雨",":/images/type/DaBaoYu.png");
    mTypeMap.insert("大暴雨到特大暴雨",":/images/type/DaBaoYuDaoTeDaBaoYu.png");
    mTypeMap.insert("大到暴雪",":/images/type/DaDaoBaoXue.png");
    mTypeMap.insert("大雪",":/images/type/DaXue.png");
    mTypeMap.insert("大雨",":/images/type/DaYu.png");
    mTypeMap.insert("冻雨",":/images/type/DongYu.png");
    mTypeMap.insert("多云",":/images/type/DuoYun.png");
    mTypeMap.insert("浮沉",":/images/type/FuChen.png");
    mTypeMap.insert("雷阵雨",":/images/type/LeiZhenYu.png");
    mTypeMap.insert("雷阵雨伴有冰雹",":/images/type/LeiZhenYuBanYouBingBao.png");
    mTypeMap.insert("霾",":/images/type/Mai.png");
    mTypeMap.insert("强沙尘暴",":/images/type/QiangShaChenBao.png");
    mTypeMap.insert("晴",":/images/type/Qing.png");
    mTypeMap.insert("沙尘暴",":/images/type/ShaChenBao.png");
    mTypeMap.insert("特大暴雨",":/images/type/TeDaBaoYu.png");
    mTypeMap.insert("undefined",":/images/type/undefined.png");
    mTypeMap.insert("雾",":/images/type/Wu.png");
    mTypeMap.insert("小到中雪",":/images/type/XiaoDaoZhongXue.png");
    mTypeMap.insert("小到中雨",":/images/type/XiaoDaoZhongYu.png");
    mTypeMap.insert("小雪",":/images/type/XiaoXue.png");
    mTypeMap.insert("小雨",":/images/type/XiaoYu.png");
    mTypeMap.insert("雪",":/images/type/Xue.png");
    mTypeMap.insert("扬沙",":/images/type/YangSha.png");
    mTypeMap.insert("阴",":/images/type/Yin.png");
    mTypeMap.insert("雨",":/images/type/Yu.png");
    mTypeMap.insert("雨夹雪",":/images/type/YuJiaXue.png");
    mTypeMap.insert("阵雪",":/images/type/ZhenXue.png");
    mTypeMap.insert("阵雨",":/images/type/ZhenYu.png");
    mTypeMap.insert("中到大雪",":/images/type/ZhongDaoDaXue.png");
    mTypeMap.insert("中到大雨",":/images/type/ZhongDaoDaYu.png");
    mTypeMap.insert("中雪",":/images/type/ZhongXue.png");
    mTypeMap.insert("中雨",":/images/type/ZhongYu.png");
    // 安装事件过滤器
    ui->widget0404->installEventFilter(this);
    ui->widget0405->installEventFilter(this);



}
void Weather::weather_working()
{
    // 由QNetworkAccessManager发起get请求
    manager = new QNetworkAccessManager(this);
    //strUrl = "http://v1.yiketianqi.com/api?unescape=1&version=v63&appid=71755637&appsecret=cxCm8AxT";
    strUrl = "http://v1.yiketianqi.com/api?unescape=1&version=v9&appid=71755637&appsecret=cxCm8AxT";
    //strUrl = "http://v1.yiketianqi.com/api?unescape=1&version=v9&appid=26371314&appsecret=qOhoD413";

    QUrl urlTianQi(strUrl);
    // 指定请求的url地址
    QNetworkRequest res(urlTianQi);
    reply = manager->get(res);
    // 网络请求后进行数据读取
    connect(manager,&QNetworkAccessManager::finished,this,&Weather::readHttpRply);
}
Weather::~Weather()
{
    delete ui;
}



// 重写鼠标点击
void Weather::mousePressEvent(QMouseEvent *event)
{
    // 捕获右键被按下
    if(event->button() == Qt::RightButton){
        //qDebug() << "right Mouse clicked!";
        menuQuit->exec(QCursor::pos());
    }
    // 捕获左键被按下

//    if(event->button() == Qt::LeftButton){
//        // qDebug() << "left Mouse clicked!";
//        // 鼠标当前位置
//        // qDebug() << event->globalPos();
//        // 窗口当前位置
//        // qDebug() << this->pos();
//        // 窗口新位置 event->globalPos() - this->pos()
//        mOffset = event->globalPos() - this->pos();
//    }
}

// 鼠标左键按下后的移动，导致事件被调用，设置窗口的新位置
//void Widget::mouseMoveEvent(QMouseEvent *event)
//{
//    this->move(event->globalPos() - mOffset);
//}

// 绘制温度曲线变化
bool Weather::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->widget0404 && event->type() == QEvent::Paint){
        drawTempLineHigh();
        return true;
    }
    if(watched == ui->widget0405 && event->type() == QEvent::Paint){
        drawTempLineLow();
        return true;
    }
    return QWidget::eventFilter(watched,event);
}
// 解析json数据 单天数据
void Weather::parseWeatherJsonData(QByteArray rawData)
{
    QJsonDocument jsonObj = QJsonDocument::fromJson(rawData);
    if(!jsonObj.isNull() && jsonObj.isObject()){
        QJsonObject objRoot = jsonObj.object();
        // 解析当前日期
        QString date = objRoot["date"].toString();
        QString week = objRoot["week"].toString();
        ui->labelCurrentDate->setText(date + " " + week);
        // 解析城市名词
        QString cityName = objRoot["city"].toString();
        ui->labelCity->setText(cityName + "市");
        // 解析当前温度
        QString currentTemp = objRoot["tem"].toString();
        ui->labelTmp->setText(currentTemp + "℃");
        ui->labelTempRange->setText(objRoot["tem2"].toString() + "~" + objRoot["tem1"].toString());
        // 解析天气类型
        ui->labelweatherType->setText(objRoot["wea"].toString());
        ui->labelWeatherIcon->setPixmap(mTypeMap[objRoot["wea"].toString()]);
        // 解析感冒指数
        ui->labelGanmao->setText(objRoot["air_tips"].toString());
        // 解析风向
        ui->labelFXType->setText(objRoot["win"].toString());
        // 解析风力
        ui->labelFXType_2->setText(objRoot["win_speed"].toString());
        // 解析PM2.5
        ui->labelPM25Data->setText(objRoot["air_pm25"].toString());
        // 解析湿度
        ui->labelShiduData->setText(objRoot["humidity"].toString());
        // 解析空气质量
        ui->labelairData->setText(objRoot["air_level"].toString());
    }
}

// 解析json数据 七天数据
void Weather::parseWeatherJsonDataNew(QByteArray rawData)
{
    // 将JSON作为UTF-8编码的JSON文档进行解析，并基于它创建一个 QJsonDocument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rawData);
    // 解析对象
    if(!jsonDoc.isNull() && jsonDoc.isObject()){
        QJsonObject jsonRoot = jsonDoc.object();
        days[0].mCity = jsonRoot["city"].toString();
        days[0].mPm25 = jsonRoot["aqi"].toObject()["pm25"].toString();
        // 解析数组
        if(jsonRoot.contains("data") && jsonRoot["data"].isArray()){
            // 转换类型为QT表示JSON数组的数据类型
            QJsonArray weaArray = jsonRoot["data"].toArray();
            for(int i = 0; i < weaArray.size(); i++){
                // 转换类型为QT表示JSON对象的数据类型
                QJsonObject obj = weaArray[i].toObject();
                days[i].mDate = obj["date"].toString();
                days[i].mWeek = obj["week"].toString();
                days[i].mWeathType = obj["wea"].toString();
                days[i].mTemp = obj["tem"].toString();
                days[i].mTempLow = obj["tem2"].toString();
                days[i].mTempHigh = obj["tem1"].toString();
                days[i].mFx = obj["win"].toArray()[0].toString();
                days[i].mFl = obj["win_speed"].toString();
                days[i].mAirq = obj["air_level"].toString();
                days[i].mTips = obj["air_tips"].toString();
                days[i].mHu = obj["humidity"].toString();

            }
        }
    }
    // 更新界面
    updateUI();
}

void Weather::updateUI()
{
    QPixmap pixmap;
    // 解析当前日期
    ui->labelCurrentDate->setText(days[0].mDate + " " + days[0].mWeek);
    // 解析城市名词
    ui->labelCity->setText(days[0].mCity + "市");
    // 解析当前温度
    ui->labelTmp->setText(days[0].mTemp + "℃");
    ui->labelTempRange->setText(
                days[0].mTempLow + "℃" + "~"  + days[0].mTempHigh +  "℃");
    // 解析天气类型
    ui->labelweatherType->setText(days[0].mWeathType);
    ui->labelWeatherIcon->setPixmap(mTypeMap[days[0].mWeathType]);
    // 解析感冒指数
    ui->labelGanmao->setText(days[0].mTips);
    // 解析风向
    ui->labelFXType->setText(days[0].mFx);
    // 解析风力
    ui->labelFXType_2->setText(days[0].mFl);
    // 解析PM2.5
    ui->labelPM25Data->setText(days[0].mPm25);
    // 解析湿度
    ui->labelShiduData->setText(days[0].mHu);
    // 解析空气质量
    ui->labelairData->setText(days[0].mAirq);
    ui->lbweatherTypeDate3->setText(days[3].mWeathType);
    ui->lbweatherTypeDate0->setText(days[0].mWeathType);
    for(int i = 0; i < 6; i++){

        mWeekList[i]->setText(days[i].mWeek);
        mWeekList[0]->setText("今天");
        mWeekList[1]->setText("明天");
        mWeekList[2]->setText("后天");
        QStringList dayList = days[i].mDate.split("-");
        mDateList[i]->setText(dayList.at(1) + "-" + dayList.at(2));
        // 缩放图片大小和label大小能匹配
        int index = days[i].mWeathType.indexOf("转");
        if(index != -1){
            pixmap = mTypeMap[days[i].mWeathType.left(index)];
        }else{
            pixmap = mTypeMap[days[i].mWeathType];
        }
        pixmap = pixmap.scaled(mIconList[i]->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        mIconList[i]->setMaximumHeight(50);
        mIconList[i]->setMaximumWidth(ui->widget02->width()/6.5);
        mIconList[i]->setPixmap(pixmap);
        mWeaTypeList[i]->setText(days[i].mWeathType);
        QString airQ = days[i].mAirq;
        mAirqList[i]->setText(airQ);
        if(airQ == "优"){
            mAirqList[i]->setStyleSheet("background-color:rgb(150, 213, 32);border-radius: 7px;color: rgb(230, 230, 230)");
        }
        if(airQ == "良"){
            mAirqList[i]->setStyleSheet("background-color:rgb(255, 170, 127);border-radius: 7px;color: rgb(230, 230, 230)");
        }
        if(airQ == "轻度污染"){
            mAirqList[i]->setStyleSheet("background-color:rgb(170, 200, 129);border-radius: 7px;color: rgb(230, 230, 230)");
        }
        if(airQ == "中度污染"){
            mAirqList[i]->setStyleSheet("background-color:rgb(255, 17, 17);border-radius: 7px;color: rgb(230, 230, 230)");
        }
        if(airQ == "重度污染"){
            mAirqList[i]->setStyleSheet("background-color:rgb(153, 0, 0);border-radius: 7px;color: rgb(230, 230, 230)");
        }

        mFxList[i]->setText(days[i].mFx);
        index = days[i].mFl.indexOf("转");
        if(index != -1){
            mFlList[i]->setText(days[i].mFl.left(index));
        }else{
            mFlList[i]->setText(days[i].mFl);
        }


    }
   update();
}

// 绘制最高温度曲线
void Weather::drawTempLineHigh()
{
    QPainter painter(ui->widget0404);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::yellow);

    int ave;
    int sum = 0;
    int offSet = 0;
    int middle = ui->widget0404->height()*0.7;
    for (int i = 0; i < 6; i++) {
        sum += days[i].mTempHigh.toInt();
    }
    ave = sum / 6;

    // 定义6个点
    QPoint points[6];
    for (int i = 0; i < 6; i++) {
          // 以mAirqList为参照算出 x 位置
          points[i].setX(mAirqList[i]->x() + mAirqList[i]->width()/2);
          // 算出偏移量 （最高温度 - 平均温度） * 2
          offSet = (days[i].mTempHigh.toInt() - ave)*2;
          // 算出 Y 位置 中值 - 偏移量
          points[i].setY(middle - offSet);
          // 画出6个点
          painter.drawEllipse(QPoint(points[i]),3,3);
          // 画出当天温度
          painter.drawText(points[i].x() - 15,points[i].y()-15,days[i].mTempHigh + "°");
    }
    for (int i = 0; i < 5; i++) {
        // 点与点之间连线
        painter.drawLine(points[i],points[i+1]);
    }

}
// 绘制最低温度曲线
void Weather::drawTempLineLow()
{
    QPainter painter(ui->widget0405);
    // 抗锯齿
    painter.setRenderHint(QPainter::Antialiasing,true);
    // 设置画刷
    painter.setBrush(QColor(70,192,203));
    // 设置画笔
    painter.setPen(QColor(70,192,203));

    int ave;
    int sum = 0;
    int offSet = 0;
    int middle = ui->widget0405->height()*0.7;
    for (int i = 0; i < 6; i++) {
        sum += days[i].mTempLow.toInt();
    }
    ave = sum / 6;

    // 定义6个点
    QPoint points[6];
    for (int i = 0; i < 6; i++) {
          points[i].setX(mAirqList[i]->x() + mAirqList[i]->width()/2);
          offSet = (days[i].mTempLow.toInt() - ave)*2;
          points[i].setY(middle - offSet);
          // 画出6个点
          painter.drawEllipse(QPoint(points[i]),3,3);
          // 画出当天温度
          painter.drawText(points[i].x() - 15,points[i].y()-15,days[i].mTempLow + "°");
    }
    for (int i = 0; i < 5; i++) {
        // 画出两点之间的线
        painter.drawLine(points[i],points[i+1]);
    }
}


// 读取数据
void Weather::readHttpRply(QNetworkReply *reply)
{
    // 获取返回码 获取从HTTP服务器接收到的HTTP状态码 2xx:成功状态码 4xx：客户端错误状态码 5xx:服务端错误状态码
    int resCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(reply->error() == QNetworkReply::NoError && resCode == 200){
        // 从设备中读取所有剩余数据，并将其作为字节数组返回。
        QByteArray data = reply->readAll();
        //parseWeatherJsonData(data);
        // 解析数据
        parseWeatherJsonDataNew(data);
        qDebug() << QString::fromUtf8(data);
    }else{
        //qDebug() << "请求失败:" << reply->errorString();
        QMessageBox mes;
        mes.setWindowTitle("错误");
        mes.setText("网络请求失败");
        mes.setStyleSheet("QPushButton {color:red}");
        mes.setStandardButtons(QMessageBox::Ok);
        mes.exec();

    }

}


void Weather::on_back_clicked()
{
    emit this->back();
}
