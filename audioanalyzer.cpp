#include "audioanalyzer.h"

#include <QtMultimedia/QAudioFormat>
#include <QDebug>
#include <algorithm>
#include <cmath>

extern "C" {
#include <kissfft/kiss_fftr.h>
}

struct AudioAnalyzer::KissFftRealHandle
{
    kiss_fftr_cfg cfg = nullptr;
    int fftSize = 0;
};

namespace {
constexpr float kPi = 3.14159265358979323846f;

static inline float clampFloat(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}
}

AudioAnalyzer::AudioAnalyzer(QObject *parent)
    : QObject(parent),
    m_fftSize(2048),
    m_barCount(48),
    m_waveformPointCount(256),
    m_fftHandle(new KissFftRealHandle)
{
    ensureFft();
}

AudioAnalyzer::~AudioAnalyzer()
{
    if (m_fftHandle) {
        if (m_fftHandle->cfg) {
            kiss_fftr_free(m_fftHandle->cfg);
            m_fftHandle->cfg = nullptr;
        }
        delete m_fftHandle;
        m_fftHandle = nullptr;
    }
}

void AudioAnalyzer::setFftSize(int fftSize)
{
    if (fftSize < 256 || (fftSize % 2) != 0)
        return;

    if (m_fftSize == fftSize)
        return;

    m_fftSize = fftSize;
    ensureFft();
    m_sampleBuffer.clear();
    m_prevBars.clear();
}

void AudioAnalyzer::setBarCount(int barCount)
{
    if (barCount <= 0)
        return;

    m_barCount = barCount;
    m_prevBars.clear();
}

void AudioAnalyzer::setWaveformPointCount(int points)
{
    if (points <= 8)
        return;

    m_waveformPointCount = points;
}

void AudioAnalyzer::reset()
{
    m_sampleBuffer.clear();
    m_prevBars.clear();

    QVector<float> emptyBars;
    QVector<float> emptyWave;
    emit spectrumReady(emptyBars, emptyWave);
}

void AudioAnalyzer::ensureFft()
{
    if (!m_fftHandle)
        return;

    if (m_fftHandle->cfg && m_fftHandle->fftSize == m_fftSize)
        return;

    if (m_fftHandle->cfg) {
        kiss_fftr_free(m_fftHandle->cfg);
        m_fftHandle->cfg = nullptr;
    }

    m_fftHandle->cfg = kiss_fftr_alloc(m_fftSize, 0, nullptr, nullptr);
    m_fftHandle->fftSize = m_fftSize;
}

QVector<float> AudioAnalyzer::extractMonoSamples(const QAudioBuffer &buffer) const
{
    QVector<float> mono;

    if (!buffer.isValid())
        return mono;

    const QAudioFormat fmt = buffer.format();
    const int channels = fmt.channelCount();
    const int frames = buffer.frameCount();

    if (channels <= 0 || frames <= 0)
        return mono;

    mono.resize(frames);

    switch (fmt.sampleFormat()) {
    case QAudioFormat::UInt8: {
        const quint8 *p = buffer.constData<quint8>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                const quint8 v = p[i * channels + ch];
                sum += (float(v) - 128.0f) / 128.0f;
            }
            mono[i] = clampFloat(sum / float(channels), -1.0f, 1.0f);
        }
        break;
    }
    case QAudioFormat::Int16: {
        const qint16 *p = buffer.constData<qint16>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                sum += float(p[i * channels + ch]) / 32768.0f;
            }
            mono[i] = clampFloat(sum / float(channels), -1.0f, 1.0f);
        }
        break;
    }
    case QAudioFormat::Int32: {
        const qint32 *p = buffer.constData<qint32>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                sum += float(double(p[i * channels + ch]) / 2147483648.0);
            }
            mono[i] = clampFloat(sum / float(channels), -1.0f, 1.0f);
        }
        break;
    }
    case QAudioFormat::Float: {
        const float *p = buffer.constData<float>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                sum += p[i * channels + ch];
            }
            mono[i] = clampFloat(sum / float(channels), -1.0f, 1.0f);
        }
        break;
    }
    default:
        mono.clear();
        break;
    }

    return mono;
}

