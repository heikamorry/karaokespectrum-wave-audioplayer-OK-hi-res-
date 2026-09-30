#include "audioanalyzer.h"

#include <QtMultimedia/QAudioFormat>
#include <algorithm>
#include <cmath>
#include <complex>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr int kMaxFftSize = 65536;
constexpr int kMaxBarCount = 512;
constexpr int kMaxWaveformPointCount = 8192;

static inline float clampFloat(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}

template<typename SampleAt>
float energyPreservingDownmix(int channels, SampleAt sampleAt)
{
    float sumSquares = 0.0f;
    float strongestSample = 0.0f;
    for (int channel = 0; channel < channels; ++channel) {
        float sample = sampleAt(channel);
        if (!std::isfinite(sample))
            sample = 0.0f;
        sample = clampFloat(sample, -1.0f, 1.0f);
        sumSquares += sample * sample;
        if (std::fabs(sample) > std::fabs(strongestSample))
            strongestSample = sample;
    }

    const float rms = std::sqrt(sumSquares / float(channels));
    return std::copysign(clampFloat(rms, 0.0f, 1.0f),
                         strongestSample == 0.0f ? 1.0f : strongestSample);
}

bool isPowerOfTwo(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

void radix2Fft(QVector<std::complex<float>> &values)
{
    const int size = values.size();

    for (int i = 1, reversed = 0; i < size; ++i) {
        int bit = size >> 1;
        while (reversed & bit) {
            reversed ^= bit;
            bit >>= 1;
        }
        reversed ^= bit;

        if (i < reversed)
            std::swap(values[i], values[reversed]);
    }

    for (int length = 2; length <= size;) {
        const float angle = -2.0f * kPi / float(length);
        const std::complex<float> phaseStep(std::cos(angle), std::sin(angle));
        const int halfLength = length / 2;

        for (int offset = 0; offset < size; offset += length) {
            std::complex<float> phase(1.0f, 0.0f);
            for (int i = 0; i < halfLength; ++i) {
                const std::complex<float> even = values[offset + i];
                const std::complex<float> odd = values[offset + i + halfLength] * phase;
                values[offset + i] = even + odd;
                values[offset + i + halfLength] = even - odd;
                phase *= phaseStep;
            }
        }

        if (length == size)
            break;
        length <<= 1;
    }
}
}

AudioAnalyzer::AudioAnalyzer(QObject *parent)
    : QObject(parent),
    m_fftSize(2048),
    m_barCount(48),
    m_waveformPointCount(256)
{
    m_emitTimer.start();
}

AudioAnalyzer::~AudioAnalyzer() = default;

void AudioAnalyzer::setFftSize(int fftSize)
{
    if (fftSize < 256 || fftSize > kMaxFftSize || !isPowerOfTwo(fftSize))
        return;

    if (m_fftSize == fftSize)
        return;

    m_fftSize = fftSize;
    m_sampleBuffer.clear();
    m_prevBars.clear();
}

void AudioAnalyzer::setBarCount(int barCount)
{
    if (barCount <= 0 || barCount > kMaxBarCount)
        return;

    m_barCount = barCount;
    m_prevBars.clear();
}

void AudioAnalyzer::setWaveformPointCount(int points)
{
    if (points <= 8 || points > kMaxWaveformPointCount)
        return;

    m_waveformPointCount = points;
}

void AudioAnalyzer::reset()
{
    m_sampleBuffer.clear();
    m_prevBars.clear();
    m_emitTimer.restart();

    QVector<float> emptyBars;
    QVector<float> emptyWave;
    emit spectrumReady(emptyBars, emptyWave);
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
            mono[i] = energyPreservingDownmix(channels, [&](int ch) {
                const quint8 v = p[i * channels + ch];
                return (float(v) - 128.0f) / 128.0f;
            });
        }
        break;
    }
    case QAudioFormat::Int16: {
        const qint16 *p = buffer.constData<qint16>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            mono[i] = energyPreservingDownmix(channels, [&](int ch) {
                return float(p[i * channels + ch]) / 32768.0f;
            });
        }
        break;
    }
    case QAudioFormat::Int32: {
        const qint32 *p = buffer.constData<qint32>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            mono[i] = energyPreservingDownmix(channels, [&](int ch) {
                return float(double(p[i * channels + ch]) / 2147483648.0);
            });
        }
        break;
    }
    case QAudioFormat::Float: {
        const float *p = buffer.constData<float>();
        if (!p)
            return {};
        for (int i = 0; i < frames; ++i) {
            mono[i] = energyPreservingDownmix(channels, [&](int ch) {
                return p[i * channels + ch];
            });
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

        float signedPeak = 0.0f;
        for (int j = start; j < end; ++j) {
            if (std::fabs(samples[j]) > std::fabs(signedPeak))
                signedPeak = samples[j];
        }
        out[i] = signedPeak;
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
    if (fftInput.size() != m_fftSize || !isPowerOfTwo(m_fftSize) || sampleRate <= 0)
        return bars;

    QVector<std::complex<float>> fftData(m_fftSize);

    for (int i = 0; i < m_fftSize; ++i) {
        const float hann = 0.5f - 0.5f * std::cos((2.0f * kPi * i) / float(m_fftSize - 1));
        fftData[i] = std::complex<float>(fftInput[i] * hann, 0.0f);
    }

    radix2Fft(fftData);

    QVector<float> mags(m_fftSize / 2 + 1, 0.0f);
    for (int i = 0; i < mags.size(); ++i) {
        const float re = fftData[i].real();
        const float im = fftData[i].imag();
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
    if (!buffer.isValid()) {
        reset();
        return;
    }

    const QVector<float> mono = extractMonoSamples(buffer);
    if (mono.isEmpty())
        return;

    appendSamples(mono);

    // Multimedia backends may deliver very small buffers. Keep all samples
    // for analysis, but cap FFT/repaint work to roughly 60 frames per second.
    if (m_emitTimer.isValid() && m_emitTimer.elapsed() < 16)
        return;
    m_emitTimer.restart();

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
