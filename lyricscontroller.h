#ifndef LYRICSCONTROLLER_H
#define LYRICSCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>

#include "lrcparser.h"

class LyricsController : public QObject
{
    Q_OBJECT
public:
    explicit LyricsController(QObject *parent = nullptr);

    void clear();

    bool loadLrcFile(const QString &lrcFilePath);
    bool loadFromAudioFile(const QString &audioFilePath);

    void setDuration(qint64 durationMs);
    void setPosition(qint64 positionMs);

    bool hasLyrics() const;
    QString currentLrcPath() const;

    const LrcInfo &info() const;
    const QVector<LrcLine> &lines() const;

    int currentIndex() const;
    qreal currentProgress() const;
    qint64 duration() const;
    qint64 position() const;

    QString currentText() const;
    QString previousText() const;
    QString nextText() const;

signals:
    void lyricsChanged();
    void currentLineChanged(int index);
    void progressChanged(qreal progress);

private:
    QString findLrcFileForAudio(const QString &audioFilePath) const;
    int findLineIndex(qint64 positionMs) const;
    qreal computeProgress(int index, qint64 positionMs) const;

private:
    LrcInfo m_info;
    QString m_lrcFilePath;

    qint64 m_durationMs = 0;
    qint64 m_positionMs = 0;

    int m_currentIndex = -1;
    qreal m_currentProgress = 0.0;
};

#endif // LYRICSCONTROLLER_H