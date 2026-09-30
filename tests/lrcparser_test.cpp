#include "lrcparser.h"
#include "lyricscontroller.h"
#include "audioanalyzer.h"

#include <QAudioFormat>
#include <QDir>
#include <QFile>
#include <QStringConverter>
#include <QTemporaryDir>
#include <QTextCodec>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <limits>

class LrcParserTest : public QObject
{
    Q_OBJECT

private slots:
    void appliesOffsetRegardlessOfMetadataOrder();
    void usesTheFinalOffsetForEveryLine();
    void parsesOnlyLeadingTimeTags();
    void supportsMultipleTagsAndLongDurations();
    void rejectsOverflowingTimestampsAndClampsOffsets();
    void finalizesLineEndTimes();
    void finalizesEndTimesWithoutOverflow();
    void decodesUtf16AndGb18030Files();
    void preservesEnergyForOppositePhaseStereo();
    void findsLrcExtensionCaseInsensitively();
};

void LrcParserTest::appliesOffsetRegardlessOfMetadataOrder()
{
    const LrcInfo info = LrcParser::parseText(
        QStringLiteral("[00:01.00]first\n[offset:500]\n[00:02.25]second"));

    QCOMPARE(info.offsetMs, 500);
    QCOMPARE(info.lines.size(), 2);
    QCOMPARE(info.lines.at(0).startMs, 1500);
    QCOMPARE(info.lines.at(1).startMs, 2750);
}

void LrcParserTest::usesTheFinalOffsetForEveryLine()
{
    const LrcInfo info = LrcParser::parseText(
        QStringLiteral("[offset:100]\n[00:01]first\n[offset:500]\n[00:02]second"));

    QCOMPARE(info.offsetMs, 500);
    QCOMPARE(info.lines.size(), 2);
    QCOMPARE(info.lines.at(0).startMs, 1500);
    QCOMPARE(info.lines.at(1).startMs, 2500);
}

void LrcParserTest::parsesOnlyLeadingTimeTags()
{
    const LrcInfo info =
        LrcParser::parseText(QStringLiteral("[00:10]Call me at [12:30]"));

    QCOMPARE(info.lines.size(), 1);
    QCOMPARE(info.lines.first().startMs, 10000);
    QCOMPARE(info.lines.first().text, QStringLiteral("Call me at [12:30]"));
}

void LrcParserTest::supportsMultipleTagsAndLongDurations()
{
    const LrcInfo info =
        LrcParser::parseText(QStringLiteral("[123:59.5][124:00.050]chorus\n[01:75]invalid"));

    QCOMPARE(info.lines.size(), 2);
    QCOMPARE(info.lines.at(0).startMs, qint64(123 * 60000 + 59500));
    QCOMPARE(info.lines.at(1).startMs, qint64(124 * 60000 + 50));
    QCOMPARE(info.lines.at(0).text, QStringLiteral("chorus"));
    QCOMPARE(info.lines.at(1).text, QStringLiteral("chorus"));
}

void LrcParserTest::rejectsOverflowingTimestampsAndClampsOffsets()
{
    const LrcInfo overflow =
        LrcParser::parseText(QStringLiteral("[999999999999999999999:00]invalid"));
    QVERIFY(overflow.lines.isEmpty());

    const LrcInfo offset = LrcParser::parseText(
        QStringLiteral("[offset:9223372036854775807]\n[00:00]line"));
    QCOMPARE(offset.offsetMs, qint64(24 * 60 * 60 * 1000));
    QCOMPARE(offset.lines.size(), 1);
    QCOMPARE(offset.lines.first().startMs, offset.offsetMs);
}

void LrcParserTest::finalizesLineEndTimes()
{
    QVector<LrcLine> lines = {
        {1000, 0, QStringLiteral("one")},
        {2500, 0, QStringLiteral("two")}
    };
    LrcParser::finalizeEndTimes(lines, 5000);

    QCOMPARE(lines.at(0).endMs, 2500);
    QCOMPARE(lines.at(1).endMs, 5000);
}

