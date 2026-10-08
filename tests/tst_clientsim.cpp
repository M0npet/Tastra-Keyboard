// SPDX-License-Identifier: GPL-3.0-or-later

// The typing engine against a modelled GTK 3 / Firefox text field behind
// KWin (tests/textinputclient.h): what the user finally sees in the field.

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>

#include "core/keyboardcontroller.h"
#include "core/locallexicon.h"
#include "core/typingengine.h"
#include "textinputclient.h"

namespace
{

// One keyboard + one field, with a clock the engine and the client share.
struct Session {
    explicit Session(TextInputClient::Echo echo = TextInputClient::Echo::Current)
        : client(echo), controller(client), engine(controller)
    {
        engine.setClockForTesting([this] { return now; });
        engine.setLanguage(QStringLiteral("en"));
        client.engine = &engine;
        client.advance = [this](qint64 ms) { now += ms; };
        // A field that reports its text, as text-input-v3 clients do.
        client.setText(QString());
    }

    // A user action, then the client's answer, then a human pause.
    template<typename F>
    void act(F &&f)
    {
        f();
        client.deliver();
        now += 180;
    }
    void type(const QString &letters)
    {
        for (const QChar ch : letters) {
            if (ch == QLatin1Char(' ')) act([&] { engine.space(); });
            else act([&] { engine.typeLetter(QString(ch)); });
        }
    }

    qint64 now = 100000;
    TextInputClient client;
    Tastra::KeyboardController controller;
    Tastra::TypingEngine engine;
};

}

class ClientSimTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_clientsim"));
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
        Tastra::LocalLexicon::setBigramSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    }

    void init() { QSettings().clear(); }

    void echoModes_data()
    {
        QTest::addColumn<int>("echo");
        QTest::newRow("gtk") << int(TextInputClient::Echo::Current);
        QTest::newRow("firefox") << int(TextInputClient::Echo::Lagging);
    }

    void plainTypingAndAutocorrect_data() { echoModes_data(); }
    void plainTypingAndAutocorrect()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.engine.setAutoCapitalizationEnabled(false);
        s.type(QStringLiteral("teh cat "));
        QCOMPARE(s.client.visible(), QStringLiteral("the cat "));
        QCOMPARE(s.client.badDeletes, 0);
    }

    // Tapping at the end of a word and typing on: the word is resumed as
    // the word being typed, so a suggestion replaces all of it (LatinIME
    // restartSuggestionsOnWordTouchedByCursor; Gboard underlines it again).
    void aSuggestionAfterTypingOnAnOldWordReplacesItInOrder_data() { echoModes_data(); }
    void aSuggestionAfterTypingOnAnOldWordReplacesItInOrder()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.client.setText(QStringLiteral("I need hel"));
        s.act([&] { s.engine.typeLetter(QStringLiteral("p")); });
        QCOMPARE(s.engine.currentWord(), QStringLiteral("help"));
        s.act([&] { s.engine.chooseSuggestion(QStringLiteral("helpful")); });
        s.act([&] { s.engine.space(); });
        QCOMPARE(s.client.visible(), QStringLiteral("I need helpful "));
        QCOMPARE(s.client.badDeletes, 0);
    }

    void aCorrectionAfterTypingOnAnOldWordReplacesItInOrder_data() { echoModes_data(); }
    void aCorrectionAfterTypingOnAnOldWordReplacesItInOrder()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.client.setText(QStringLiteral("hel"));
        s.act([&] { s.engine.typeLetter(QStringLiteral("o")); });
        s.act([&] { s.engine.chooseSuggestion(QStringLiteral("hello")); });
        QCOMPARE(s.client.visible().trimmed(), QStringLiteral("hello"));
        QCOMPARE(s.client.badDeletes, 0);
    }

    // "hello", Space, Backspace, "s": one word "hellos", as in Gboard; the
    // "s" alone is never autocorrected.
    void backspaceIntoAWordContinuesIt_data() { echoModes_data(); }
    void backspaceIntoAWordContinuesIt()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.engine.setAutoCapitalizationEnabled(false);
        s.type(QStringLiteral("cat "));
        s.act([&] { s.engine.backspace(); });
        QCOMPARE(s.client.visible(), QStringLiteral("cat"));
        QCOMPARE(s.engine.currentWord(), QStringLiteral("cat"));
        s.type(QStringLiteral("s "));
        QCOMPARE(s.client.visible(), QStringLiteral("cats "));
        QCOMPARE(s.client.badDeletes, 0);
    }
    // With composition turned off the letters are committed one by one; a
    // suggestion tap before the client confirmed the word used Backspace
    // keys, which GTK and Firefox apply after the commit sent with them.
    void aSuggestionTapWithoutCompositionReplacesTheWordInOrder_data() { echoModes_data(); }
    void aSuggestionTapWithoutCompositionReplacesTheWordInOrder()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.engine.setCompositionEnabled(false);
        s.engine.setAutoCapitalizationEnabled(false);
        s.type(QStringLiteral("we hepl"));
        s.act([&] { s.engine.chooseSuggestion(QStringLiteral("help")); });
        QCOMPARE(s.client.visible(), QStringLiteral("we help "));
        // The last letter's answer has not reached the keyboard yet.
        s.type(QStringLiteral("yu"));
        s.engine.typeLetter(QStringLiteral("o"));
        s.client.deliver(false);
        s.now += 200;
        s.act([&] { s.engine.chooseSuggestion(QStringLiteral("you")); });
        QCOMPARE(s.client.visible(), QStringLiteral("we help you "));
        QCOMPARE(s.client.badDeletes, 0);
    }

    // tst_typingstress against the modelled clients: Backspace keys and
    // text-input requests reach them on different channels, so any action
    // that mixes both in one go shows up here.
    void randomTypingOnRealClients_data() { echoModes_data(); }
    void randomTypingOnRealClients()
    {
        QFETCH(int, echo);
        for (quint32 seed = 1; seed <= 60; ++seed) {
            QRandomGenerator rng(seed * 104729u + quint32(echo));
            Session s{static_cast<TextInputClient::Echo>(echo)};
            s.engine.setAutocorrectEnabled(false);
            s.engine.setAutoCapitalizationEnabled(false);
            s.engine.setDoubleSpacePeriodEnabled(false);
            QString expected, log;
            int wordLength = 0;
            bool heldAfterWord = false, swapped = false, heldSpace = false, afterSuggestion = false;
            const QString letters = QStringLiteral("abcdeéжщї");
            for (int step = 0; step < 200; ++step) {
                const int op = rng.bounded(100);
                if (op < 58) {
                    const QString ch(letters.at(rng.bounded(letters.size())));
                    s.engine.typeLetter(ch);
                    expected += ch;
                    log += ch;
                    ++wordLength;
                    heldAfterWord = swapped = heldSpace = afterSuggestion = false;
                } else if (op < 62) {
                    s.engine.enter();
                    log += QStringLiteral("⏎");
                    if (heldSpace) expected.chop(1);
                    expected += QLatin1Char('\n');
                    wordLength = 0;
                    heldAfterWord = swapped = heldSpace = afterSuggestion = false;
                } else if (op < 78) {
                    s.engine.space();
                    log += QStringLiteral("␣");
                    if (afterSuggestion) {
                        afterSuggestion = false;          // confirms the suggestion's space
                    } else if (swapped) {
                        swapped = false;
                    } else {
                        expected += QLatin1Char(' ');
                        heldAfterWord = wordLength > 0;
                        heldSpace = wordLength > 0 || heldSpace;
                    }
                    wordLength = 0;
                } else if (op < 86) {
                    const QString p = QStringList{QStringLiteral(","), QStringLiteral("."), QStringLiteral("!")}.at(rng.bounded(3));
                    s.engine.typeText(p);
                    log += p;
                    if (heldAfterWord) {
                        expected.chop(1);
                        expected += p + QLatin1Char(' ');
                        swapped = heldSpace = true;
                    } else {
                        expected += p;
                        swapped = heldSpace = false;
                    }
                    heldAfterWord = afterSuggestion = false;
                    wordLength = 0;
                } else if (op >= 96 && !s.engine.currentWord().isEmpty()) {
                    // The word itself from the strip: replaced by itself, plus a space.
                    const QString word = s.engine.currentWord();
                    s.engine.chooseSuggestion(word);
                    log += QStringLiteral("[") + word + QStringLiteral("]");
                    expected += QLatin1Char(' ');
                    wordLength = 0;
                    heldAfterWord = heldSpace = afterSuggestion = true;
                    swapped = false;
                } else {
                    s.engine.backspace();
                    log += QStringLiteral("⌫");
                    if (swapped) {
                        const QChar punct = expected.at(expected.size() - 2);
                        expected.chop(2);
                        expected += QLatin1Char(' ');
                        expected += punct;
                    } else if (!expected.isEmpty()) {
                        expected.chop(1);
                    }
                    wordLength = 0;
                    while (wordLength < expected.size() && letters.contains(expected.at(expected.size() - 1 - wordLength)))
                        ++wordLength;
                    heldAfterWord = swapped = heldSpace = afterSuggestion = false;
                }
                s.client.deliver();
                s.now += 40 + rng.bounded(300);
                if (s.client.visible() != expected) qWarning() << "seed" << seed << log.right(40);
                QCOMPARE(s.client.visible(), expected);
                QCOMPARE(s.client.badDeletes, 0);
            }
        }
    }
    // At the start of an empty paragraph the first letter is committed (rich
    // editors break a composition there); a suggestion that changes it
    // replaces it too instead of typing after it ("ythe").
    void aSuggestionCanChangeTheFirstLetterOfAParagraph_data() { echoModes_data(); }
    void aSuggestionCanChangeTheFirstLetterOfAParagraph()
    {
        QFETCH(int, echo);
        Session s{static_cast<TextInputClient::Echo>(echo)};
        s.engine.setAutoCapitalizationEnabled(false);
        s.type(QStringLiteral("yhe"));
        s.act([&] { s.engine.chooseSuggestion(QStringLiteral("the")); });
        QCOMPARE(s.client.visible(), QStringLiteral("the "));
        QCOMPARE(s.client.badDeletes, 0);
        for (const QString &typed : {QStringLiteral("Cst"), QStringLiteral("Xat")}) {
            Session line{static_cast<TextInputClient::Echo>(echo)};
            line.client.setText(QStringLiteral("ok\n"));
            line.type(typed);
            line.act([&] { line.engine.chooseSuggestion(QStringLiteral("Cat")); });
            QCOMPARE(line.client.visible(), QStringLiteral("ok\nCat "));
            QCOMPARE(line.client.badDeletes, 0);
        }
    }
    // LatinIME onStartBatchInput: a glide right after a word or a period
    // starts with a space, and the word the cursor touches is never
    // replaced by the glided one.
    void aGlideRightAfterAWordOrAPeriodGetsASpace_data()
    {
        QTest::addColumn<QString>("setup");
        QTest::addColumn<QString>("expected");
        QTest::newRow("typed word") << QStringLiteral("type:a cat") << QStringLiteral("a cat the ");
        QTest::newRow("cursor put after a word") << QStringLiteral("text:a cat") << QStringLiteral("a cat the ");
        QTest::newRow("backspace into a word") << QStringLiteral("type:a cat |") << QStringLiteral("a cat the ");
        QTest::newRow("period") << QStringLiteral("type:hi.") << QStringLiteral("hi. The ");
        QTest::newRow("after a space") << QStringLiteral("type:a ") << QStringLiteral("a the ");
        QTest::newRow("empty field") << QString() << QStringLiteral("The ");
        QTest::newRow("after a bracket") << QStringLiteral("text:(") << QStringLiteral("(the ");
    }
    void aGlideRightAfterAWordOrAPeriodGetsASpace()
    {
        QFETCH(QString, setup);
        QFETCH(QString, expected);
        Session s;
        if (setup.startsWith(QLatin1String("text:"))) s.client.setText(setup.mid(5));
        if (setup.startsWith(QLatin1String("type:"))) {
            QString typed = setup.mid(5);
            const bool thenBackspace = typed.endsWith(QLatin1Char('|'));
            if (thenBackspace) typed.chop(1);
            for (const QChar ch : typed) {
                if (ch.isLetter()) s.act([&] { s.engine.typeLetter(QString(ch)); });
                else if (ch == QLatin1Char(' ')) s.act([&] { s.engine.space(); });
                else s.act([&] { s.engine.typeText(QString(ch)); });
            }
            if (thenBackspace) s.act([&] { s.engine.backspace(); });
        }
        s.act([&] {
            s.engine.beginGlide(QStringLiteral("t"));
            s.engine.glideThrough(QStringLiteral("h"));
            s.engine.glideThrough(QStringLiteral("e"));
            QVERIFY(!s.engine.endGlide().isEmpty());
        });
        QCOMPARE(s.client.visible(), expected);
    }
};

QTEST_GUILESS_MAIN(ClientSimTest)
#include "tst_clientsim.moc"
