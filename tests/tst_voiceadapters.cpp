// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest/QTest>

#include "voice/qtaudiorecorder.h"
#include "voice/whisperrecognizer.h"

using namespace Tastra;

class VoiceAdaptersTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stereo48kInt16BecomesMono16kFloat()
    {
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        QByteArray pcm;
        for (int i = 0; i < 4800; ++i) {                 // 0.1 s
            const qint16 left = 16384, right = -16384 / 2;
            pcm.append(reinterpret_cast<const char *>(&left), 2);
            pcm.append(reinterpret_cast<const char *>(&right), 2);
        }
        const std::vector<float> out = QtAudioRecorder::toMono16k(pcm, format);
        QCOMPARE(out.size(), size_t(1600));
        QVERIFY(qAbs(out[800] - 0.125f) < 0.001f);       // (0.5 + -0.25) / 2
    }

    void missingModelIsReported()
    {
        WhisperRecognizer recognizer(QStringLiteral("/nonexistent/model.bin"));
        QVERIFY(recognizer.unavailableReason().contains(QStringLiteral("tastra-voice-setup")));
    }

    void realWhisperRunCompletesOffThread()
    {
        const QString model = qEnvironmentVariable("TASTRA_TEST_WHISPER_MODEL");
        if (model.isEmpty()) QSKIP("set TASTRA_TEST_WHISPER_MODEL to run whisper end-to-end");
        WhisperRecognizer recognizer(model);
        QVERIFY(recognizer.unavailableReason().isEmpty());
        bool finished = false;
        QString error = QStringLiteral("unset");
        recognizer.recognize(std::vector<float>(16000, 0.0f), QStringLiteral("en"),
                             [&](QString, QString e) { finished = true; error = e; });
        QTRY_VERIFY_WITH_TIMEOUT(finished, 60000);
        QVERIFY2(error.isEmpty() || error == QStringLiteral("Speech recognition failed"), qPrintable(error));
    }
};

QTEST_GUILESS_MAIN(VoiceAdaptersTest)
#include "tst_voiceadapters.moc"
