#include "map.h"

#include "simplemessagebox.h"
#include "commonhelper.h"

#include <QSslConfiguration>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QVBoxLayout>
#include <QPixmap>
#include <QSerialPortInfo>

Map::Map(QWidget *parent) : QDialog(parent)
{
    initUi();

}

Map::~Map()
{
    if (m_port.isOpen())
        m_port.close();
}

void Map::initUi()
{
    auto pMovie = new QMovie(this);
    pMovie->setFileName(":/images/loading.gif");
    pMovie->start();
    this->setFixedSize(1024,600);
    m_logoLabel.setObjectName("logoLabel");
    m_logoLabel.setText(QString("当前展示定位坐标：[") +m_location + "]");
    m_back = new QPushButton(tr("返回"),this);
    m_iconLabel.setMovie(pMovie);
    m_iconLabel.setAlignment(Qt::AlignCenter);
    m_logoLabel.setAlignment(Qt::AlignRight);

    auto *pVLayout = new QVBoxLayout;
    pVLayout->addStretch();
    pVLayout->addWidget(&m_logoLabel);
    pVLayout->addWidget(m_back);
    m_iconLabel.setLayout(pVLayout);
    connect(m_back,&QPushButton::clicked,this,&Map::on_back_clicked);
    auto *pMainLayout = new QVBoxLayout;
    pMainLayout->addWidget(&m_iconLabel);
    pMainLayout->setMargin(0);
    setLayout(pMainLayout);
    setObjectName("map_widget");
    CommonHelper::setStyleSheet(":/style/default.qss", this);
}

void Map::initCtrl()
{
    initSerialPort();

    requestMapImage(m_location);
}

// 串口初始化
void Map::initSerialPort()
{
    // 设置串口名称 (UART6)
    m_port.setPortName("ttymxc6");
    // 以读写方式打开串口设备
    if(!m_port.open(QIODevice::ReadWrite)) {
        SimpleMessageBox::infomationMessageBox("未检测到设备，请重试");
        m_logoLabel.setText(QString("未检测到GPS设备，当前展示定位坐标：[") +m_location + "]");
        return;
    }
    // 设置波特率为9600
    m_port.setBaudRate(QSerialPort::Baud9600, QSerialPort::AllDirections);
    // 设置数据位为8位
    m_port.setDataBits(QSerialPort::Data8);
    // 设置流控制为无流控制模式
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    // 设置为无校验位
    m_port.setParity(QSerialPort::NoParity);
    // 设置停止位为1位
    m_port.setStopBits(QSerialPort::OneStop);
    // readyRead信号是当串口接收到新的数据并且这些数据可以从串口读取时被触发。
    connect(&m_port, &QSerialPort::readyRead, this, &Map::portReadReady);
}
// 请求地图照片
void Map::requestMapImage(const QString &center)
{
    // 构建请求地图图片的URL地址
    QString url = baidudUrl.arg(baidudAk).arg(center);

    m_array.clear();
    // 发出网络请求
    requestNetwork(url);
}

void Map::requestNetwork(const QString &url)
{
    QSslConfiguration config;
    // 设置对等方验证模式为不进行验证
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    // 设置SSL协议版本为TlsV1_0
    config.setProtocol(QSsl::TlsV1_0);

    QNetworkRequest networkRequest;
    // 将之前配置好的SSL配置信息设置到网络请求对象中
    networkRequest.setSslConfiguration(config);
    // 设置请求地址
    networkRequest.setUrl(url);
    // 获取请求返回数据
    QNetworkReply *newReply = m_networkAccessManager.get(networkRequest);
    // 当一旦请求到数据时执行netReadyRead
    connect(newReply, &QNetworkReply::readyRead, this, &Map::netReadyRead);
    // 网络请求完成后执行deleteLater
    connect(newReply, &QNetworkReply::finished, newReply, &QNetworkReply::deleteLater);
}

void Map::netReadyRead()
{
    // 使用qobject_cast将发送信号的对象转换为QNetworkReply *类型
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    // 追加请求到的数据
    m_array.append(reply->readAll());

    QPixmap pixmap(m_array);
    // 从获取的数据构建出有效的QPixmap对象并设置
    if(pixmap.loadFromData(m_array)) {
        m_iconLabel.setPixmap(pixmap);
    }
}

// 读取串口数据
void Map::portReadReady()
{
    // 当串口仍有数据
    while (m_port.bytesAvailable()) {
        QString line = m_port.readLine(); // 读取一行
        auto location = parseGpsData(line); // 解析GPS数据
        if (!location.isEmpty()) {
            m_location = location;
            line = "定位成功，[" + location + "]";
            requestMapImage(location); // 根据经纬度定位
        }
        else {
            line = ("定位中... 请尝试移到空旷位置，展示坐标 [" + m_location + "]");
        }

        m_logoLabel.setText(line);
    }
}



void Map::map_working()
{
    initCtrl();
    setModal(true);

    this->show();
}

void Map::on_back_clicked()
{
    emit backout();
}
// 解析GPS数据
QString Map::parseGpsData(const QString &data)
{
    QString ret;
    // 过滤无效信息
    if (data.contains(",,,,,"))
        return ret;
    // 去除输入数据字符串两端的空白字符后，按照逗号进行分割
    auto list = data.trimmed().split(',');
    if (list.count() != 15)
        return ret;
    // 判断第一个字符串是否是GPS定位数据
    if (!list.at(0).contains("$GPGGA"))
        return ret;
    // 保存原始经度数据
    bool ok;
    auto e = list.at(4).toDouble(&ok);
    if (!ok)
        return ret;
    // 保存原始纬度数据
    auto n = list.at(2).toDouble(&ok);
    if (!ok)
        return ret;
    // 计算经度的整数部分
    int i = e / 100;
    // 计算经度的小数部分
    double j = (e / 100  - i) * 100 / 60.0;
    // 拼接并保留6位小数
    ret = QString::number(i+j, 'f', 6);
    // 计算纬度的整数部分
    i = n / 100;
    // 计算纬度的小数部分
    j = (n / 100  - i) * 100 / 60.0;
    // 将经纬度拼接
    ret = ret + ',' + QString::number(i+j, 'f', 6);

    return ret;
}
