#ifndef LRCPARSER_H
#define LRCPARSER_H

#include <QString>
#include <QVector>

struct LrcLine
{
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;
};

struct LrcInfo
{
    QString title;
    QString artist;
    QString album;
    QString by;
    qint64 offsetMs = 0;
    QVector<LrcLine> lines;
};

class LrcParser
{
public:
    static LrcInfo parseFile(const QString &filePath);
    static LrcInfo parseText(const QString &text);

    static void finalizeEndTimes(QVector<LrcLine> &lines, qint64 durationMs);

private:
    static bool parseTimeTag(const QString &tag, qint64 &timeMs);
    static bool parseMetaTag(const QString &line, LrcInfo &info);
};

#endif // LRCPARSER_H