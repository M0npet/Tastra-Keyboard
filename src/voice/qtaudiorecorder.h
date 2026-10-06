// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/voicecontroller.h"

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>

#include <memory>

class QAudioSource;
class QIODevice;

namespace Tastra
{

// Captures the default microphone into memory only (never to disk) and
// returns mono 16 kHz float samples. Recording stops by itself after 30 s.
class QtAudioRecorder final : public QObject, public AudioRecorder
{
    Q_OBJECT

public:
    explicit QtAudioRecorder(QObject *parent = nullptr);
    ~QtAudioRecorder() override;

    bool start(QString *error) override;
    std::vector<float> stop() override;

    // Exposed for tests: converts interleaved PCM in `format` to mono 16 kHz.
    static std::vector<float> toMono16k(const QByteArray &pcm, const QAudioFormat &format);

private:
    std::unique_ptr<QAudioSource> m_source;
    QIODevice *m_device = nullptr;
    QByteArray m_buffer;
    QAudioFormat m_format;
};

}
