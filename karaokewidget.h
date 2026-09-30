#ifndef KARAOKEWIDGET_H
#define KARAOKEWIDGET_H

#include <QPointer>
#include <QWidget>

class LyricsController;

class KaraokeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KaraokeWidget(QWidget *parent = nullptr);

    void setLyricsController(LyricsController *controller);
    LyricsController *lyricsController() const;

public slots:
    void clearDisplay();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawCenteredText(QPainter &p, const QRect &rect, const QString &text,
                          const QColor &color, const QFont &font);
    void drawCurrentLine(QPainter &p, const QRect &rect, const QString &text,
                         qreal progress, const QFont &font);

private:
    QPointer<LyricsController> m_controller;
};

#endif // KARAOKEWIDGET_H
