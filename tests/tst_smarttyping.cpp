// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/locallexicon.h"
#include "core/keyboardmodel.h"
#include "core/typingengine.h"

class FakeBackend final : public Tastra::InputMethodBackend
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
    bool deleteAroundCursor(const QString &before, const QString &after) override
    {
        if (!textChannel) return false;
        events.append(QStringLiteral("delAround:") + before + QLatin1Char('|') + after);
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
void echo(Tastra::TypingEngine &engine, const QString &textBeforeCursor)
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
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_smarttyping"));
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
    }

    void init() { QSettings().clear(); }

    void fallbackLexiconSuggestsPrefixesAndCorrectsTypos()
    {
        Tastra::LocalLexicon lexicon;
        lexicon.setLanguage(QStringLiteral("en"));
        QVERIFY(lexicon.waitForDictionary(5000));

        QVERIFY(lexicon.dictionarySize() > 20);
        QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}).contains(QStringLiteral("hello")));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
    }

    void autocorrectReplacesCommittedWordBeforeSpace()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

        const QString text = QStringLiteral("hello wor");
        engine.syncSurroundingText(text, text.toUtf8().size(), text.toUtf8().size());

        QCOMPARE(engine.currentWord(), QStringLiteral("wor"));
        QCOMPARE(engine.previousWord(), QStringLiteral("hello"));
    }

    void staleSurroundingEchoDoesNotClobberLocalWord()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
    // Mid-text by default ("Hi. " before the cursor: a sentence start, but not
    // an empty paragraph); empty paragraphs have their own test below.
    static void composing(FakeBackend &backend, Tastra::TypingEngine &engine,
                          const QString &before = QStringLiteral("Hi. "))
    {
        backend.textChannel = true;
        backend.preeditSupport = true;
        echo(engine, before);
    }

    void aCorrectionThatChangesNothingIsNoCorrection()
    {
        // "I" typed with Shift: the lexicon's answer for "i" is "I" again.
        // That must not count as an autocorrection, or Backspace after the
        // space would "undo" it and keep "i" as a word of the user's.
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/latinime/hunspell")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/latinime/frequency")});
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        engine.setLanguage(QStringLiteral("en"));
        QVERIFY(engine.waitForDictionaryForTesting(5000));
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        engine.typeLetter(QStringLiteral("i"));
        QCOMPARE(engine.autocorrectTarget(), QStringLiteral("I"));
        engine.backspace();
        engine.typeLetter(QStringLiteral("I"));
        QVERIFY(engine.autocorrectTarget().isEmpty());   // nothing to correct
        engine.space();
        engine.backspace();                              // the space; no correction to undo
        engine.space();
        engine.typeLetter(QStringLiteral("i"));
        QCOMPARE(engine.autocorrectTarget(), QStringLiteral("I"));   // "i" was not kept as a word

        // Clients without composition: no delete-and-retype of the same word.
        FakeBackend plain;
        plain.textChannel = true;
        Tastra::KeyboardController plainController(plain);
        Tastra::TypingEngine plainEngine(plainController);
        plainEngine.setLanguage(QStringLiteral("en"));
        QVERIFY(plainEngine.waitForDictionaryForTesting(5000));
        echo(plainEngine, QStringLiteral("Hi. "));
        plainEngine.setAutoCapitalizationEnabled(false);
        plainEngine.typeLetter(QStringLiteral("I"));
        echo(plainEngine, QStringLiteral("Hi. I"));     // the client confirms the letter
        plainEngine.space();
        QVERIFY2(plain.deletions.isEmpty(), qPrintable(plain.events.join(QLatin1Char(' '))));
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    }

    void aWordOfAnotherEnabledLanguageIsKnownAndKept()
    {
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/latinime/hunspell")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/latinime/frequency")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        Tastra::LocalLexicon::preloadForeignWordsForTesting(QStringLiteral("de"));
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        engine.setLanguage(QStringLiteral("en"));
        QVERIFY(engine.waitForDictionaryForTesting(5000));
        engine.setCompanionLanguages({QStringLiteral("de")});
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        for (const QChar ch : QStringLiteral("danke")) engine.typeLetter(QString(ch));
        QVERIFY(engine.currentWordKnown());               // no "+ Add to dictionary" offer
        QVERIFY(engine.autocorrectTarget().isEmpty());    // and no "dance"
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
    }

    void capitalisationFollowsLatinIMEOnTheFieldText()
    {
        auto capsAfter = [](const QString &language, const QString &before) {
            FakeBackend backend;
            Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller);
            engine.setLanguage(language);
            composing(backend, engine, before);
            return engine.wantsAutoUppercase();
        };
        QVERIFY(!capsAfter(QStringLiteral("en"), QStringLiteral("We met in the U.S. ")));   // abbreviation
        QVERIFY(capsAfter(QStringLiteral("en"), QStringLiteral("He said \"Hi.\" ")));     // American quotes
        QVERIFY(!capsAfter(QStringLiteral("de"), QStringLiteral("Wir sehen uns am 3. ")));  // German date
        QVERIFY(!capsAfter(QStringLiteral("de"), QStringLiteral("Liebe Sara,\n")));        // German letter line
        QVERIFY(capsAfter(QStringLiteral("ru"), QStringLiteral("Привет! ")));
        QVERIFY(!capsAfter(QStringLiteral("ru"), QStringLiteral("т.е. ")));                // abbreviation
    }

    void returningToAnAutocorrectedWordOffersWhatWasTyped()
    {
        // Gboard: put the cursor back at an autocorrected word and the strip
        // offers the original; choosing it restores and keeps it.
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        qint64 now = 1000;
        engine.setClockForTesting([&now] { return now; });
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        for (const QChar ch : QStringLiteral("teh")) engine.typeLetter(QString(ch));
        engine.space();                                   // -> "the " (held)
        engine.typeLetter(QStringLiteral("x"));           // correction committed
        engine.space();
        now += 2000;
        engine.resetComposition();                        // tapping into the text: KWin "reset"
        echo(engine, QStringLiteral("Hi. the"));          // cursor now right behind "the"
        QCOMPARE(engine.currentWord(), QStringLiteral("the"));
        QCOMPARE(engine.suggestions().value(0), QStringLiteral("teh"));
        engine.chooseSuggestion(QStringLiteral("teh"));
        QCOMPARE(backend.deletions.last(), QStringLiteral("the"));
        QVERIFY(backend.commits.contains(QStringLiteral("teh")));
        // Kept from now on: typing it again is not corrected.
        for (const QChar ch : QStringLiteral("teh")) engine.typeLetter(QString(ch));
        engine.space();
        QVERIFY2(!(backend.commits.join(QString()) + backend.preedit).endsWith(QStringLiteral("the ")),
                 qPrintable(backend.commits.join(QString()) + backend.preedit));
    }

    void tappingIntoAWordOffersSuggestionsForTheWholeWord()
    {
        // Gboard: put the cursor inside a word and the strip shows options for
        // the whole word; choosing one replaces the whole word.
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        qint64 now = 1000;
        engine.setClockForTesting([&now] { return now; });
        composing(backend, engine);
        engine.addUserWord(QStringLiteral("world"));
        now += 2000;
        engine.resetComposition();                                   // the user taps into the text
        const QString text = QStringLiteral("Hi. wrold is");
        const int cursor = QStringLiteral("Hi. wr").toUtf8().size();
        engine.syncSurroundingText(text, cursor, cursor);
        QCOMPARE(engine.currentWord(), QStringLiteral("wr"));
        QVERIFY2(engine.suggestions().contains(QStringLiteral("world")), qPrintable(engine.suggestions().join(',')));
        QVERIFY(engine.autocorrectTarget().isEmpty());              // no autocorrect mid-word
        engine.chooseSuggestion(QStringLiteral("world"));
        QCOMPARE(backend.events.mid(backend.events.size() - 2),
                 QStringList({QStringLiteral("delAround:wr|old"), QStringLiteral("commit:world")}));
        QVERIFY(!backend.commits.contains(QStringLiteral(" ")));     // " is" already follows: no extra space

        // Space inside a word splits it; the left part is not autocorrected.
        now += 2000;
        engine.resetComposition();
        const QString text2 = QStringLiteral("Hi. tehx is");
        const int cursor2 = QStringLiteral("Hi. teh").toUtf8().size();       // "teh|x": "teh" alone would be corrected
        engine.syncSurroundingText(text2, cursor2, cursor2);
        QCOMPARE(engine.currentWord(), QStringLiteral("teh"));
        const int deletionsBefore = backend.deletions.size();
        engine.space();
        QCOMPARE(backend.deletions.size(), deletionsBefore);
        QCOMPARE(backend.commits.last(), QStringLiteral(" "));
    }

    void wrongLayoutIsRecognisedAndOffered()
    {
        // "ghbdtn" is "привет" typed with the English layout active.
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/frequency-foreign")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/blocklist-foreign")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        Tastra::LocalLexicon::preloadForeignWordsForTesting(QStringLiteral("ru"));
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        const QStringList en = Tastra::KeyboardModel::rowsForLanguage(QStringLiteral("en"));
        const QStringList ru = Tastra::KeyboardModel::rowsForLanguage(QStringLiteral("ru"));
        QHash<QChar, QChar> map;
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < qMin(en.at(r).size(), ru.at(r).size()); ++i) map.insert(en.at(r).at(i), ru.at(r).at(i));
        engine.setForeignLayouts({{QStringLiteral("ru"), map}});
        for (const QChar ch : QStringLiteral("ghbdtn")) engine.typeLetter(QString(ch));
        QCOMPARE(engine.suggestions().value(0), QStringLiteral("привет"));
        QCOMPARE(engine.layoutSuggestionLanguage(), QStringLiteral("ru"));
        engine.space();
        // "hel" is still becoming an English word ("hello"): no "рук".
        for (const QChar ch : QStringLiteral("hel")) engine.typeLetter(QString(ch));
        QVERIFY(engine.layoutSuggestionWord().isEmpty());
        QVERIFY(!engine.suggestions().contains(QStringLiteral("рук")));
        engine.space();
        // Letters that read as no word of the other language: nothing offered
        // (the offensive-word filter itself: foreignWordListsDropOffensiveWords).
        for (const QChar ch : QStringLiteral("kjr")) engine.typeLetter(QString(ch));      // "лок"
        QVERIFY(engine.layoutSuggestionWord().isEmpty());
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        QVERIFY(!Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("x")));
    }

    void foreignWordListsDropOffensiveWords()
    {
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/frequency-foreign")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/blocklist-foreign")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        // The first lookup never blocks typing: it starts a background load.
        QVERIFY(!Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("привет")));
        QTRY_VERIFY(Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("привет")));
        QVERIFY(!Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("блок")));
        QVERIFY(Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("Привет")));
        QVERIFY(!Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QStringLiteral("приветик")));
        QVERIFY(!Tastra::LocalLexicon::knownInLanguage(QStringLiteral("ru"), QString()));
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
    }

    void emptyParagraphCommitsTheFirstLetterBeforeComposing()
    {
        // ProseMirror (claude.ai, ChatGPT) re-renders an empty paragraph's
        // placeholder on the first input and breaks a composition started
        // there; known from ProseMirror's changelog and a Yjs report.
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine, QString());              // empty field
        engine.setAutocorrectEnabled(true);
        for (const QChar ch : QStringLiteral("Teh")) engine.typeLetter(QString(ch));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("T")}));   // first letter at once
        QCOMPARE(backend.preedit, QStringLiteral("eh"));                // the rest composes
        engine.space();                                                 // "Teh" -> "The"
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("The "));
        engine.typeLetter(QStringLiteral("x"));
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("The x"));
        // A new paragraph starts empty again.
        engine.enter();
        engine.typeLetter(QStringLiteral("O"));
        QCOMPARE(backend.commits.last(), QStringLiteral("O"));
        QCOMPARE(backend.preedit, QString());
        // Suggestions never propose replacing the committed letter.
        for (const QString &s : engine.suggestions()) QVERIFY2(s.startsWith(QLatin1Char('O'), Qt::CaseInsensitive), qPrintable(s));
    }

    void compositionCommitsCorrectedWordWithoutDeletions()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("t"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("h"));
        QCOMPARE(backend.preedit, QStringLiteral("teh"));
        QVERIFY(backend.commits.isEmpty());
        engine.space();

        // The correction stays revertible (in the preedit) until the next key.
        QVERIFY(backend.commits.isEmpty());
        QCOMPARE(backend.preedit, QStringLiteral("the "));
        engine.typeLetter(QStringLiteral("x"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("the ")}));
        QCOMPARE(backend.preedit, QStringLiteral("x"));
        QCOMPARE(backend.backspaces, 0);
        QVERIFY(backend.deletions.isEmpty());
    }

    void backspaceRightAfterAutocorrectRevertsAndRemembers()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);

        for (const QChar ch : QStringLiteral("teh")) engine.typeLetter(QString(ch));
        engine.space();
        QCOMPARE(backend.preedit, QStringLiteral("the "));
        engine.backspace();                                   // undo the correction
        QCOMPARE(backend.preedit, QStringLiteral("teh"));
        QCOMPARE(engine.currentWord(), QStringLiteral("teh"));
        engine.space();                                       // keep what was typed
        QCOMPARE(backend.commits, QStringList({QStringLiteral("teh")}));

        // Reverted words are learned and no longer autocorrected.
        for (const QChar ch : QStringLiteral("teh")) engine.typeLetter(QString(ch));
        engine.space();
        QCOMPARE(backend.commits, QStringList({QStringLiteral("teh"), QStringLiteral(" "), QStringLiteral("teh")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));
    }

    void correctionThenDoubleSpaceOrEnterOrPunctuation()
    {
        for (const QString &finish : {QStringLiteral("space"), QStringLiteral("enter"), QStringLiteral(",")}) {
            FakeBackend backend;
            Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller);
            composing(backend, engine);
            for (const QChar ch : QStringLiteral("teh")) engine.typeLetter(QString(ch));
            engine.space();
            if (finish == QStringLiteral("space")) {
                engine.space();
                // ". " is held in the preedit so Backspace can revert it (LatinIME).
                QCOMPARE(backend.commits, QStringList({QStringLiteral("the")}));
                QCOMPARE(backend.preedit, QStringLiteral(". "));
                continue;
            } else if (finish == QStringLiteral("enter")) {
                engine.enter();
                QCOMPARE(backend.events.mid(backend.events.size() - 2),
                         QStringList({QStringLiteral("commit:the"), QStringLiteral("enter")}));
            } else {
                engine.typeText(finish);
                // The space typed after the word moves behind the comma (both
                // held until the next key so Backspace can undo the swap).
                QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("the, "));
                QCOMPARE(backend.backspaces, 0);
                continue;
            }
            QCOMPARE(backend.preedit, QString());
        }
    }

    void compositionDoubleSpaceAndNextWord()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("i"));
        engine.space();
        engine.space();
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hi")}));
        QCOMPARE(backend.preedit, QStringLiteral(". "));   // held until the next letter
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);

        engine.typeLetter(QStringLiteral("h"));
        engine.chooseSuggestion(QStringLiteral("hello"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));
        QCOMPARE(backend.backspaces, 0);
    }

    void placeholderEchoDuringCompositionKeepsPendingSpace()
    {
        // Trace 0.2.5 (claude.ai in Firefox): the empty composer reports one
        // placeholder byte that disappears once text exists, so the first echo
        // after "Slovo" + Space looked foreign; the pending space was dropped
        // and "i" was committed without it -> "Slovoi".
        FakeBackend backend;
        backend.textChannel = true;
        backend.preeditSupport = true;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        echo(engine, QStringLiteral("\n"));
        for (const QChar ch : QStringLiteral("Slovo")) engine.typeLetter(QString(ch));
        engine.space();
        echo(engine, QStringLiteral("Slovo"));
        engine.typeLetter(QStringLiteral("i"));

        // The "\n" placeholder is an empty paragraph: since 1.2.1 "S" is
        // committed before composing (ProseMirror placeholder issue).
        QCOMPARE(backend.commits.first(), QStringLiteral("S"));
        QCOMPARE(backend.commits.join(QString()), QStringLiteral("Slovo "));
        QCOMPARE(backend.preedit, QStringLiteral("i"));
    }

    void selfCausedEchoesAreIgnoredButLaterExternalEditsAdopted()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        qint64 now = 1000;
        engine.setClockForTesting([&now] { return now; });

        engine.typeLetter(QStringLiteral("a"));
        now += 5;                                   // measured echo latency: 2-36 ms
        echo(engine, QStringLiteral("zz"));
        QCOMPARE(engine.currentWord(), QStringLiteral("a"));
        now += 1000;                                // a human-speed external edit
        echo(engine, QStringLiteral("hello wor"));
        QCOMPARE(engine.currentWord(), QStringLiteral("wor"));
    }

    void glidedWordBehavesLikeASuggestion()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);

        engine.beginGlide(QStringLiteral("h"));
        for (const QString &k : {QStringLiteral("e"), QStringLiteral("l"), QStringLiteral("o")}) engine.glideThrough(k);
        QCOMPARE(engine.endGlide(), QStringLiteral("hello"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello")}));
        QCOMPARE(backend.preedit, QStringLiteral(" "));     // auto space, held
        engine.space();                                     // confirms it, no period
        engine.typeLetter(QStringLiteral("w"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("hello"), QStringLiteral(" ")}));
    }

    void dictationCommitsPendingWordThenSentenceCasedText()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);

        engine.insertDictation(QStringLiteral("привет как дела."));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("Привет как дела. ")}));
        QVERIFY(engine.wantsAutoUppercase());               // ended a sentence

        engine.typeLetter(QStringLiteral("o"));
        engine.typeLetter(QStringLiteral("k"));
        engine.insertDictation(QStringLiteral("and more"));
        // The unfinished word is committed first, then a separating space.
        // (Upper-casing typed letters is the bridge's job; the engine got "o".)
        QCOMPARE(backend.commits.mid(1), QStringList({QStringLiteral("ok"), QStringLiteral(" and more ")}));
        QVERIFY(!engine.wantsAutoUppercase());
    }

    void autoSpaceAfterPunctuationIsOptInLikeGboard()
    {
        for (const bool composition : {false, true}) {
            FakeBackend backend;
            Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller);
            if (composition) composing(backend, engine);
            engine.setAutoCapitalizationEnabled(false);

            engine.typeLetter(QStringLiteral("h"));
            engine.typeText(QStringLiteral(","));
            engine.typeLetter(QStringLiteral("o"));                // default: off
            QVERIFY(!backend.events.join(QString()).contains(QStringLiteral(", ")));

            engine.setAutoSpaceAfterPunctuation(true);
            engine.space();
            engine.typeLetter(QStringLiteral("k"));
            engine.typeText(QStringLiteral("."));
            engine.typeLetter(QStringLiteral("n"));
            engine.typeText(QStringLiteral("3"));
            engine.typeText(QStringLiteral("."));                  // "3." is a number:
            engine.typeText(QStringLiteral("5"));                  // no space inside "3.5"
            const QString all = backend.commits.join(QString()) + backend.preedit;
            QVERIFY2(all.contains(QStringLiteral("k. n")), qPrintable(all));
            QVERIFY2(all.contains(QStringLiteral("3.5")), qPrintable(all));
        }
    }

    void spaceBeforePunctuationIsSwappedLikeLatinIME()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        engine.setAutocorrectEnabled(false);

        for (const QChar ch : QStringLiteral("hi")) engine.typeLetter(QString(ch));
        engine.space();
        engine.typeText(QStringLiteral(","));        // "hi ," -> "hi, "
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, "));
        engine.space();                              // absorbed: still one space
        engine.typeLetter(QStringLiteral("x"));
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, x"));

        // LatinIME revertSwapPunctuation: Backspace right after the swap
        // restores what was typed ("hi, " -> "hi ,"), without deletions.
        engine.space();
        engine.typeLetter(QStringLiteral("y"));
        engine.space();
        engine.typeText(QStringLiteral(","));
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, x y, "));
        engine.backspace();
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, x y ,"));
        QCOMPARE(backend.backspaces, 0);
        engine.typeLetter(QStringLiteral("z"));
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, x y ,z"));

        // Consecutive spaces are not a "space after a word": no swap.
        engine.setDoubleSpacePeriodEnabled(false);
        engine.space();
        engine.space();
        engine.typeText(QStringLiteral("."));
        QCOMPARE(backend.commits.join(QString()) + backend.preedit, QStringLiteral("hi, x y ,z  ."));
    }

    void clearingTheFieldReArmsCapitalisation()
    {
        for (const bool composition : {false, true}) {
            FakeBackend backend;
            Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller);
            if (composition) composing(backend, engine);
            else echo(engine, QString());          // empty field known
            engine.typeLetter(QStringLiteral("H"));
            engine.typeLetter(QStringLiteral("i"));
            engine.space();
            QVERIFY(!engine.wantsAutoUppercase());
            for (int i = 0; i < 3; ++i) engine.backspace();   // field is empty again
            QVERIFY2(engine.wantsAutoUppercase(), composition ? "composition" : "immediate");
        }
    }

    // Rules from AOSP LatinIME InputLogic.tryPerformDoubleSpacePeriod /
    // RichInputConnection.revertDoubleSpacePeriod (Apache-2.0).
    void doubleSpacePeriodFollowsLatinIME()
    {
        auto visible = [](FakeBackend &b) { return b.commits.join(QString()) + b.preedit; };
        {   // two quick spaces after a word -> ". "
            FakeBackend backend; Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller); composing(backend, engine);
            qint64 now = 1000; engine.setClockForTesting([&now] { return now; });
            engine.setAutoCapitalizationEnabled(false); engine.setAutocorrectEnabled(false);
            engine.typeLetter(QStringLiteral("h")); engine.typeLetter(QStringLiteral("i"));
            engine.space(); now += 300; engine.space();
            QCOMPARE(visible(backend), QStringLiteral("hi. "));
            // Backspace right after it: ". " -> " ", and no capital next.
            engine.backspace();
            QCOMPARE(visible(backend), QStringLiteral("hi "));
            QVERIFY(!engine.wantsAutoUppercase());
            engine.typeLetter(QStringLiteral("x"));
            QCOMPARE(visible(backend), QStringLiteral("hi x"));
        }
        {   // the second space comes later than 1100 ms -> just a space
            FakeBackend backend; Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller); composing(backend, engine);
            qint64 now = 1000; engine.setClockForTesting([&now] { return now; });
            engine.setAutoCapitalizationEnabled(false); engine.setAutocorrectEnabled(false);
            engine.typeLetter(QStringLiteral("h")); engine.typeLetter(QStringLiteral("i"));
            engine.space(); now += 1500; engine.space();
            QCOMPARE(visible(backend), QStringLiteral("hi  "));
        }
        {   // after punctuation there is no second period
            FakeBackend backend; Tastra::KeyboardController controller(backend);
            Tastra::TypingEngine engine(controller); composing(backend, engine);
            qint64 now = 1000; engine.setClockForTesting([&now] { return now; });
            engine.setAutoCapitalizationEnabled(false); engine.setAutocorrectEnabled(false);
            engine.typeLetter(QStringLiteral("h")); engine.typeLetter(QStringLiteral("i"));
            engine.typeText(QStringLiteral("."));
            engine.space(); now += 200; engine.space();
            QCOMPARE(visible(backend), QStringLiteral("hi.  "));
        }
    }

    void touchOffsetsTravelWithTheWord()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
        composing(backend, engine);
        engine.setAutoCapitalizationEnabled(false);
        engine.setKeyboardRows({QStringLiteral("qwertyuiop"), QStringLiteral("asdfghjkl"), QStringLiteral("zxcvbnm")});
        engine.addUserWord(QStringLiteral("bat"));
        engine.addUserWord(QStringLiteral("bet"));
        engine.typeLetter(QStringLiteral("b"));
        engine.typeLetter(QStringLiteral("s"), QPointF(0.30, -0.42));
        engine.typeLetter(QStringLiteral("t"));
        QCOMPARE(engine.suggestions().value(0), QStringLiteral("bet"));
        engine.backspace();                                   // offsets shrink with the word
        engine.typeLetter(QStringLiteral("t"));
        QCOMPARE(engine.suggestions().value(0), QStringLiteral("bet"));
    }

    void spaceAfterSuggestionIsNotADoubleSpace()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        QCOMPARE(backend.preedit, QStringLiteral(". "));
    }

    void compositionBackspaceEditsPreeditFirst()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("a"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("a")}));
    }

    void oneStepStaleEchoesDoNotFlipCaseOrWord()
    {
        FakeBackend backend;
        backend.textChannel = true;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);
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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

        qint64 now = 0;
        engine.setClockForTesting([&now] { return now; });
        engine.typeLetter(QStringLiteral("h"));
        engine.typeLetter(QStringLiteral("e"));
        engine.typeLetter(QStringLiteral("l"));
        // The user tapped elsewhere in the field (at human speed): the client
        // reports a state the keyboard never produced. It must be adopted,
        // otherwise a suggestion tap would replace the wrong text.
        now += 1000;
        echo(engine, QStringLiteral("hello wor"));
        QCOMPARE(engine.currentWord(), QStringLiteral("wor"));
        QCOMPARE(engine.previousWord(), QStringLiteral("hello"));
    }

    void autocorrectUsesTextChannelOnceClientConfirmedWord()
    {
        FakeBackend backend;
        backend.textChannel = true;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

        engine.typeLetter(QStringLiteral("H"));
        engine.typeLetter(QStringLiteral("i"));
        engine.typeText(QStringLiteral("."));
        QVERIFY(engine.wantsAutoUppercase());
    }


    void repeatedBackspaceUpdatesCompositionInOneOperation()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
        Tastra::KeyboardController controller(backend);
        Tastra::TypingEngine engine(controller);

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
