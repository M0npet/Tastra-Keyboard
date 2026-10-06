// SPDX-License-Identifier: GPL-3.0-or-later

#include "voicecontroller.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcVoice, "tastra.voice", QtWarningMsg)

namespace Tastra
{
namespace
{
constexpr size_t MinimumSamples = 16000 * 3 / 10;   // 0.3 s: shorter is an accidental tap
}

VoiceController::VoiceController(AudioRecorder *recorder, SpeechRecognizer *recognizer, QObject *parent)
    : QObject(parent)
    , m_recorder(recorder)
    , m_recognizer(recognizer)
{
    if (!available()) m_state = QStringLiteral("unavailable");
}

VoiceController::~VoiceController() = default;

bool VoiceController::available() const
{
    return m_recorder && m_recognizer && m_recognizer->unavailableReason().isEmpty();
}

QString VoiceController::state() const { return m_state; }
QString VoiceController::message() const { return m_message; }
void VoiceController::setLanguage(const QString &code) { m_language = code; }

void VoiceController::setState(const QString &state, const QString &message)
{
    if (m_state == state && m_message == message) return;
    m_state = state;
    m_message = message;
    qCDebug(lcVoice) << "state" << state;
    Q_EMIT stateChanged();
}

void VoiceController::toggle()
{
    if (!available()) {
        setState(QStringLiteral("unavailable"),
                 m_recognizer ? m_recognizer->unavailableReason() : QStringLiteral("Voice input is not built in"));
        return;
    }
    if (m_state == QStringLiteral("recognizing")) return;
    if (m_state != QStringLiteral("recording")) {
        QString error;
        if (!m_recorder->start(&error)) {
            setState(QStringLiteral("idle"), error);
            return;
        }
        setState(QStringLiteral("recording"));
        return;
    }

    std::vector<float> samples = m_recorder->stop();
    qCDebug(lcVoice) << "captured samples" << samples.size();
    if (samples.size() < MinimumSamples) {
        setState(QStringLiteral("idle"));
        return;
    }
    setState(QStringLiteral("recognizing"));
    const int generation = ++*m_generation;
    std::weak_ptr<int> alive = m_generation;
    m_recognizer->recognize(std::move(samples), m_language,
                            [this, alive, generation](QString text, QString error) {
        const auto current = alive.lock();
        if (!current || *current != generation) return;          // cancelled or destroyed
        const QString trimmed = text.simplified();
        setState(QStringLiteral("idle"), error);
        if (error.isEmpty() && !trimmed.isEmpty()) Q_EMIT textRecognized(trimmed);
    });
}

void VoiceController::cancel()
{
    ++*m_generation;
    if (m_state == QStringLiteral("recording")) m_recorder->stop();
    if (m_state == QStringLiteral("recording") || m_state == QStringLiteral("recognizing")) {
        setState(QStringLiteral("idle"));
    }
}

}
