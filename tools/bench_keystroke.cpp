// SPDX-License-Identifier: GPL-3.0-or-later
// Per-keystroke cost through the full UI bridge (engine, suggestions,
// autocorrect target, emoji/shortcut lookups) on the real system dictionaries.
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QThread>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "app/keyboarduibridge.h"
#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/keyboardmodel.h"
#include "core/locallexicon.h"

namespace
{
class NullBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &) override {}
    void backspace() override {}
    void deleteForward() override {}
    void moveLeft() override {}
    void moveRight() override {}
    void moveHome() override {}
    void moveEnd() override {}
    void enter() override {}
    bool setPreedit(const QString &) override { return true; }
};
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    app.setOrganizationName(QStringLiteral("bench"));
    app.setApplicationName(QStringLiteral("bench_keystroke"));
    const std::vector<std::pair<const char *, const char *>> texts = {
        {"en", "the quick brown fox jumps over teh lazy dog and wnats to go home"},
        {"de", "ich habe heute keine zeit weil wir nciht nach hause gehen"},
        {"ru", "привет как дела сегодня првиет хорошо спаисбо большое"},
        {"uk", "привіт як справи сьогодні дякую тпер добре будь ласка"}};
    const QByteArray only = qgetenv("BENCH_LANG");
    for (const auto &[lang, text] : texts) {
        if (!only.isEmpty() && only != lang) continue;
        NullBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setLanguage(QString::fromLatin1(lang));
        if (qEnvironmentVariableIsSet("BENCH_NO_LEARNING")) bridge.setLearningEnabled(false);
        bridge.setSurroundingText(QString(), 0, 0);
        // Let the bridge's own background dictionary load finish first.
        QThread::msleep(qEnvironmentVariableIntValue("BENCH_SETTLE_MS") > 0 ? qEnvironmentVariableIntValue("BENCH_SETTLE_MS") : 3000);
        QCoreApplication::processEvents();
        std::vector<double> ms;
        std::vector<std::pair<double, QString>> slowest;
        double maxKey = 0, maxSug = 0, maxSpace = 0;
        for (int round = 0; round < 3; ++round) {
            double roundMax = 0;
            for (const QChar ch : QString::fromUtf8(text)) {
                QElapsedTimer t;
                t.start();
                if (ch == QLatin1Char(' ')) bridge.space(); else bridge.tapLetter(QString(ch));
                const double key = t.nsecsElapsed() / 1e6;
                (void)bridge.suggestions();
                (void)bridge.autocorrectSuggestion();
                const double total = t.nsecsElapsed() / 1e6;
                ms.push_back(total);
                slowest.push_back({total, QStringLiteral("r%1 '%2' word=%3").arg(round + 1).arg(ch).arg(bridge.currentWord())});
                if (ch == QLatin1Char(' ')) maxSpace = std::max(maxSpace, key);
                else maxKey = std::max(maxKey, key);
                maxSug = std::max(maxSug, total - key);
                roundMax = std::max(roundMax, total);
            }
            std::printf("   round %d max=%.2f ms\n", round + 1, roundMax);
        }
        std::printf("   breakdown: max letter=%.2f ms, max space=%.2f ms, max suggestions()=%.2f ms\n", maxKey, maxSpace, maxSug);
        std::sort(slowest.begin(), slowest.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
        for (int i = 0; i < 5 && i < int(slowest.size()); ++i)
            std::printf("   slow %.2f ms  %s\n", slowest[i].first, slowest[i].second.toUtf8().constData());
        std::sort(ms.begin(), ms.end());
        std::printf("%s: keystrokes=%zu  p50=%.2f ms  p95=%.2f ms  max=%.2f ms\n", lang, ms.size(),
                    ms[ms.size() / 2], ms[ms.size() * 95 / 100], ms.back());
    }
}
