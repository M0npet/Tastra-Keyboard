// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest/QTest>
#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include <csignal>
#include <cstdio>

#include "app/terminationsaver.h"
#include "core/locallexicon.h"

using Tastra::LocalLexicon;

namespace
{
void useNoDictionaries()
{
    LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    LocalLexicon::setBigramSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
}

void useTestSettings()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
    QCoreApplication::setApplicationName(QStringLiteral("tst_terminationsaver"));
}

// The keyboard as KWin runs it: it has learned a few words (fewer than the
// 16 that are saved at once) when KWin ends it.
int runChild(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    useTestSettings();
    useNoDictionaries();
    LocalLexicon lexicon;
    lexicon.setLanguage(QStringLiteral("en"));
    lexicon.waitForDictionary(5000);
    for (int i = 0; i < 3; ++i) lexicon.learnWordWithContext(QStringLiteral("quokka"), QStringLiteral("happy"));
    const bool installed = qEnvironmentVariableIsSet("TASTRA_TERMINATION_UNHANDLED")
        || Tastra::saveBeforeTermination(&app, [&lexicon] {
               lexicon.flushLearning();
               LocalLexicon::waitForLearningWrites();
           });
    std::printf(installed ? "ready\n" : "failed\n");
    std::fflush(stdout);
    return app.exec();
}
}

class TerminationSaverTest : public QObject
{
    Q_OBJECT

private:
    // Starts the child, sends it `signal` and returns how many times it had
    // learned "quokka" according to the saved settings.
    static int learnedAfter(int signal, bool handled)
    {
        QSettings().clear();
        QProcess child;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("TASTRA_TERMINATION_CHILD"), QStringLiteral("1"));
        if (!handled) environment.insert(QStringLiteral("TASTRA_TERMINATION_UNHANDLED"), QStringLiteral("1"));
        child.setProcessEnvironment(environment);
        child.start(QCoreApplication::applicationFilePath(), {});
        if (!child.waitForReadyRead(10000)) return -1;
        if (child.readLine().trimmed() != "ready") return -2;
        ::kill(pid_t(child.processId()), signal);
        if (!child.waitForFinished(10000)) return -3;
        // Ended as the signal would have ended it (KWin sees no difference).
        if (child.exitStatus() != QProcess::CrashExit) return -4;
        QSettings settings;
        settings.sync();
        return settings.value(QStringLiteral("learning/en/words")).toMap().value(QStringLiteral("quokka")).toInt();
    }

private Q_SLOTS:
    void initTestCase() { useTestSettings(); }

    void wordsLearnedSinceTheLastSaveSurviveSigterm()
    {
        // KWin's InputMethod::stopInputMethod() sends SIGTERM when another
        // virtual keyboard is chosen and when the session ends.
        QCOMPARE(learnedAfter(SIGTERM, true), 3);
        QCOMPARE(learnedAfter(SIGINT, true), 3);
        QSettings settings;
        QCOMPARE(settings.value(QStringLiteral("learning/en/bigrams")).toMap().value(QStringLiteral("happy") + QChar(0x001f) + QStringLiteral("quokka")).toInt(), 3);
    }

    void withoutTheHandlerTheyWereLost()
    {
        // What happened before: the default action ends the process at once.
        QCOMPARE(learnedAfter(SIGTERM, false), 0);
    }
};

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("TASTRA_TERMINATION_CHILD")) return runChild(argc, argv);
    QCoreApplication app(argc, argv);
    TerminationSaverTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_terminationsaver.moc"
