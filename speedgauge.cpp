#include "speedgauge.h"

#include <QPainter>
#include <QtMath>

SpeedGauge::SpeedGauge(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMinimumSize(220, 220);
}

void SpeedGauge::setSpeed(int kmh)
{
    m_speed = qBound(0, kmh, m_maxSpeed);
    update();
}

void SpeedGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int side = qMin(width(), height());
    p.translate(width() / 2.0, height() / 2.0 + side * 0.04);
    p.scale(side / 280.0, side / 280.0);

    QRectF ring(-110, -110, 220, 220);
    const int startAngle = 225 * 16;
    const int spanAngle = -270 * 16;

    p.setPen(QPen(QColor(226, 236, 247), 16, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(ring, startAngle, spanAngle);

    const double ratio = static_cast<double>(m_speed) / m_maxSpeed;
    p.setPen(QPen(QColor(56, 189, 248), 16, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(ring, startAngle, static_cast<int>(spanAngle * ratio));

    p.setPen(QPen(QColor(148, 163, 184), 2));
    for (int i = 0; i <= 9; ++i) {
        const double a = qDegreesToRadians(225.0 - i * 30.0);
        const QPointF p1(98 * qCos(a), -98 * qSin(a));
        const QPointF p2(108 * qCos(a), -108 * qSin(a));
        p.drawLine(p1, p2);
    }

    const double needleDeg = 225.0 - 270.0 * ratio;
    const double needleRad = qDegreesToRadians(needleDeg);
    p.setPen(QPen(QColor(239, 68, 68), 4, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(0, 0), QPointF(78 * qCos(needleRad), -78 * qSin(needleRad)));
    p.setBrush(QColor(239, 68, 68));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(0, 0), 7, 7);

    p.setPen(QColor(30, 64, 175));
    QFont numFont = p.font();
    numFont.setPixelSize(42);
    numFont.setBold(true);
    p.setFont(numFont);
    p.drawText(QRectF(-80, -18, 160, 50), Qt::AlignHCenter | Qt::AlignVCenter,
               QString::number(m_speed));

    QFont unitFont = p.font();
    unitFont.setPixelSize(14);
    unitFont.setBold(false);
    p.setFont(unitFont);
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRectF(-80, 28, 160, 24), Qt::AlignHCenter, QStringLiteral("km/h"));

    QFont titleFont = p.font();
    titleFont.setPixelSize(13);
    p.setFont(titleFont);
    p.drawText(QRectF(-90, -132, 180, 24), Qt::AlignHCenter, QStringLiteral("实时速度"));
}
