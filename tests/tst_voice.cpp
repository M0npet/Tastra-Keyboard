// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/voicecontroller.h"

using namespace V3Keyboard;

namespace
{

class FakeRecorder final : public AudioRecorder
{
public:
    bool start(QString *error) override
    {
        if (!startError.isEmpty()) { *error = startError; return false; }
        recording = true;
        return true;
    }
    std::vector<float> stop() override { recording = false; return audio; }
    bool recording = false;
    QString startError;
    std::vector<float> audio = std::vector<float>(16000, 0.1f);   // 1 s
};

class FakeRecognizer final : public SpeechRecognizer
{
public:
    QString unavailableReason() const override { return reason; }
    void recognize(std::vector<float> samples, const QString &lang,
                   std::function<void(QString, QString)> done) override
    {
        lastSamples = samples.size();
        language = lang;
        pending = std::move(done);
    }
    void finish(const QString &text, const QString &error = {}) { auto d = std::move(pending); d(text, error); }
    QString reason;
    QString language;
    size_t lastSamples = 0;
    std::function<void(QString, QString)> pending;
};

}

class VoiceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tapToRecordTapToRecognize()
    {
        FakeRecorder recorder;
        FakeRecognizer recognizer;
        VoiceController voice(&recorder, &recognizer);
        voice.setLanguage(QStringLiteral("uk"));
        QSignalSpy text(&voice, &VoiceController::textRecognized);

        QVERIFY(voice.available());
        QCOMPARE(voice.state(), QStringLiteral("idle"));
        voice.toggle();
        QCOMPARE(voice.state(), QStringLiteral("recording"));
        QVERIFY(recorder.recording);
        voice.toggle();
        QCOMPARE(voice.state(), QStringLiteral("recognizing"));
        QCOMPARE(recognizer.language, QStringLiteral("uk"));
        QCOMPARE(recognizer.lastSamples, size_t(16000));
        recognizer.finish(QStringLiteral(" привіт світ "));
        QCOMPARE(voice.state(), QStringLiteral("idle"));
        QCOMPARE(text.size(), 1);
        QCOMPARE(text.at(0).at(0).toString(), QStringLiteral("привіт світ"));
    }

    void missingModelIsReportedNotRecorded()
    {
        FakeRecorder recorder;
        FakeRecognizer recognizer;
        recognizer.reason = QStringLiteral("model missing");
        VoiceController voice(&recorder, &recognizer);
        QVERIFY(!voice.available());
        voice.toggle();
        QVERIFY(!recorder.recording);
        QCOMPARE(voice.state(), QStringLiteral("unavailable"));
        QCOMPARE(voice.message(), QStringLiteral("model missing"));
    }

    void microphoneErrorsAndTooShortClips()
    {
        FakeRecorder recorder;
        FakeRecognizer recognizer;
        VoiceController voice(&recorder, &recognizer);
        recorder.startError = QStringLiteral("no microphone");
        voice.toggle();
        QCOMPARE(voice.state(), QStringLiteral("idle"));
        QCOMPARE(voice.message(), QStringLiteral("no microphone"));

        recorder.startError.clear();
        recorder.audio = std::vector<float>(1600, 0.1f);          // 0.1 s: accidental tap
        voice.toggle();
        voice.toggle();
        QCOMPARE(voice.state(), QStringLiteral("idle"));
        QVERIFY(!recognizer.pending);                               // nothing sent
    }

    void emptyResultsAndCancelledRecognitionInsertNothing()
    {
        FakeRecorder recorder;
        FakeRecognizer recognizer;
        VoiceController voice(&recorder, &recognizer);
        QSignalSpy text(&voice, &VoiceController::textRecognized);

        voice.toggle(); voice.toggle();
        recognizer.finish(QStringLiteral("  "));
        QCOMPARE(text.size(), 0);

        voice.toggle(); voice.toggle();
        voice.cancel();                                             // e.g. keyboard hidden
        QCOMPARE(voice.state(), QStringLiteral("idle"));
        recognizer.finish(QStringLiteral("late result"));
        QCOMPARE(text.size(), 0);

        voice.toggle(); voice.toggle();
        recognizer.finish({}, QStringLiteral("decoder failed"));
        QCOMPARE(voice.message(), QStringLiteral("decoder failed"));
        QCOMPARE(text.size(), 0);
    }

    void noBackendsMeansUnavailable()
    {
        VoiceController voice(nullptr, nullptr);
        QVERIFY(!voice.available());
        voice.toggle();
        QCOMPARE(voice.state(), QStringLiteral("unavailable"));
    }
};

QTEST_GUILESS_MAIN(VoiceTest)
#include "tst_voice.moc"