void LrcParserTest::finalizesEndTimesWithoutOverflow()
{
    QVector<LrcLine> lines = {
        {std::numeric_limits<qint64>::max() - 1000, 0, QStringLiteral("last")}
    };
    LrcParser::finalizeEndTimes(lines, 0);
    QCOMPARE(lines.first().endMs, std::numeric_limits<qint64>::max());
}

void LrcParserTest::decodesUtf16AndGb18030Files()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString source = QStringLiteral("[00:00.00]中文歌词");

    QStringEncoder utf16Encoder(QStringEncoder::Utf16LE);
    QFile utf16File(directory.filePath(QStringLiteral("utf16.lrc")));
    QVERIFY(utf16File.open(QIODevice::WriteOnly));
    utf16File.write(QByteArray::fromHex("fffe") + utf16Encoder.encode(source));
    utf16File.close();
    const LrcInfo utf16Info = LrcParser::parseFile(utf16File.fileName());
    QCOMPARE(utf16Info.lines.size(), 1);
    QCOMPARE(utf16Info.lines.first().text, QStringLiteral("中文歌词"));

    QTextCodec *gb18030 = QTextCodec::codecForName("GB18030");
    QVERIFY(gb18030);
    QFile gbFile(directory.filePath(QStringLiteral("gb18030.lrc")));
    QVERIFY(gbFile.open(QIODevice::WriteOnly));
    gbFile.write(gb18030->fromUnicode(source));
    gbFile.close();
    const LrcInfo gbInfo = LrcParser::parseFile(gbFile.fileName());
    QCOMPARE(gbInfo.lines.size(), 1);
    QCOMPARE(gbInfo.lines.first().text, QStringLiteral("中文歌词"));
}

void LrcParserTest::preservesEnergyForOppositePhaseStereo()
{
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    constexpr int frameCount = 2048;
    QByteArray data(frameCount * 2 * int(sizeof(qint16)), '\0');
    auto *samples = reinterpret_cast<qint16 *>(data.data());
    for (int frame = 0; frame < frameCount; ++frame) {
        const auto sample = static_cast<qint16>(
            std::sin(2.0 * 3.141592653589793 * 440.0 * frame / 48000.0) * 12000.0);
        samples[frame * 2] = sample;
        samples[frame * 2 + 1] = -sample;
    }

    AudioAnalyzer analyzer;
    QSignalSpy spy(&analyzer, &AudioAnalyzer::spectrumReady);
    QTest::qWait(20);
    analyzer.processBuffer(QAudioBuffer(data, format));

    QCOMPARE(spy.size(), 1);
    const QVector<float> bars =
        qvariant_cast<QVector<float>>(spy.first().at(0));
    const QVector<float> waveform =
        qvariant_cast<QVector<float>>(spy.first().at(1));
    QVERIFY(!bars.isEmpty());
    QVERIFY(!waveform.isEmpty());
    QVERIFY(std::any_of(bars.cbegin(), bars.cend(),
                        [](float value) { return value > 0.0f; }));
    QVERIFY(std::any_of(waveform.cbegin(), waveform.cend(),
                        [](float value) { return std::fabs(value) > 0.1f; }));
}

void LrcParserTest::findsLrcExtensionCaseInsensitively()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString audioPath = directory.filePath(QStringLiteral("My Song.mp3"));
    QFile audioFile(audioPath);
    QVERIFY(audioFile.open(QIODevice::WriteOnly));
    audioFile.close();

    QFile lrcFile(directory.filePath(QStringLiteral("my song.LRC")));
    QVERIFY(lrcFile.open(QIODevice::WriteOnly));
    lrcFile.write("[00:00.00]hello");
    lrcFile.close();

    LyricsController controller;
    QVERIFY(controller.loadFromAudioFile(audioPath));
    QCOMPARE(controller.lines().size(), 1);
    QCOMPARE(controller.lines().first().endMs, 4000);
    QVERIFY(QFileInfo::exists(controller.currentLrcPath()));
    QCOMPARE(QDir::cleanPath(controller.currentLrcPath()).toCaseFolded(),
             QDir::cleanPath(lrcFile.fileName()).toCaseFolded());
}

QTEST_MAIN(LrcParserTest)
#include "lrcparser_test.moc"
