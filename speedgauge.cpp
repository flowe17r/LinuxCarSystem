#include "speedgauge.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QtMath>

namespace {
// 圆环角度约定：起始 225°（左上），顺时针扫 270°，到 -45°（右下），底部留缺口
const double kStartDeg = 225.0;
const double kEndDeg = -45.0;
const double kSpanDeg = 270.0;
const int kMaxSpeed = 180;

double ratioOf(int kmh)
{
    return qBound(0, kmh, kMaxSpeed) / double(kMaxSpeed);
}

// 把数学角规整到 (-135, 225]，再折成圆环进度；底部缺口 (-135,-45) 取最近端点
double ratioFromAngle(double deg)
{
    while (deg > kStartDeg)
        deg -= 360.0;
    while (deg <= kStartDeg - 360.0)
        deg += 360.0;

    if (deg < kEndDeg)
        return (qAbs(deg - kStartDeg) < qAbs(deg - kEndDeg)) ? 0.0 : 1.0;
    return qBound(0.0, (kStartDeg - deg) / kSpanDeg, 1.0);
}
}

SpeedGauge::SpeedGauge(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMinimumSize(220, 220);
    setCursor(Qt::PointingHandCursor);
}

void SpeedGauge::setSpeed(int kmh)
{
    const int clamped = qBound(0, kmh, kMaxSpeed);
    if (clamped == m_speed)
        return;
    m_speed = clamped;
    update();
    emit speedChanged(m_speed);
}

// 把触点换算成圆环上的角度，再映射成时速（底部缺口内取最近的一端）
void SpeedGauge::setSpeedFromPos(const QPoint &pos)
{
    const QPointF center(width() / 2.0, height() / 2.0);
    const double dx = pos.x() - center.x();
    const double dy = pos.y() - center.y();
    const double reach = qMin(width(), height()) * 0.18;
    if (qSqrt(dx * dx + dy * dy) < reach)
        return; // 圆心区域是数字，不参与拖动

    // 数学角：0° 在右，逆时针为正；屏幕 y 轴向下需取反
    const double deg = qRadiansToDegrees(qAtan2(-dy, dx)); // (-180, 180]
    setSpeed(qRound(ratioFromAngle(deg) * kMaxSpeed));
}

void SpeedGauge::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setSpeedFromPos(event->pos());
        event->accept();
    }
}

void SpeedGauge::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        setSpeedFromPos(event->pos());
        event->accept();
    }
}

void SpeedGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const int side = qMin(width(), height());
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 280.0, side / 280.0);

    const double ratio = ratioOf(m_speed);
    const int startAngle = int(kStartDeg * 16);
    const int spanAngle = -int(kSpanDeg * 16 * ratio);

    const QRectF ring(-100, -100, 200, 200);
    const QRectF outerRing(-124, -124, 248, 248);

    // 1. 最外层细圈
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(44, 64, 95), 1.5));
    p.drawEllipse(outerRing);

    // 2. 未走过的轨道（完整 270° 底环）
    p.setPen(QPen(QColor(30, 45, 72), 12, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(ring, startAngle, -int(kSpanDeg * 16));

    // 3. 高速警示区（85%~100%）
    p.setPen(QPen(QColor(127, 29, 29, 150), 12, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(ring, int((kStartDeg - kSpanDeg * 0.85) * 16), -int(kSpanDeg * 0.15 * 16));

    // 4. 进度弧：青→紫渐变
    QLinearGradient grad(ring.topLeft(), ring.bottomRight());
    grad.setColorAt(0.0, QColor(56, 189, 248));
    grad.setColorAt(0.55, QColor(129, 140, 248));
    grad.setColorAt(1.0, QColor(167, 139, 250));
    p.setPen(QPen(grad, 12, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(ring, startAngle, spanAngle);

    // 5. 刻度：每 20 km/h 一条主刻度并在环外标数字，中间一条细刻度
    QFont tickFont = p.font();
    tickFont.setPixelSize(11);
    tickFont.setBold(false);
    for (int v = 0; v <= kMaxSpeed; v += 10) {
        const double deg = kStartDeg - kSpanDeg * (v / double(kMaxSpeed));
        const double rad = qDegreesToRadians(deg);
        const bool major = (v % 20 == 0);
        const double inner = major ? 72.0 : 78.0;
        const double outer = 88.0;
        p.setPen(QPen(major ? QColor(148, 163, 184) : QColor(71, 85, 105),
                      major ? 2.0 : 1.0));
        p.drawLine(QPointF(inner * qCos(rad), -inner * qSin(rad)),
                   QPointF(outer * qCos(rad), -outer * qSin(rad)));
        if (major) {
            p.setFont(tickFont);
            p.setPen(QColor(148, 163, 184));
            const QPointF label(115 * qCos(rad), -115 * qSin(rad));
            p.drawText(QRectF(label.x() - 16, label.y() - 9, 32, 18),
                       Qt::AlignCenter, QString::number(v));
        }
    }

    // 6. 指针：细长菱形 + 中心轮毂
    const double needleDeg = kStartDeg - kSpanDeg * ratio;
    const double needleRad = qDegreesToRadians(needleDeg);
    const QPointF tip(70 * qCos(needleRad), -70 * qSin(needleRad));
    const QPointF perp(qSin(needleRad) * 3.2, qCos(needleRad) * 3.2);
    QPainterPath needle;
    needle.moveTo(tip);
    needle.lineTo(perp);
    needle.lineTo(-perp * 1.6);
    needle.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 113, 113));
    p.drawPath(needle);

    // 小轮毂：只做轴心点缀，不压住数字
    p.setBrush(QColor(11, 21, 38));
    p.setPen(QPen(QColor(248, 113, 113), 1.5));
    p.drawEllipse(QPointF(0, 0), 7, 7);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 113, 113));
    p.drawEllipse(QPointF(0, 0), 2.5, 2.5);

    // 7. 数字下移到圆心下方，避开指针扫过的区域
    QFont numFont = p.font();
    numFont.setPixelSize(44);
    numFont.setBold(true);
    p.setFont(numFont);
    p.setPen(QColor(241, 245, 249));
    p.drawText(QRectF(-80, 10, 160, 48), Qt::AlignCenter, QString::number(m_speed));

    QFont unitFont = p.font();
    unitFont.setPixelSize(13);
    unitFont.setBold(false);
    p.setFont(unitFont);
    p.setPen(QColor(148, 163, 184));
    p.drawText(QRectF(-80, 56, 160, 18), Qt::AlignCenter, QStringLiteral("km/h"));

    QFont hintFont = p.font();
    hintFont.setPixelSize(11);
    p.setFont(hintFont);
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRectF(-80, 74, 160, 16), Qt::AlignCenter,
               QStringLiteral("拖动圆环调节"));
}
