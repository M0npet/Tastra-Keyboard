// SPDX-License-Identifier: GPL-3.0-or-later

#include "whisperrecognizer.h"

#include <QCoreApplication>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <whisper.h>

#include <algorithm>

Q_LOGGING_CATEGORY(lcWhisper, "tastra.voice.whisper", QtWarningMsg)

namespace Tastra
{
namespace
{
void quietWhisperLog(enum ggml_log_level, const char *, void *) {}
}

WhisperRecognizer::WhisperRecognizer(const QString &modelPath, QObject *parent)
    : QObject(parent)
    , m_modelPath(modelPath)
{
    whisper_log_set(quietWhisperLog, nullptr);
    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(60000);
    connect(&m_idleTimer, &QTimer::timeout, this, &WhisperRecognizer::releaseModel);
}

WhisperRecognizer::~WhisperRecognizer()
{
    if (m_worker.joinable()) m_worker.join();
    std::lock_guard lock(m_mutex);
    if (m_context) whisper_free(m_context);
}

QString WhisperRecognizer::defaultModelPath()
{
    const QString overridePath = qEnvironmentVariable("TASTRA_VOICE_MODEL");
    if (!overridePath.isEmpty()) return overridePath;
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/tastra/voice/ggml-base-q5_1.bin");
}

QString WhisperRecognizer::unavailableReason() const
{
    if (!QFileInfo::exists(m_modelPath)) {
        return QCoreApplication::translate("Tastra", "Voice model not installed: run tastra-voice-setup");
    }
    return {};
}

void WhisperRecognizer::releaseModel()
{
    std::unique_lock lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock() || !m_context) return;      // busy: try again later
    whisper_free(m_context);
    m_context = nullptr;
    qCDebug(lcWhisper) << "model released";
}

void WhisperRecognizer::recognize(std::vector<float> samples, const QString &language,
                                  std::function<void(QString, QString)> done)
{
    m_idleTimer.stop();
    if (m_worker.joinable()) m_worker.join();          // one recognition at a time
    const std::string lang = language.toStdString();
    const std::string path = m_modelPath.toStdString();
    m_worker = std::thread([this, samples = std::move(samples), lang, path, done = std::move(done)]() mutable {
        QString text;
        QString error;
        {
            std::lock_guard lock(m_mutex);
            if (!m_context) {
                whisper_context_params cparams = whisper_context_default_params();
                m_context = whisper_init_from_file_with_params(path.c_str(), cparams);
            }
            if (!m_context) {
                error = QCoreApplication::translate("Tastra", "Voice model could not be loaded");
            } else {
                whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
                params.language = lang.c_str();
                params.translate = false;
                params.no_timestamps = true;
                params.print_progress = false;
                params.print_realtime = false;
                params.print_special = false;
                params.print_timestamps = false;
                params.n_threads = int(std::clamp(std::thread::hardware_concurrency() / 2, 1u, 4u));
                if (whisper_full(m_context, params, samples.data(), int(samples.size())) != 0) {
                    error = QCoreApplication::translate("Tastra", "Speech recognition failed");
                } else {
                    const int segments = whisper_full_n_segments(m_context);
                    for (int i = 0; i < segments; ++i) text += QString::fromUtf8(whisper_full_get_segment_text(m_context, i));
                }
            }
        }
        std::fill(samples.begin(), samples.end(), 0.0f);  // do not keep audio around
        QMetaObject::invokeMethod(this, [this, done = std::move(done), text, error] {
            m_idleTimer.start();
            done(text, error);
        }, Qt::QueuedConnection);
    });
}

}
