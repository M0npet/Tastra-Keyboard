// SPDX-License-Identifier: GPL-3.0-or-later
//
// Randomised stress test: whatever the sequence of letters, spaces,
// punctuation and backspaces — and whatever (possibly stale) surrounding-text
// echoes the client sends — the text that ends up in the field must equal the
// text the user typed. Smart features that intentionally change text
// (autocorrect, capitalisation, double-space period) are switched off.

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/locallexicon.h"
#include "core/typingengine.h"

namespace
{

// A field model that behaves like a text-input client driven through KWin:
// commits replace the preedit; Backspace deletes before the committed cursor.
class FieldBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override { preedit.clear(); field += text; }
    void backspace() override
    {
        if (!preedit.isEmpty()) return;          // like a client with an active composition
        if (!field.isEmpty()) field.chop(1);
    }
    void deleteForward() override {}
    void moveLeft() override {}
    void moveRight() override {}
    void moveHome() override {}
    void moveEnd() override {}
    void enter() override { preedit.clear(); field += QLatin1Char('\n'); }
    bool deleteBeforeCursor(const QString &text) override
    {
        if (!textChannel || !field.endsWith(text)) return false;
        field.chop(text.size());
        return true;
    }
    bool setPreedit(const QString &text) override
    {
        if (!preeditSupport) return false;
        preedit = text;
        return true;
    }

    QString visible() const { return field + preedit; }
    QString field;
    QString preedit;
    bool textChannel = false;
    bool preeditSupport = false;
};

}

class TypingStressTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("V3KeyboardTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_typingstress"));
        QSettings().clear();
        V3Keyboard::LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        V3Keyboard::LocalLexicon::setFrequencySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        V3Keyboard::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
    }

    void typedTextSurvivesRandomSequences_data()
    {
        QTest::addColumn<bool>("composition");
        QTest::addColumn<int>("echoMode");     // 0 none, 1 fresh echoes, 2 one-step stale (Firefox)
        for (const bool composition : {false, true}) {
            for (int echo = 0; echo <= 2; ++echo) {
                QTest::addRow("composition=%d echo=%d", int(composition), echo) << composition << echo;
            }
        }
    }

    void typedTextSurvivesRandomSequences()
    {
        QFETCH(bool, composition);
        QFETCH(int, echoMode);
        for (quint32 seed = 1; seed <= 40; ++seed) {
            QRandomGenerator rng(seed * 7919u + quint32(echoMode) * 31u + (composition ? 1u : 0u));
            FieldBackend backend;
            backend.textChannel = composition;
            backend.preeditSupport = composition;
            V3Keyboard::KeyboardController controller(backend);
            V3Keyboard::TypingEngine engine(controller);
            engine.setAutocorrectEnabled(false);
            engine.setAutoCapitalizationEnabled(false);
            engine.setDoubleSpacePeriodEnabled(false);
            qint64 now = 0;
            engine.setClockForTesting([&now] { return now; });
            auto echo = [&](const QString &before) {
                const int bytes = before.toUtf8().size();
                engine.syncSurroundingText(before, bytes, bytes);
            };
            if (composition || echoMode > 0) echo(QString());   // text-input client, empty field

            QString expected;
            QString previousVisible;
            // Reference for the one intentional rewrite in composition mode
            // (LatinIME/Gboard): "word ," -> "word, ", next Space absorbed.
            int wordLength = 0;        // letters of the word being composed
            bool heldAfterWord = false;
            bool swapped = false;
            bool heldSpace = false;    // the trailing space is still in the preedit
            const QString letters = QStringLiteral("abcdeéжщї");
            for (int step = 0; step < 250; ++step) {
                now += 5 + rng.bounded(300);
                const int op = rng.bounded(100);
                if (op < 58) {
                    const QString ch(letters.at(rng.bounded(letters.size())));
                    engine.typeLetter(ch);
                    expected += ch;
                    ++wordLength;
                    heldAfterWord = swapped = heldSpace = false;
                } else if (op < 62) {
                    // New paragraph: exercises the empty-paragraph first letter.
                    engine.enter();
                    if (composition && heldSpace) expected.chop(1);   // a held space is dropped
                    expected += QLatin1Char('\n');
                    wordLength = 0;
                    heldAfterWord = swapped = heldSpace = false;
                } else if (op < 78) {
                    engine.space();
                    if (composition && swapped) {
                        swapped = false;                       // already there
                    } else {
                        expected += QLatin1Char(' ');
                        heldAfterWord = composition && wordLength > 0;
                        heldSpace = composition && (wordLength > 0 || heldSpace);
                    }
                    wordLength = 0;
                } else if (op < 86) {
                    const QString p = QStringList{QStringLiteral(","), QStringLiteral("."), QStringLiteral("!")}
                        .at(rng.bounded(3));
                    engine.typeText(p);
                    if (heldAfterWord) {
                        expected.chop(1);
                        expected += p + QLatin1Char(' ');
                        swapped = true;
                        heldSpace = true;
                    } else {
                        expected += p;
                        swapped = false;
                        heldSpace = false;
                    }
                    heldAfterWord = false;
                    wordLength = 0;
                } else {
                    engine.backspace();
                    if (!expected.isEmpty()) expected.chop(1);
                    if (wordLength > 0) --wordLength;
                    heldAfterWord = swapped = heldSpace = false;
                }
                // Echoes the client might send after this step.
                if (echoMode == 1) echo(backend.field);
                if (echoMode == 2 && !previousVisible.isNull()) echo(previousVisible.left(backend.field.size()));
                previousVisible = backend.field;

                QCOMPARE(backend.visible(), expected);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TypingStressTest)
#include "tst_typingstress.moc"
