// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/locallexicon.h"
#include "core/typingengine.h"

class FakeBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override
    {
        // Contract (as KWin does it): a commit replaces the preedit atomically.
        preedit.clear();
        commits.append(text);
        events.append(QStringLiteral("commit:") + text);
    }
    void backspace() override { ++backspaces; events.append(QStringLiteral("bs")); }
    void deleteForward() override {}
    void moveLeft() override {}
    void moveRight() override {}
    void moveHome() override {}
    void moveEnd() override {}
    void enter() override { ++enters; events.append(QStringLiteral("enter")); }

    bool deleteBeforeCursor(const QString &text) override
    {
        if (!textChannel) return false;
        deletions.append(text);
        events.append(QStringLiteral("del:") + text);
        return true;
    }

    bool setPreedit(const QString &text) override
    {
        if (!preeditSupport) return false;
        preedit = text;
        events.append(QStringLiteral("pre:") + text);
        return true;
    }

    QStringList commits;
    QStringList deletions;
    int backspaces = 0;
    int enters = 0;
    bool textChannel = false;
    bool preeditSupport = false;
    QString preedit;
    QStringList events;
};

namespace
{
// Simulates the client echoing its text state back after an IM edit.
void echo(V3Keyboard::TypingEngine &engine, const QString &textBeforeCursor)
{
    const int bytes = textBeforeCursor.toUtf8().size();
    engine.syncSurroundingText(textBeforeCursor, bytes, bytes);
}
}

class SmartTypingTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("V3KeyboardTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_smarttyping"));
        V3Keyboard::LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
    }

    void init() { QSettings().clear(); }

    void fallbackLexiconSuggestsPrefixesAndCorrectsTypos()
    {
        V3Keyboard::LocalLexicon lexicon;
        lexicon.setLanguage(QStringLiteral("en"));
        QVERIFY(lexicon.waitForDictionary(5000));

        QVERIFY(lexicon.dictionarySize() > 20);
        QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}).contains(QStringLiteral("hello")));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
    }

    void autocorrectReplacesCommittedWordBeforeSpace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        engine.setLanguage(QStringLiteral("en"));

        engine.typeLetter(QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("h"));
        engine.space();

        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("the"), QStringLiteral(" ")}));
        QCOMPARE(engine.currentWord(), QString());
    }

    void suggestionChoiceReplacesWordAndAddsSpace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("l"));
        engine.chooseSuggestion(QStringLiteral("hello"));

        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(backend.commits.last(), QStringLiteral(" "));
        QCOMPARE(engine.previousWord(), QStringLiteral("hello"));
    }

    void shortTokensAreNotAggressivelyAutocorrected()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        engine.setLanguage(QStringLiteral("en"));

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("i"));
        engine.space();

        QCOMPARE(backend.backspaces, 0);
        QCOMPARE(backend.commits.last(), QStringLiteral(" "));
        QCOMPARE(engine.previousWord(), QStringLiteral("hi"));
    }

    void doubleSpaceProducesPeriodSpace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("i"));
        engine.space();
        engine.space();

        QCOMPARE(backend.backspaces, 1);
        QCOMPARE(backend.commits.last(), QStringLiteral(". "));
        QVERIFY(engine.wantsAutoUppercase());
    }

    void sentencePunctuationArmsAutoCapitalization()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        QVERIFY(engine.wantsAutoUppercase());
        engine.typeLetter(QStringLiteral("H"));
        QVERIFY(!engine.wantsAutoUppercase());
        engine.typeText(QStringLiteral("."));
        engine.space();
        QVERIFY(engine.wantsAutoUppercase());
    }

    void sensitiveContextSuppressesLearningAndSuggestions()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.setSensitiveContext(true);
        engine.typeLetter(QStringLiteral("s"));
        engine.typeLetter(QStringLiteral("e"));

        QVERIFY(engine.sensitiveContext());
        QCOMPARE(engine.currentWord(), QString());
        QCOMPARE(engine.suggestions(), QStringList());
        QVERIFY(!engine.wantsAutoUppercase());
    }

    void surroundingTextSeedsCurrentAndPreviousWords()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        const QString text = QStringLiteral("hello wor");
        engine.syncSurroundingText(text, text.toUtf8().size(), text.toUtf8().size());

        QCOMPARE(engine.currentWord(), QStringLiteral("wor"));
        QCOMPARE(engine.previousWord(), QStringLiteral("hello"));
    }

    void staleSurroundingEchoDoesNotClobberLocalWord()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        engine.setLanguage(QStringLiteral("en"));

        engine.typeLetter(QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e"));
        QVERIFY(!engine.syncSurroundingText(QStringLiteral("t"), 1, 1));
        QCOMPARE(engine.currentWord(), QStringLiteral("te"));

        engine.typeLetter(QStringLiteral("h"));
        echo(engine, QStringLiteral("te"));
        echo(engine, QStringLiteral("teh"));
        QCOMPARE(engine.currentWord(), QStringLiteral("teh"));
        engine.space();
        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("the"), QStringLiteral(" ")}));
    }


    // ---- Composition (preedit): the current word never leaves the keyboard
    // until it is final, so no deletions and no Backspace keys are needed to
    // correct it. Works even when the client echoes stale text (Firefox).
    static void composing(FakeBackend &backend, V3Keyboard::TypingEngine &engine)
    {
        backend.textChannel = true;
        backend.preeditSupport = true;
        echo(engine, QString());   // text-input client with an empty field
    }

    void compositionCommitsCorrectedWordWithoutDeletions()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("h"));
        QCOMPARE(backend.preedit, QStringLiteral("teh"));
        QVERIFY(backend.commits.isEmpty());
        engine.space();

        QCOMPARE(backend.commits, QStringList({QStringLiteral("the")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));      // pending space
        QCOMPARE(backend.backspaces, 0);
        QVERIFY(backend.deletions.isEmpty());
    }

    void compositionDoubleSpaceAndNextWord()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("i"));
        engine.space();
        engine.space();
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hi"), QStringLiteral(". ")}));
        QCOMPARE(backend.preedit, QString());
        QVERIFY(engine.wantsAutoUppercase());

        engine.typeLetter(QStringLiteral("J"));
        engine.typeLetter(QStringLiteral("o"));
        engine.space();
        engine.typeLetter(QStringLiteral("b"));
        QCOMPARE(backend.commits.mid(2), QStringList({QStringLiteral("Jo"), QStringLiteral(" ")}));
        QCOMPARE(backend.preedit, QStringLiteral("b"));
        QCOMPARE(backend.backspaces, 0);
        QVERIFY(backend.deletions.isEmpty());
    }

    void compositionSuggestionTapCommitsOnce()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.chooseSuggestion(QStringLiteral("hello"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));
        QCOMPARE(backend.backspaces, 0);
    }

    void spaceAfterSuggestionIsNotADoubleSpace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.chooseSuggestion(QStringLiteral("hello"));
        // Live report (0.2.4): tap suggestion, then Space -> "hello. ".
        // The suggestion's own space must not count as the user's first Space.
        engine.space();
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));
        engine.typeLetter(QStringLiteral("w"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello"), QStringLiteral(" ")}));

        // A genuine second Space still produces the period.
        engine.chooseSuggestion(QStringLiteral("world"));
        engine.space();
        engine.space();
        QCOMPARE(backend.commits.last(), QStringLiteral(". "));
    }

    void compositionBackspaceEditsPreeditFirst()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e"));
        engine.backspace();
        QCOMPARE(backend.preedit, QStringLiteral("h"));
        QCOMPARE(engine.currentWord(), QStringLiteral("h"));
        engine.backspace();
        QCOMPARE(backend.preedit, QString());
        QCOMPARE(backend.backspaces, 0);
        engine.backspace();                                  // now a real key
        QCOMPARE(backend.backspaces, 1);
    }

    void compositionEnterCommitsWordBeforeReturn()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("o"));
        engine.typeLetter(QStringLiteral("k"));
        engine.enter();
        // commit first, key second: GTK applies text-input commits before
        // queued key events, so this order is never inverted.
        QCOMPARE(backend.events.mid(backend.events.indexOf(QStringLiteral("commit:ok"))),
                 QStringList({QStringLiteral("commit:ok"), QStringLiteral("enter")}));
    }

    void compositionIgnoresEchoesAndClientResetDropsIt()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("w"));
        engine.typeLetter(QStringLiteral("o"));
        echo(engine, QStringLiteral("wo"));     // client includes its preedit
        QCOMPARE(engine.currentWord(), QStringLiteral("wo"));
        engine.resetComposition();              // client reset: it owns the text now
        QCOMPARE(engine.currentWord(), QString());
        QVERIFY(!backend.commits.contains(QStringLiteral("wo")));
    }

    void secureFieldsNeverUsePreedit()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        composing(backend, engine);
        engine.setSensitiveContext(true);
        echo(engine, QString());

        engine.typeLetter(QStringLiteral("s"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("s")}));
        QVERIFY(!backend.events.contains(QStringLiteral("pre:s")));
    }

    void clientsWithoutTextInputKeepImmediateCommits()
    {
        FakeBackend backend;
        backend.preeditSupport = true;          // backend could, client cannot
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("a"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("a")}));
    }

    void oneStepStaleEchoesDoNotFlipCaseOrWord()
    {
        FakeBackend backend;
        backend.textChannel = true;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);
        echo(engine, QStringLiteral("hi. "));          // field content on focus
        // Firefox-like: each commit is followed by an echo of the state
        // *before* that commit (GTK re-reads immediately, content lags).
        engine.typeLetter(QStringLiteral("J")); echo(engine, QStringLiteral("hi. "));
        QCOMPARE(engine.currentWord(), QStringLiteral("J"));
        QVERIFY(!engine.wantsAutoUppercase());
        engine.typeLetter(QStringLiteral("o")); echo(engine, QStringLiteral("hi. J"));
        QCOMPARE(engine.currentWord(), QStringLiteral("Jo"));
        QVERIFY(!engine.wantsAutoUppercase());
    }

    void externalCursorMoveAdoptsClientState()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("l"));
        // The user tapped elsewhere in the field: the client reports a state
        // the keyboard never produced. It must be adopted, not ignored,
        // otherwise a suggestion tap would replace the wrong text.
        echo(engine, QStringLiteral("hello wor"));
        QCOMPARE(engine.currentWord(), QStringLiteral("wor"));
        QCOMPARE(engine.previousWord(), QStringLiteral("hello"));
    }

    void autocorrectUsesTextChannelOnceClientConfirmedWord()
    {
        FakeBackend backend;
        backend.textChannel = true;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("t")); echo(engine, QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e")); echo(engine, QStringLiteral("te"));
        engine.typeLetter(QStringLiteral("h")); echo(engine, QStringLiteral("teh"));
        engine.space();

        // Delete + insert travel on the same text-input channel, so clients
        // that apply text-input events before queued key events (GTK/Firefox)
        // cannot reorder them. No Backspace key events are involved.
        QCOMPARE(backend.backspaces, 0);
        QCOMPARE(backend.deletions, QStringList({QStringLiteral("teh")}));
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("the"), QStringLiteral(" ")}));
    }

    void unconfirmedWordIsNotRewrittenOnTextInputClients()
    {
        FakeBackend backend;
        backend.textChannel = true;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("t")); echo(engine, QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("h"));
        engine.space();

        // The client has not confirmed "teh" yet; deleting now could remove
        // the wrong text. Correctness beats an autocorrection.
        QCOMPARE(backend.backspaces, 0);
        QVERIFY(backend.deletions.isEmpty());
        QCOMPARE(backend.commits, QStringList({QStringLiteral("t"), QStringLiteral("e"), QStringLiteral("h"), QStringLiteral(" ")}));
    }

    void suggestionAndDoubleSpaceUseTextChannel()
    {
        FakeBackend backend;
        backend.textChannel = true;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h")); echo(engine, QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e")); echo(engine, QStringLiteral("he"));
        engine.typeLetter(QStringLiteral("l")); echo(engine, QStringLiteral("hel"));
        engine.chooseSuggestion(QStringLiteral("hello"));
        echo(engine, QStringLiteral(""));
        echo(engine, QStringLiteral("hello"));
        echo(engine, QStringLiteral("hello "));
        // Space right after a suggestion only confirms its space (0.2.4 live
        // report); the next Space is the double-space.
        engine.space();
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("hello"), QStringLiteral(" ")}));
        engine.space();

        QCOMPARE(backend.backspaces, 0);
        QCOMPARE(backend.deletions, QStringList({QStringLiteral("hel"), QStringLiteral(" ")}));
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("hello"), QStringLiteral(" "), QStringLiteral(". ")}));
    }

    void stalePreSpaceEchoDoesNotReopenCommittedWord()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("i"));
        engine.space();
        QCOMPARE(engine.currentWord(), QString());

        QVERIFY(!engine.syncSurroundingText(QStringLiteral("hi"), 2, 2));
        QCOMPARE(engine.currentWord(), QString());

        const QString committed = QStringLiteral("hi ");
        engine.syncSurroundingText(committed, committed.toUtf8().size(), committed.toUtf8().size());
        QCOMPARE(engine.currentWord(), QString());
    }

    void terminalPunctuationImmediatelyArmsCapitalization()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("H"));
        engine.typeLetter(QStringLiteral("i"));
        engine.typeText(QStringLiteral("."));
        QVERIFY(engine.wantsAutoUppercase());
    }


    void repeatedBackspaceUpdatesCompositionInOneOperation()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("l"));
        engine.typeLetter(QStringLiteral("l"));
        engine.typeLetter(QStringLiteral("o"));
        engine.backspaceRepeated(3);

        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(engine.currentWord(), QStringLiteral("he"));
    }

    void glideDecoderCanResolveSimpleTrace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::TypingEngine engine(controller);

        engine.beginGlide(QStringLiteral("h"));
        engine.glideThrough(QStringLiteral("e"));
        engine.glideThrough(QStringLiteral("l"));
        engine.glideThrough(QStringLiteral("l"));
        engine.glideThrough(QStringLiteral("o"));
        QCOMPARE(engine.endGlide().toLower(), QStringLiteral("hello"));
        QCOMPARE(backend.commits.last(), QStringLiteral(" "));
    }
};

QTEST_APPLESS_MAIN(SmartTypingTest)
#include "tst_smarttyping.moc"
