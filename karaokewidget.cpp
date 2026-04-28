#include "karaokewidget.h"
#include "lyricscontroller.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QFontMetrics>
#include <QPen>
#include <QSizePolicy>
#include <QtGlobal>

KaraokeWidget::KaraokeWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
}

void KaraokeWidget::setLyricsController(LyricsController *controller)
{
    if (m_controller == controller)
        return;

    if (m_controller)
        disconnect(m_controller, nullptr, this, nullptr);

    m_controller = controller;

    if (m_controller) {
        connect(m_controller, &LyricsController::lyricsChanged,
                this, qOverload<>(&KaraokeWidget::update));
        connect(m_controller, &LyricsController::currentLineChanged,
                this, [this](int) { update(); });
        connect(m_controller, &LyricsController::progressChanged,
                this, [this](qreal) { update(); });
    }

    update();
}

LyricsController *KaraokeWidget::lyricsController() const
{
    return m_controller;
}

void KaraokeWidget::clearDisplay()
{
    update();
}

void KaraokeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    p.fillRect(rect(), QColor(12, 14, 18));

    QRect drawRect = rect().adjusted(12, 12, -12, -12);
    if (drawRect.width() <= 0 || drawRect.height() <= 0)
        return;

    p.setPen(QPen(QColor(50, 55, 65), 1));
    p.drawRect(drawRect);

    const int lineSpacing = 10;
    const int topHeight = drawRect.height() / 4;
    const int midHeight = drawRect.height() / 3;
    const int bottomHeight = drawRect.height() / 4;

    QRect prevRect(drawRect.left() + 6,
                   drawRect.top() + 6,
                   drawRect.width() - 12,
                   topHeight);

    QRect currRect(drawRect.left() + 6,
                   prevRect.bottom() + lineSpacing,
                   drawRect.width() - 12,
                   midHeight);

    QRect nextRect(drawRect.left() + 6,
                   currRect.bottom() + lineSpacing,
                   drawRect.width() - 12,
                   bottomHeight);

    QFont prevNextFont = font();
    prevNextFont.setPointSize(prevNextFont.pointSize() > 0 ? prevNextFont.pointSize() + 1 : 12);

    QFont currentFont = font();
    currentFont.setBold(true);
    currentFont.setPointSize(currentFont.pointSize() > 0 ? currentFont.pointSize() + 5 : 18);

    if (!m_controller || !m_controller->hasLyrics()) {
        drawCenteredText(p, currRect,
                         QStringLiteral("暂无歌词"),
                         QColor(150, 158, 170),
                         currentFont);
        return;
    }

    const QString prevText = m_controller->previousText();
    const QString currText = m_controller->currentText();
    const QString nextText = m_controller->nextText();
    const qreal progress = m_controller->currentProgress();

    if (!prevText.isEmpty()) {
        drawCenteredText(p, prevRect, prevText,
                         QColor(125, 132, 145),
                         prevNextFont);
    }

    if (!currText.isEmpty()) {
        drawCurrentLine(p, currRect, currText, progress, currentFont);
    } else {
        drawCenteredText(p, currRect,
                         QStringLiteral("♪"),
                         QColor(150, 158, 170),
                         currentFont);
    }

    if (!nextText.isEmpty()) {
        drawCenteredText(p, nextRect, nextText,
                         QColor(125, 132, 145),
                         prevNextFont);
    }
}

void KaraokeWidget::drawCenteredText(QPainter &p, const QRect &rect, const QString &text,
                                     const QColor &color, const QFont &font)
{
    p.save();
    p.setFont(font);
    p.setPen(color);
    p.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);
    p.restore();
}

void KaraokeWidget::drawCurrentLine(QPainter &p, const QRect &rect, const QString &text,
                                    qreal progress, const QFont &font)
{
    progress = qBound<qreal>(0.0, progress, 1.0);

    p.save();
    p.setFont(font);

    QFontMetrics fm(font);
    const int textWidth = fm.horizontalAdvance(text);
    const int textHeight = fm.height();

    QRect textRect(rect.center().x() - textWidth / 2,
                   rect.center().y() - textHeight / 2,
                   textWidth,
                   textHeight);

    p.setPen(QColor(95, 100, 110));
    p.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);

    QRect clipRect = textRect;
    clipRect.setWidth(int(textRect.width() * progress));

    p.save();
    p.setClipRect(clipRect);
    p.setPen(QColor(80, 230, 160));
    p.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);
    p.restore();

    p.restore();
}