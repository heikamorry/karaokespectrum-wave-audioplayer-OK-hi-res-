#include "lrcparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextCodec>
#include <algorithm>
#include <limits>

namespace {

constexpr qint64 kMaxLrcTimestampMs = 365LL * 24 * 60 * 60 * 1000;
constexpr qint64 kMaxOffsetMs = 24LL * 60 * 60 * 1000;

bool parseTimestampParts(const QString &minutesText,
                         const QString &secondsText,
                         const QString &fractionText,
                         qint64 &timeMs)
{
    bool minutesOk = false;
    const qint64 minutes = minutesText.toLongLong(&minutesOk);
    if (!minutesOk || minutes < 0)
        return false;

    bool secondsOk = false;
    const int seconds = secondsText.toInt(&secondsOk);
    if (!secondsOk || seconds < 0 || seconds > 59)
        return false;

    int milliseconds = 0;
    if (!fractionText.isEmpty()) {
        if (fractionText.size() == 1)
            milliseconds = fractionText.toInt() * 100;
        else if (fractionText.size() == 2)
            milliseconds = fractionText.toInt() * 10;
        else
            milliseconds = fractionText.left(3).toInt();
    }

    const qint64 subMinuteMs = qint64(seconds) * 1000 + milliseconds;
    if (minutes > (kMaxLrcTimestampMs - subMinuteMs) / 60000)
        return false;

    timeMs = minutes * 60000 + subMinuteMs;
    return true;
}

qint64 applyOffset(qint64 timeMs, qint64 offsetMs)
{
    return std::clamp(timeMs + offsetMs, qint64(0), kMaxLrcTimestampMs);
}

QString decodeLrcBytes(const QByteArray &bytes)
{
    const auto byteAt = [&bytes](int index) {
        return static_cast<unsigned char>(bytes.at(index));
    };

    if (bytes.size() >= 2 && byteAt(0) == 0xff && byteAt(1) == 0xfe) {
        QStringDecoder decoder(QStringDecoder::Utf16LE);
        return decoder.decode(bytes.mid(2));
    }
    if (bytes.size() >= 2 && byteAt(0) == 0xfe && byteAt(1) == 0xff) {
        QStringDecoder decoder(QStringDecoder::Utf16BE);
        return decoder.decode(bytes.mid(2));
    }

    const int utf8BomLength =
        bytes.size() >= 3 && byteAt(0) == 0xef && byteAt(1) == 0xbb && byteAt(2) == 0xbf
        ? 3
        : 0;
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    const QString utf8Text = utf8Decoder.decode(bytes.mid(utf8BomLength));
    if (!utf8Decoder.hasError())
        return utf8Text;

    if (QTextCodec *gb18030 = QTextCodec::codecForName("GB18030"))
        return gb18030->toUnicode(bytes);
    return QString::fromLocal8Bit(bytes);
}

} // namespace

LrcInfo LrcParser::parseFile(const QString &filePath)
{
    LrcInfo info;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return info;

    const QByteArray bytes = file.readAll();
    file.close();

    return parseText(decodeLrcBytes(bytes));
}

LrcInfo LrcParser::parseText(const QString &text)
{
    LrcInfo info;

    const QStringList rawLines = text.split(QRegularExpression(R"(\r\n|\n|\r)"),
                                            Qt::KeepEmptyParts);

    // Metadata, especially [offset], is allowed anywhere in an LRC file.
    // Resolve it before parsing any timestamps so every line receives the
    // same final offset.
    for (QString line : rawLines)
        parseMetaTag(line.trimmed(), info);

    QRegularExpression timeTagRe(
        R"(\[(\d+):([0-5]?\d)([.:](\d{1,3}))?\])");

    for (QString line : rawLines) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        LrcInfo ignoredMeta;
        if (parseMetaTag(line, ignoredMeta))
            continue;

        QVector<qint64> times;
        int consumed = 0;

        // Only consume the contiguous timestamp tags at the beginning of the
        // line. Bracketed times inside the lyric text remain lyric text.
        while (consumed < line.size()) {
            const QRegularExpressionMatch match =
                timeTagRe.match(line, consumed,
                                QRegularExpression::NormalMatch,
                                QRegularExpression::AnchorAtOffsetMatchOption);
            if (!match.hasMatch())
                break;

            const QString fracStr = match.captured(4);
            qint64 rawTimeMs = 0;
            if (!parseTimestampParts(match.captured(1), match.captured(2),
                                     fracStr, rawTimeMs)) {
                times.clear();
                break;
            }

            times.push_back(applyOffset(rawTimeMs, info.offsetMs));
            consumed = match.capturedEnd();
        }

        if (times.isEmpty())
            continue;

        const QString lyricText = line.mid(consumed).trimmed();

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
                lines[i].endMs =
                    lines[i].startMs > std::numeric_limits<qint64>::max() - 4000
                    ? std::numeric_limits<qint64>::max()
                    : lines[i].startMs + 4000;
        }

        if (lines[i].endMs < lines[i].startMs)
            lines[i].endMs = lines[i].startMs;
    }
}

bool LrcParser::parseTimeTag(const QString &tag, qint64 &timeMs)
{
    QRegularExpression re(R"(^\[(\d+):([0-5]?\d)([.:](\d{1,3}))?\]$)");
    QRegularExpressionMatch match = re.match(tag);
    if (!match.hasMatch())
        return false;

    return parseTimestampParts(match.captured(1), match.captured(2),
                               match.captured(4), timeMs);
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
            info.offsetMs = std::clamp(offset, -kMaxOffsetMs, kMaxOffsetMs);
    }

    return true;
}
