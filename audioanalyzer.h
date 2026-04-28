#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QObject>
#include <QVector>
#include <QtMultimedia/QAudioBuffer>


class AudioAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit AudioAnalyzer(QObject *parent = nullptr);
    ~AudioAnalyzer() override;

    void setFftSize(int fftSize);
    void setBarCount(int barCount);
    void setWaveformPointCount(int points);

public slots:
    void processBuffer(const QAudioBuffer &buffer);
    void reset();

signals:
    void spectrumReady(const QVector<float> &bars, const QVector<float> &waveform);

private:
    QVector<float> extractMonoSamples(const QAudioBuffer &buffer) const;
    QVector<float> downsampleWaveform(const QVector<float> &samples, int targetCount) const;
    void ensureFft();
    QVector<float> computeBars(const QVector<float> &fftInput, int sampleRate);
    void appendSamples(const QVector<float> &samples);

private:
    int m_fftSize;
    int m_barCount;
    int m_waveformPointCount;

    QVector<float> m_sampleBuffer;
    QVector<float> m_prevBars;

    struct KissFftRealHandle;
    KissFftRealHandle *m_fftHandle;
};

#endif // AUDIOANALYZER_H