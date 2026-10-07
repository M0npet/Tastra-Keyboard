// SPDX-License-Identifier: GPL-3.0-or-later

#include "qtaudiorecorder.h"

#include <QCoreApplication>

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Tastra
{
namespace
{
constexpr int TargetRate = 16000;
constexpr int MaxSeconds = 30;
}

QtAudioRecorder::QtAudioRecorder(QObject *parent) : QObject(parent) {}
QtAudioRecorder::~QtAudioRecorder() { if (m_source) m_source->stop(); }

bool QtAudioRecorder::start(QString *error)
{
    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        *error = QCoreApplication::translate("Tastra", "No microphone found");
        return false;
    }
    QAudioFormat wanted;
    wanted.setSampleRate(TargetRate);
    wanted.setChannelCount(1);
    wanted.setSampleFormat(QAudioFormat::Float);
    m_format = device.isFormatSupported(wanted) ? wanted : device.preferredFormat();
    m_buffer.clear();
    m_source = std::make_unique<QAudioSource>(device, m_format);
    m_device = m_source->start();
    if (!m_device) {
        m_source.reset();
        *error = QCoreApplication::translate("Tastra", "Microphone could not be opened");
        return false;
    }
    const qint64 limit = qint64(m_format.bytesForDuration(qint64(MaxSeconds) * 1000000));
    connect(m_device, &QIODevice::readyRead, this, [this, limit] {
        m_buffer += m_device->readAll();
        if (m_buffer.size() >= limit) {
            m_buffer.truncate(int(limit));
            m_source->suspend();                  // hard cap; stop() still returns the audio
        }
    });
    return true;
}

std::vector<float> QtAudioRecorder::stop()
{
    if (!m_source) return {};
    if (m_device) m_buffer += m_device->readAll();
    m_source->stop();
    m_source.reset();
    m_device = nullptr;
    std::vector<float> samples = toMono16k(m_buffer, m_format);
    m_buffer.fill('\0');                           // do not keep audio around
    m_buffer.clear();
    return samples;
}

std::vector<float> QtAudioRecorder::toMono16k(const QByteArray &pcm, const QAudioFormat &format)
{
    const int channels = qMax(1, format.channelCount());
    const int bytes = format.bytesPerSample();
    const int rate = format.sampleRate();
    if (bytes <= 0 || rate <= 0) return {};
    const qsizetype frames = pcm.size() / (bytes * channels);
    std::vector<float> mono(size_t(qMax<qsizetype>(0, frames)));
    const char *data = pcm.constData();
    for (qsizetype f = 0; f < frames; ++f) {
        float sum = 0.0f;
        for (int c = 0; c < channels; ++c) {
            const char *p = data + (f * channels + c) * bytes;
            float v = 0.0f;
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8: v = (float(quint8(*p)) - 128.0f) / 128.0f; break;
            case QAudioFormat::Int16: { qint16 s; std::memcpy(&s, p, 2); v = float(s) / 32768.0f; break; }
            case QAudioFormat::Int32: { qint32 s; std::memcpy(&s, p, 4); v = float(double(s) / 2147483648.0); break; }
            case QAudioFormat::Float: std::memcpy(&v, p, 4); break;
            default: break;
            }
            sum += v;
        }
        mono[size_t(f)] = sum / float(channels);
    }
    if (rate == TargetRate) return mono;
    // Linear resampling is adequate for speech recognition input.
    const double ratio = double(rate) / TargetRate;
    const size_t outCount = size_t(double(mono.size()) / ratio);
    std::vector<float> out(outCount);
    for (size_t i = 0; i < outCount; ++i) {
        const double pos = double(i) * ratio;
        const size_t i0 = size_t(pos);
        const size_t i1 = std::min(i0 + 1, mono.size() - 1);
        const float t = float(pos - double(i0));
        out[i] = mono[i0] * (1.0f - t) + mono[i1] * t;
    }
    return out;
}

}
