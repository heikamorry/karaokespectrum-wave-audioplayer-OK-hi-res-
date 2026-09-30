#include "spectrumwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <algorithm>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(72);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
}

void SpectrumWidget::setSpectrumData(const QVector<float> &bars, const QVector<float> &waveform)
{
    m_bars = bars;
    m_waveform = waveform;
    update();
}

void SpectrumWidget::clearData()
{
    m_bars.clear();
    m_waveform.clear();
    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect drawRect = rect().adjusted(6, 6, -6, -6);
    if (drawRect.width() <= 0 || drawRect.height() <= 0)
        return;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(247, 247, 250));
    p.drawRoundedRect(drawRect, 10, 10);

    const int waveHeight = drawRect.height() * 35 / 100;
    const QRect waveRect(drawRect.left(), drawRect.top(), drawRect.width(), waveHeight);
    const QRect barsRect(drawRect.left(),
                         waveRect.bottom() + 5,
                         drawRect.width(),
                         drawRect.bottom() - (waveRect.bottom() + 5) + 1);

    if (!m_waveform.isEmpty()) {
        QPainterPath path;
        const int n = m_waveform.size();

        for (int i = 0; i < n; ++i) {
            const qreal x = waveRect.left() + qreal(i) / qMax(1, n - 1) * waveRect.width();
            const qreal amp = std::clamp<double>(m_waveform[i], -1.0, 1.0);
            const qreal y = waveRect.center().y() - amp * (waveRect.height() * 0.42);
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }

        p.setPen(QPen(QColor(250, 45, 85, 145), 1.35));
        p.drawPath(path);

        p.setPen(QPen(QColor(199, 199, 204, 125), 1));
        p.drawLine(waveRect.left(), waveRect.center().y(), waveRect.right(), waveRect.center().y());
    }

    if (!m_bars.isEmpty() && barsRect.height() > 0) {
        const int count = m_bars.size();
        const qreal gap = qBound<qreal>(1.0, barsRect.width() / qreal(count * 5), 2.5);
        const qreal totalGap = gap * (count - 1);
        const qreal barWidth =
            qMax<qreal>(0.8, (barsRect.width() - totalGap) / qMax(1, count));

        for (int i = 0; i < count; ++i) {
            const qreal value = std::clamp<double>(m_bars[i], 0.0, 1.0);
            const qreal h = qMax<qreal>(2.0, value * barsRect.height());

            QRectF barRect(barsRect.left() + i * (barWidth + gap),
                           barsRect.bottom() - h + 1,
                           barWidth,
                           h);

            QColor fillColor(250, 45, 85, 190);
            if (value > 0.75)
                fillColor = QColor(255, 55, 95);
            if (value > 0.90)
                fillColor = QColor(255, 105, 135);

            p.setBrush(fillColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(barRect, barWidth / 2.0, barWidth / 2.0);
        }
    }
}
