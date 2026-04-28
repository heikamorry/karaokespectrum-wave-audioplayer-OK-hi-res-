#include "lrcparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QStringConverter>
#include <algorithm>

LrcInfo LrcParser::parseFile(const QString &filePath)
{
    LrcInfo info;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return info;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString text = in.readAll();
    file.close();

    return parseText(text);
}

LrcInfo LrcParser::parseText(const QString &text)
{
    LrcInfo info;

    const QStringList rawLines = text.split(QRegularExpression(R"(\r\n|\n|\r)"),
                                            Qt::KeepEmptyParts);

    QRegularExpression timeTagRe(R"(\[(\d{1,2}):(\d{1,2})([.:](\d{1,3}))?\])");

    for (QString line : rawLines) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        if (parseMetaTag(line, info))
            continue;

        QRegularExpressionMatchIterator it = timeTagRe.globalMatch(line);
        QVector<qint64> times;
        int lastTagEnd = 0;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();

            const QString mmStr = match.captured(1);
            const QString ssStr = match.captured(2);
            const QString fracStr = match.captured(4);

            int mm = mmStr.toInt();
            int ss = ssStr.toInt();
            int ms = 0;

            if (!fracStr.isEmpty()) {
                if (fracStr.size() == 1)
                    ms = fracStr.toInt() * 100;
                else if (fracStr.size() == 2)
                    ms = fracStr.toInt() * 10;
                else
                    ms = fracStr.left(3).toInt();
            }

            qint64 timeMs = qint64(mm) * 60000 + qint64(ss) * 1000 + ms;
            timeMs += info.offsetMs;
            if (timeMs < 0)
                timeMs = 0;

            times.push_back(timeMs);
            lastTagEnd = match.capturedEnd();
        }

        if (times.isEmpty())
            continue;

        const QString lyricText = line.mid(lastTagEnd).trimmed();

        for (qint64 t : times) {
            LrcLine lyricLine;
            lyricLine.startMs = t;
            lyricLine.endMs = 0;
            lyricLine.text = lyricText;
            info.lines.push_back(lyricLine);
        }
    }

    std::sort(info.lines.begin(), info.lines.end(),
              [](const LrcLine &a, const LrcLine &b) {
                  return a.startMs < b.startMs;
              });

    return info;
}

void LrcParser::finalizeEndTimes(QVector<LrcLine> &lines, qint64 durationMs)
{
    if (lines.isEmpty())
        return;

    for (int i = 0; i < lines.size(); ++i) {
        if (i < lines.size() - 1) {
            lines[i].endMs = lines[i + 1].startMs;
        } else {
            if (durationMs > lines[i].startMs)
                lines[i].endMs = durationMs;
            else
                lines[i].endMs = lines[i].startMs + 4000;
        }

        if (lines[i].endMs < lines[i].startMs)
            lines[i].endMs = lines[i].startMs;
    }
}

bool LrcParser::parseTimeTag(const QString &tag, qint64 &timeMs)
{
    QRegularExpression re(R"(^\[(\d{1,2}):(\d{1,2})([.:](\d{1,3}))?\]$)");
    QRegularExpressionMatch match = re.match(tag);
    if (!match.hasMatch())
        return false;

    int mm = match.captured(1).toInt();
    int ss = match.captured(2).toInt();
    QString fracStr = match.captured(4);

    int ms = 0;
    if (!fracStr.isEmpty()) {
        if (fracStr.size() == 1)
            ms = fracStr.toInt() * 100;
        else if (fracStr.size() == 2)
            ms = fracStr.toInt() * 10;
        else
            ms = fracStr.left(3).toInt();
    }

    timeMs = qint64(mm) * 60000 + qint64(ss) * 1000 + ms;
    return true;
}

bool LrcParser::parseMetaTag(const QString &line, LrcInfo &info)
{
    QRegularExpression metaRe(R"(^\[(ti|ar|al|by|offset):(.*)\]$)",
                              QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = metaRe.match(line);
    if (!match.hasMatch())
        return false;

    const QString key = match.captured(1).toLower();
    const QString value = match.captured(2).trimmed();

    if (key == "ti") {
        info.title = value;
    } else if (key == "ar") {
        info.artist = value;
    } else if (key == "al") {
        info.album = value;
    } else if (key == "by") {
        info.by = value;
    } else if (key == "offset") {
        bool ok = false;
        qint64 offset = value.toLongLong(&ok);
        if (ok)
            info.offsetMs = offset;
    }

    return true;
}