QVector<float> AudioAnalyzer::downsampleWaveform(const QVector<float> &samples, int targetCount) const
{
    QVector<float> out;
    if (samples.isEmpty() || targetCount <= 0)
        return out;

    if (samples.size() <= targetCount)
        return samples;

    out.resize(targetCount);
    const float step = float(samples.size()) / float(targetCount);

    for (int i = 0; i < targetCount; ++i) {
        const int sampleCount = int(samples.size());
        const int start = std::min(int(i * step), sampleCount - 1);
        const int end   = std::min(int((i + 1) * step), sampleCount);

        if (end <= start) {
            out[i] = samples[start];
            continue;
        }

        float peak = 0.0f;
        for (int j = start; j < end; ++j) {
            peak = std::max(peak, std::fabs(samples[j]));
        }

        float signSample = samples[start];
        out[i] = (signSample >= 0.0f) ? peak : -peak;
    }

    return out;
}

void AudioAnalyzer::appendSamples(const QVector<float> &samples)
{
    if (samples.isEmpty())
        return;

    m_sampleBuffer += samples;

    const int maxKeep = std::max(m_fftSize * 4, 8192);
    if (m_sampleBuffer.size() > maxKeep) {
        const int removeCount = m_sampleBuffer.size() - maxKeep;
        m_sampleBuffer.remove(0, removeCount);
    }
}

QVector<float> AudioAnalyzer::computeBars(const QVector<float> &fftInput, int sampleRate)
{
    QVector<float> bars;
    if (!m_fftHandle || !m_fftHandle->cfg || fftInput.size() != m_fftSize || sampleRate <= 0)
        return bars;

    QVector<kiss_fft_scalar> timedata(m_fftSize);
    QVector<kiss_fft_cpx> freqdata(m_fftSize / 2 + 1);

    for (int i = 0; i < m_fftSize; ++i) {
        const float hann = 0.5f - 0.5f * std::cos((2.0f * kPi * i) / float(m_fftSize - 1));
        timedata[i] = fftInput[i] * hann;
    }

    kiss_fftr(m_fftHandle->cfg, timedata.data(), freqdata.data());

    QVector<float> mags(m_fftSize / 2 + 1, 0.0f);
    for (int i = 0; i < mags.size(); ++i) {
        const float re = freqdata[i].r;
        const float im = freqdata[i].i;
        const float mag = std::sqrt(re * re + im * im) / float(m_fftSize);
        mags[i] = mag;
    }

    bars.resize(m_barCount);
    bars.fill(0.0f);

    const float minHz = 20.0f;
    const float maxHz = std::min(16000.0f, sampleRate * 0.5f);

    for (int i = 0; i < m_barCount; ++i) {
        const float ratio1 = float(i) / float(m_barCount);
        const float ratio2 = float(i + 1) / float(m_barCount);

        const float hz1 = minHz * std::pow(maxHz / minHz, ratio1);
        const float hz2 = minHz * std::pow(maxHz / minHz, ratio2);

        int bin1 = int(hz1 / sampleRate * m_fftSize);
        int bin2 = int(hz2 / sampleRate * m_fftSize);

        const int magCount = int(mags.size());
        bin1 = std::clamp(bin1, 0, magCount - 1);
        bin2 = std::clamp(bin2, bin1 + 1, magCount);

        float sum = 0.0f;
        for (int b = bin1; b < bin2; ++b)
            sum += mags[b];

        float avg = sum / float(bin2 - bin1);

        float db = 20.0f * std::log10(avg + 1e-6f);
        float normalized = (db + 60.0f) / 60.0f;
        normalized = clampFloat(normalized, 0.0f, 1.0f);

        bars[i] = normalized;
    }

    if (m_prevBars.size() != bars.size())
        m_prevBars = QVector<float>(bars.size(), 0.0f);

    for (int i = 0; i < bars.size(); ++i) {
        const float prev = m_prevBars[i];
        const float current = bars[i];
        const float alpha = (current > prev) ? 0.35f : 0.18f;
        bars[i] = prev * (1.0f - alpha) + current * alpha;
    }

    m_prevBars = bars;
    return bars;
}

void AudioAnalyzer::processBuffer(const QAudioBuffer &buffer)
{
    if (!buffer.isValid())
        return;

    const QVector<float> mono = extractMonoSamples(buffer);
    if (mono.isEmpty())
        return;

    appendSamples(mono);

    QVector<float> waveform = downsampleWaveform(mono, m_waveformPointCount);

    if (m_sampleBuffer.size() < m_fftSize) {
        QVector<float> emptyBars;
        emit spectrumReady(emptyBars, waveform);
        return;
    }

    QVector<float> fftInput(m_fftSize);
    const int start = m_sampleBuffer.size() - m_fftSize;
    for (int i = 0; i < m_fftSize; ++i)
        fftInput[i] = m_sampleBuffer[start + i];

    const int sampleRate = buffer.format().sampleRate();
    QVector<float> bars = computeBars(fftInput, sampleRate);

    emit spectrumReady(bars, waveform);
}