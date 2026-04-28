#include "lyricscontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QtGlobal>

LyricsController::LyricsController(QObject *parent)
    : QObject(parent)
{
}

void LyricsController::clear()
{
    m_info = LrcInfo{};
    m_lrcFilePath.clear();
    m_durationMs = 0;
    m_positionMs = 0;
    m_currentIndex = -1;
    m_currentProgress = 0.0;

    emit lyricsChanged();
    emit currentLineChanged(-1);
    emit progressChanged(0.0);
}

bool LyricsController::loadLrcFile(const QString &lrcFilePath)
{
    if (lrcFilePath.isEmpty()) {
        clear();
        return false;
    }

    LrcInfo parsed = LrcParser::parseFile(lrcFilePath);
    if (parsed.lines.isEmpty()) {
        clear();
        return false;
    }

    if (m_durationMs > 0)
        LrcParser::finalizeEndTimes(parsed.lines, m_durationMs);

    m_info = parsed;
    m_lrcFilePath = lrcFilePath;
    m_currentIndex = -1;
    m_currentProgress = 0.0;

    emit lyricsChanged();

    setPosition(m_positionMs);
    return true;
}

bool LyricsController::loadFromAudioFile(const QString &audioFilePath)
{
    const QString lrcPath = findLrcFileForAudio(audioFilePath);
    if (lrcPath.isEmpty()) {
        clear();
        return false;
    }

    return loadLrcFile(lrcPath);
}

void LyricsController::setDuration(qint64 durationMs)
{
    if (durationMs < 0)
        durationMs = 0;

    m_durationMs = durationMs;

    if (!m_info.lines.isEmpty())
        LrcParser::finalizeEndTimes(m_info.lines, m_durationMs);

    setPosition(m_positionMs);
}

void LyricsController::setPosition(qint64 positionMs)
{
    if (positionMs < 0)
        positionMs = 0;

    m_positionMs = positionMs;

    const int newIndex = findLineIndex(positionMs);
    const qreal newProgress = computeProgress(newIndex, positionMs);

    const bool indexChanged = (newIndex != m_currentIndex);
    const bool progressReallyChanged =
        !qFuzzyCompare(m_currentProgress + 1.0, newProgress + 1.0);

    m_currentIndex = newIndex;
    m_currentProgress = newProgress;

    if (indexChanged)
        emit currentLineChanged(m_currentIndex);

    if (indexChanged || progressReallyChanged)
        emit progressChanged(m_currentProgress);
}

bool LyricsController::hasLyrics() const
{
    return !m_info.lines.isEmpty();
}

QString LyricsController::currentLrcPath() const
{
    return m_lrcFilePath;
}

const LrcInfo &LyricsController::info() const
{
    return m_info;
}

const QVector<LrcLine> &LyricsController::lines() const
{
    return m_info.lines;
}

int LyricsController::currentIndex() const
{
    return m_currentIndex;
}

qreal LyricsController::currentProgress() const
{
    return m_currentProgress;
}

qint64 LyricsController::duration() const
{
    return m_durationMs;
}

qint64 LyricsController::position() const
{
    return m_positionMs;
}

QString LyricsController::currentText() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_info.lines.size())
        return m_info.lines[m_currentIndex].text;
    return {};
}

QString LyricsController::previousText() const
{
    const int idx = m_currentIndex - 1;
    if (idx >= 0 && idx < m_info.lines.size())
        return m_info.lines[idx].text;
    return {};
}

QString LyricsController::nextText() const
{
    const int idx = m_currentIndex + 1;
    if (idx >= 0 && idx < m_info.lines.size())
        return m_info.lines[idx].text;
    return {};
}

QString LyricsController::findLrcFileForAudio(const QString &audioFilePath) const
{
    QFileInfo fi(audioFilePath);
    if (!fi.exists())
        return {};

    const QString dirPath = fi.absolutePath();
    const QString baseName = fi.completeBaseName();

    const QString lrcPath = QDir(dirPath).filePath(baseName + ".lrc");
    if (QFileInfo::exists(lrcPath))
        return lrcPath;

    return {};
}

int LyricsController::findLineIndex(qint64 positionMs) const
{
    const QVector<LrcLine> &ls = m_info.lines;
    if (ls.isEmpty())
        return -1;

    if (positionMs < ls.first().startMs)
        return -1;

    int left = 0;
    int right = ls.size() - 1;
    int ans = -1;

    while (left <= right) {
        const int mid = (left + right) / 2;
        if (ls[mid].startMs <= positionMs) {
            ans = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return ans;
}

qreal LyricsController::computeProgress(int index, qint64 positionMs) const
{
    if (index < 0 || index >= m_info.lines.size())
        return 0.0;

    const LrcLine &line = m_info.lines[index];

    if (line.endMs <= line.startMs)
        return 0.0;

    qreal ratio = qreal(positionMs - line.startMs) / qreal(line.endMs - line.startMs);

    if (ratio < 0.0)
        ratio = 0.0;
    if (ratio > 1.0)
        ratio = 1.0;

    return ratio;
}