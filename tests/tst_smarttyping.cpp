// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/locallexicon.h"
#include "core/typingengine.h"

class FakeBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override { commits.append(text); }
    void backspace() override { ++backspaces; }
    void deleteForward() override {}
    void moveLeft() override {}
    void moveRight() override {}
    void moveHome() override {}
    void moveEnd() override {}
    void enter() override { ++enters; }

    QStringList commits;
    int backspaces = 0;
    int enters = 0;
};

class SmartTypingTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fallbackLexiconSuggestsPrefixesAndCorrectsTypos()
    {
        V3Keyboard::LocalLexicon lexicon;
        lexicon.setLanguage(QStringLiteral("en"));

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
        engine.space();
        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("the"), QStringLiteral(" ")}));
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
