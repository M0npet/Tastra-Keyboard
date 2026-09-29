// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/voicecontroller.h"

#include <QObject>
#include <QTimer>

#include <mutex>
#include <thread>

struct whisper_context;

namespace V3Keyboard
{

// Offline speech recognition with whisper.cpp. The model is loaded on first
// use and released after a minute without dictation to keep memory low.
class WhisperRecognizer final : public QObject, public SpeechRecognizer
{
    Q_OBJECT

public:
    explicit WhisperRecognizer(const QString &modelPath, QObject *parent = nullptr);
    ~WhisperRecognizer() override;

    static QString defaultModelPath();

    QString unavailableReason() const override;
    void recognize(std::vector<float> samples, const QString &language,
                   std::function<void(QString, QString)> done) override;

private:
    void releaseModel();

    QString m_modelPath;
    std::mutex m_mutex;              // guards m_context
    whisper_context *m_context = nullptr;
    std::thread m_worker;
    QTimer m_idleTimer;
};

}
