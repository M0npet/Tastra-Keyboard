// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

#include <functional>
#include <memory>
#include <vector>

namespace V3Keyboard
{

// Mono float samples at 16 kHz.
class AudioRecorder
{
public:
    virtual ~AudioRecorder() = default;
    virtual bool start(QString *error) = 0;
    virtual std::vector<float> stop() = 0;
};

class SpeechRecognizer
{
public:
    virtual ~SpeechRecognizer() = default;
    // Empty when ready; otherwise a user-facing reason (e.g. model missing).
    virtual QString unavailableReason() const = 0;
    // Asynchronous; `done(text, error)` is invoked on the caller's thread.
    virtual void recognize(std::vector<float> samples, const QString &language,
                           std::function<void(QString, QString)> done) = 0;
};

// Tap to record, tap again to recognize. Offline; no audio is stored.
class VoiceController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString message READ message NOTIFY stateChanged)

public:
    VoiceController(AudioRecorder *recorder, SpeechRecognizer *recognizer, QObject *parent = nullptr);
    ~VoiceController() override;

    bool available() const;
    QString state() const;
    QString message() const;
    void setLanguage(const QString &code);

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void stateChanged();
    void textRecognized(const QString &text);

private:
    void setState(const QString &state, const QString &message = {});

    AudioRecorder *m_recorder;
    SpeechRecognizer *m_recognizer;
    QString m_language = QStringLiteral("en");
    QString m_state = QStringLiteral("idle");
    QString m_message;
    // Bumped on cancel so late results from an abandoned run are ignored.
    std::shared_ptr<int> m_generation = std::make_shared<int>(0);
};

}
