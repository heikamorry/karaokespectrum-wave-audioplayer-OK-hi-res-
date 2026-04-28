#include "spectrumwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <algorithm>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(180);
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

    p.fillRect(rect(), QColor(18, 20, 26));

    QRect drawRect = rect().adjusted(10, 10, -10, -10);
    if (drawRect.width() <= 0 || drawRect.height() <= 0)
        return;

    const int waveHeight = drawRect.height() * 4 / 10;
    const QRect waveRect(drawRect.left(), drawRect.top(), drawRect.width(), waveHeight);
    const QRect barsRect(drawRect.left(),
                         waveRect.bottom() + 8,
                         drawRect.width(),
                         drawRect.bottom() - (waveRect.bottom() + 8) + 1);

    p.setPen(QPen(QColor(50, 55, 65), 1));
    p.drawRect(drawRect);

    for (int i = 1; i < 4; ++i) {
        const int y = barsRect.top() + i * barsRect.height() / 4;
        p.drawLine(barsRect.left(), y, barsRect.right(), y);
    }

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

        p.setPen(QPen(QColor(220, 220, 230), 1.3));
        p.drawPath(path);

        p.setPen(QPen(QColor(70, 75, 85), 1));
        p.drawLine(waveRect.left(), waveRect.center().y(), waveRect.right(), waveRect.center().y());
    }

    if (!m_bars.isEmpty() && barsRect.height() > 0) {
        const int count = m_bars.size();
        const qreal gap = 3.0;
        const qreal totalGap = gap * (count - 1);
        const qreal barWidth = qMax<qreal>(2.0, (barsRect.width() - totalGap) / qMax(1, count));

        for (int i = 0; i < count; ++i) {
            const qreal value = std::clamp<double>(m_bars[i], 0.0, 1.0);
            const qreal h = qMax<qreal>(2.0, value * barsRect.height());

            QRectF barRect(barsRect.left() + i * (barWidth + gap),
                           barsRect.bottom() - h + 1,
                           barWidth,
                           h);

            QColor fillColor(80, 220, 160);
            if (value > 0.75)
                fillColor = QColor(255, 190, 80);
            if (value > 0.90)
                fillColor = QColor(255, 120, 100);

            p.fillRect(barRect, fillColor);
        }
    }

    p.setPen(QColor(120, 128, 140));
    p.drawText(drawRect.adjusted(8, 4, -8, -4),
               Qt::AlignTop | Qt::AlignRight,
               QString::fromUtf8("Spectrum"));
}