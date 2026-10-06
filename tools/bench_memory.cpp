// SPDX-License-Identifier: GPL-3.0-or-later
// Resident memory per language: each language is measured in its own child
// process (RSS never shrinks back reliably inside one process, so a shared
// process would blur the numbers). Real system Hunspell dictionaries.
//
//   bench_memory            all languages, one child each
//   bench_memory ru         only Russian, in this process
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>

#include <cstdio>

#include "core/locallexicon.h"

namespace
{
long rssKiB()
{
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    for (const QByteArray &line : status.readAll().split('\n')) {
        if (line.startsWith("VmRSS:")) return line.mid(6).trimmed().split(' ').value(0).toLong();
    }
    return -1;
}

QString otherLanguage(const QString &code)
{
    if (code == QLatin1String("ru") || code == QLatin1String("uk")) return QStringLiteral("en");
    return QStringLiteral("ru");
}

int measure(const QString &code)
{
    const long base = rssKiB();
    QElapsedTimer timer;
    timer.start();
    Tastra::LocalLexicon lexicon;
    lexicon.setLanguage(code);
    lexicon.waitForDictionary(-1);
    // A query adopts the loaded data the way typing does.
    lexicon.suggestions(QStringLiteral("te"), QString(), 3);
    const qint64 loadMs = timer.elapsed();
    const long loaded = rssKiB();

    const QString foreign = otherLanguage(code);
    Tastra::LocalLexicon::knownInLanguage(foreign, QStringLiteral("x"));
    for (int i = 0; i < 400 && !Tastra::LocalLexicon::knownInLanguage(foreign, QStringLiteral("the"))
                     && !Tastra::LocalLexicon::knownInLanguage(foreign, QStringLiteral("и")); ++i) {
        QThread::msleep(10);
    }
    QThread::msleep(50);
    const long withForeign = rssKiB();

    std::printf("%s: base %.1f MB, dictionary %.1f MB (+%.1f, load %lld ms), with %s word list %.1f MB (+%.1f)\n",
                qPrintable(code), base / 1024.0, loaded / 1024.0, (loaded - base) / 1024.0,
                static_cast<long long>(loadMs), qPrintable(foreign), withForeign / 1024.0,
                (withForeign - loaded) / 1024.0);
    return 0;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    app.setOrganizationName(QStringLiteral("bench"));
    app.setApplicationName(QStringLiteral("bench_memory"));
    if (argc > 1) return measure(QString::fromLocal8Bit(argv[1]));

    int failures = 0;
    for (const char *code : {"en", "de", "ru", "uk"}) {
        QProcess child;
        child.setProcessChannelMode(QProcess::ForwardedChannels);
        child.start(QCoreApplication::applicationFilePath(), {QString::fromLatin1(code)});
        if (!child.waitForFinished(120000) || child.exitCode() != 0) ++failures;
    }
    return failures;
}
