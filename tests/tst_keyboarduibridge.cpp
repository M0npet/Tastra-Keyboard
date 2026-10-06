// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "app/tracelog.h"
#include "app/voicecontroller.h"
#include "core/locallexicon.h"

#include <QFile>
#include <QLoggingCategory>
#include <QTemporaryDir>

Q_LOGGING_CATEGORY(lcTraceProbe, "tastra.test", QtWarningMsg)
Q_LOGGING_CATEGORY(lcOtherProbe, "other.test", QtWarningMsg)

#include "app/keyboarduibridge.h"
#include "app/keyboardhider.h"
#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"

class FakeBackend final : public Tastra::InputMethodBackend
{
public:
    void commitText(const QString &text) override
    {
        preedit.clear();
        commits.append(text);
        events.append(QStringLiteral("commit:") + text);
    }

    void backspace() override
    {
        ++backspaceCount;
    }

    void deleteForward() override
    {
        ++deleteForwardCount;
    }

    void moveUp() override { events.append(QStringLiteral("up")); }
    void moveDown() override { events.append(QStringLiteral("down")); }
    void moveLeft() override
    {
        ++moveLeftCount;
        events.append(QStringLiteral("left"));
    }

    bool setPreedit(const QString &text) override
    {
        preedit = text;
        events.append(QStringLiteral("pre:") + text);
        return true;
    }

    QString preedit;
    QStringList events;

    void moveRight() override
    {
        ++moveRightCount;
    }

    void moveHome() override
    {
        ++moveHomeCount;
    }

    void moveEnd() override
    {
        ++moveEndCount;
    }

    void enter() override
    {
        ++enterCount;
    }

    QStringList commits;
    int backspaceCount = 0;
    int deleteForwardCount = 0;
    int moveLeftCount = 0;
    int moveRightCount = 0;
    int moveHomeCount = 0;
    int moveEndCount = 0;
    int enterCount = 0;
};

class KeyboardUiBridgeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_keyboarduibridge"));
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
    }

    void init() { QSettings().clear(); }

    // KWin sends text-input-v1 enums and maps PIN to password (8); 9 is date.
    void dateFieldIsNotTreatedAsSecret()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        bridge.setContentType(0, 9);
        QVERIFY(!bridge.secureInput());
        bridge.setContentType(0, 8);
        QVERIFY(bridge.secureInput());
        bridge.setContentType(0x40, 0);
        QVERIFY(bridge.secureInput());
    }

    void urlAndEmailFieldsDisableSmartTypingWithoutBeingSecret()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        for (quint32 purpose : {5u, 6u}) {
            bridge.setContentType(0, purpose);
            QVERIFY(!bridge.secureInput());
            QVERIFY(!bridge.uppercase());             // no auto-capitalization
            bridge.tapLetter(QStringLiteral("h"));
            bridge.tapLetter(QStringLiteral("e"));
            QVERIFY(bridge.suggestions().isEmpty());  // no suggestions / learning
            bridge.resetInputContext();
        }
        QCOMPARE(backend.commits.first(), QStringLiteral("h"));
    }

    void lowercaseHintDisablesAutoCapitalization()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        QVERIFY(bridge.uppercase());                   // sentence start by default
        bridge.setContentType(0x8, 0);
        QVERIFY(!bridge.uppercase());
        bridge.resetInputContext();
        QVERIFY(bridge.uppercase());
    }

    void cursorMovesAndLanguageSwitchCommitTheComposedWordFirst()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QStringLiteral("Hi. "), 4, 4);   // mid-text        // text-input client

        bridge.tapLetter(QStringLiteral("o"));
        bridge.tapLetter(QStringLiteral("k"));
        QCOMPARE(backend.preedit, QStringLiteral("Ok"));
        bridge.moveLeft();
        QCOMPARE(backend.events.mid(backend.events.size() - 2),
                 QStringList({QStringLiteral("commit:Ok"), QStringLiteral("left")}));

        bridge.setSurroundingText(QStringLiteral("x"), 1, 1);
        bridge.tapLetter(QStringLiteral("n"));
        bridge.nextLanguage();
        QCOMPARE(backend.commits.last(), QStringLiteral("n"));
    }

    void compositionCanBeSwitchedOff()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QString(), 0, 0);

        QVERIFY(bridge.compositionEnabled());
        bridge.setCompositionEnabled(false);
        bridge.tapLetter(QStringLiteral("a"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("A")}));
        QVERIFY(backend.preedit.isEmpty());
    }

    void traceIsOptInAndOnlyCapturesKeyboardCategories()
    {
        QTemporaryDir dir;
        QVERIFY(!Tastra::enableTraceIfRequested(dir.path()));   // no flag file
        QFile flag(dir.path() + QStringLiteral("/trace.enable"));
        QVERIFY(flag.open(QIODevice::WriteOnly));
        flag.close();
        QVERIFY(Tastra::enableTraceIfRequested(dir.path()));

        qCDebug(lcTraceProbe) << "probe bytes" << 3;
        qCWarning(lcOtherProbe) << "not ours";
        QFile log(dir.path() + QStringLiteral("/trace.log"));
        QVERIFY(log.open(QIODevice::ReadOnly));
        const QByteArray content = log.readAll();
        QVERIFY(content.contains("tastra.test: probe bytes 3"));
        QVERIFY(!content.contains("not ours"));
    }

    void dictationFlowsFromVoiceIntoTheField()
    {
        struct Recorder final : Tastra::AudioRecorder {
            bool start(QString *) override { return true; }
            std::vector<float> stop() override { return std::vector<float>(16000, 0.1f); }
        } recorder;
        struct Recognizer final : Tastra::SpeechRecognizer {
            QString unavailableReason() const override { return {}; }
            void recognize(std::vector<float>, const QString &lang, std::function<void(QString, QString)> done) override
            {
                language = lang;
                done(QStringLiteral("hello there."), {});
            }
            QString language;
        } recognizer;

        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QVERIFY(!bridge.voiceBuilt());
        Tastra::VoiceController voice(&recorder, &recognizer);
        bridge.setVoiceController(&voice);
        QVERIFY(bridge.voiceBuilt());
        bridge.setLanguage(QStringLiteral("de"));

        bridge.toggleVoice();
        QCOMPARE(bridge.voiceState(), QStringLiteral("recording"));
        bridge.toggleVoice();
        QCOMPARE(recognizer.language, QStringLiteral("de"));
        QCOMPARE(backend.commits.last(), QStringLiteral("Hello there. "));
        QVERIFY(bridge.uppercase());                          // sentence ended
        QCOMPARE(bridge.voiceState(), QStringLiteral("idle"));
    }

    void emojiSuggestionFollowsAnExactWordAndInsertsAfterIt()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        for (const QChar ch : QStringLiteral("pizza")) bridge.tapLetter(QString(ch));
        QVERIFY(bridge.suggestions().contains(QStringLiteral("🍕")));
        bridge.selectSuggestion(QStringLiteral("🍕"));
        QCOMPARE(backend.commits.join(QString()), QStringLiteral("pizza 🍕"));
        bridge.setEmojiSuggestionsEnabled(false);
        for (const QChar ch : QStringLiteral(" pizza")) {
            if (ch == QLatin1Char(' ')) bridge.space(); else bridge.tapLetter(QString(ch));
        }
        QVERIFY(!bridge.suggestions().contains(QStringLiteral("🍕")));
    }

    void stripShowsTypedWordAndWhatSpaceWillInsert()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        bridge.setSurroundingText(QStringLiteral("Hi. "), 4, 4);   // mid-text
        for (const QChar ch : QStringLiteral("teh")) bridge.tapLetter(QString(ch));
        // Gboard: typed word (quoted in the UI) left, the correction centre.
        QCOMPARE(bridge.autocorrectSuggestion(), QStringLiteral("the"));
        QCOMPARE(bridge.suggestions().mid(0, 2), QStringList({QStringLiteral("teh"), QStringLiteral("the")}));
        bridge.selectSuggestion(QStringLiteral("teh"));              // keep what was typed
        QCOMPARE(backend.commits, QStringList({QStringLiteral("teh")}));
        for (const QChar ch : QStringLiteral("hel")) bridge.tapLetter(QString(ch));
        QVERIFY(bridge.autocorrectSuggestion().isEmpty());          // a prefix, nothing to correct
    }

    void fieldTypeDrivesLayoutLikeGboard()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QCOMPARE(bridge.inputPurpose(), QStringLiteral("text"));
        bridge.setContentType(0, 3);   QCOMPARE(bridge.inputPurpose(), QStringLiteral("number"));
        bridge.setContentType(0, 2);   QCOMPARE(bridge.inputPurpose(), QStringLiteral("number"));
        bridge.setContentType(0, 4);   QCOMPARE(bridge.inputPurpose(), QStringLiteral("phone"));
        bridge.setContentType(0, 6);   QCOMPARE(bridge.inputPurpose(), QStringLiteral("email"));
        QCOMPARE(bridge.alternatesForKey(QStringLiteral(".")).value(0), QStringLiteral(".com"));
        bridge.setContentType(0, 5);   QCOMPARE(bridge.inputPurpose(), QStringLiteral("url"));
        QVERIFY(bridge.alternatesForKey(QStringLiteral(".")).contains(QStringLiteral(".de")));
        bridge.resetInputContext();    QCOMPARE(bridge.inputPurpose(), QStringLiteral("text"));
        QCOMPARE(bridge.alternatesForKey(QStringLiteral(".")).value(0), QStringLiteral(","));
    }

    void textShortcutsExpandFromTheStrip()
    {
        QTemporaryDir dir;
        QFile file(dir.path() + QStringLiteral("/shortcuts.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("# comment\nadp\tAndroid Police\nomw = on my way\n");
        file.close();
        qputenv("TASTRA_SHORTCUTS_FILE", file.fileName().toUtf8());

        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        for (const QChar ch : QStringLiteral("omw")) bridge.tapLetter(QString(ch));
        QCOMPARE(bridge.suggestions().value(0), QStringLiteral("on my way"));
        bridge.selectSuggestion(QStringLiteral("on my way"));
        // No text-input client here: the typed "omw" is replaced via keys.
        QCOMPARE(backend.backspaceCount, 3);
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("on my way"), QStringLiteral(" ")}));
        qunsetenv("TASTRA_SHORTCUTS_FILE");
    }

    void apostropheOnSymbolsReturnsToLetters()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.toggleSymbols();
        QVERIFY(bridge.symbolsActive());
        bridge.tapText(QStringLiteral("'"));
        QVERIFY(!bridge.symbolsActive());                // Gboard 16.7 default
        QCOMPARE(backend.commits.last(), QStringLiteral("'"));
        bridge.toggleSymbols();
        bridge.tapText(QStringLiteral("5"));
        QVERIFY(bridge.symbolsActive());                 // other symbols keep the layer
        bridge.toggleSymbols();
        QVERIFY(bridge.alternatesForKey(QStringLiteral(".")).size() >= 16);   // Gboard: 16 marks
        QVERIFY(bridge.alternatesForKey(QStringLiteral(".")).contains(QStringLiteral("%")));
    }

    void themesFollowGboardIncludingSystem()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        {
            QSettings().setValue(QStringLiteral("amoled"), true);       // pre-1.0 setting
            Tastra::KeyboardUiBridge migrated(controller, model);
            QCOMPARE(migrated.theme(), QStringLiteral("amoled"));
            QSettings().clear();
        }
        Tastra::KeyboardUiBridge bridge(controller, model);
        QCOMPARE(bridge.theme(), QStringLiteral("system"));
        QVERIFY(bridge.effectiveTheme() == QStringLiteral("dark") || bridge.effectiveTheme() == QStringLiteral("light"));
        const QStringList order = {QStringLiteral("light"), QStringLiteral("dark"), QStringLiteral("amoled"), QStringLiteral("system")};
        for (const QString &next : order) {
            bridge.cycleTheme();
            QCOMPARE(bridge.theme(), next);
        }
        bridge.setTheme(QStringLiteral("light"));
        QCOMPARE(bridge.effectiveTheme(), QStringLiteral("light"));
        QVERIFY(!bridge.amoled());
        bridge.setTheme(QStringLiteral("bogus"));
        QCOMPARE(bridge.theme(), QStringLiteral("light"));
        QVERIFY(!bridge.version().isEmpty());
    }

    void tappingAnUnknownTypedWordOffersToSaveIt()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);

        for (const QChar ch : QStringLiteral("zorgle")) bridge.tapLetter(QString(ch));
        QVERIFY(bridge.typedWordUnknown());
        QCOMPARE(bridge.suggestions().value(0), QStringLiteral("zorgle"));   // shown quoted
        QVERIFY(bridge.saveWordCandidate().isEmpty());
        bridge.selectSuggestion(QStringLiteral("zorgle"));                   // keep it
        QCOMPARE(bridge.saveWordCandidate(), QStringLiteral("zorgle"));      // "＋ Add to dictionary"
        bridge.addWordToDictionary(bridge.saveWordCandidate());
        QVERIFY(bridge.saveWordCandidate().isEmpty());
        QCOMPARE(bridge.userWords(), QStringList({QStringLiteral("zorgle")}));
        for (const QChar ch : QStringLiteral("zor")) bridge.tapLetter(QString(ch));
        QVERIFY(bridge.suggestions().contains(QStringLiteral("zorgle")));

        // The offer disappears with the next key and is not made for known words.
        bridge.space();
        for (const QChar ch : QStringLiteral("blorp")) bridge.tapLetter(QString(ch));
        bridge.selectSuggestion(QStringLiteral("blorp"));
        QCOMPARE(bridge.saveWordCandidate(), QStringLiteral("blorp"));
        bridge.tapLetter(QStringLiteral("x"));
        QVERIFY(bridge.saveWordCandidate().isEmpty());
        bridge.space();
        for (const QChar ch : QStringLiteral("hello")) bridge.tapLetter(QString(ch));
        bridge.selectSuggestion(QStringLiteral("hello"));                     // a known word
        QVERIFY(bridge.saveWordCandidate().isEmpty());
        bridge.removeWordFromDictionary(QStringLiteral("zorgle"));
        QVERIFY(bridge.userWords().isEmpty());
    }

    void savingAnUnknownWordAlsoWorksWithComposition()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QStringLiteral("Hi. "), 4, 4);   // mid-text                  // Firefox: preedit mode
        for (const QChar ch : QStringLiteral("zorgle")) bridge.tapLetter(QString(ch));
        QCOMPARE(backend.preedit, QStringLiteral("Zorgle"));         // sentence start
        QVERIFY(bridge.typedWordUnknown());
        QCOMPARE(bridge.suggestions().value(0), QStringLiteral("Zorgle"));
        bridge.selectSuggestion(QStringLiteral("Zorgle"));
        QCOMPARE(bridge.saveWordCandidate(), QStringLiteral("Zorgle"));
        bridge.addWordToDictionary(bridge.saveWordCandidate());
        QCOMPARE(bridge.userWords(), QStringList({QStringLiteral("Zorgle")}));
    }

    void suggestionsFollowTheCaseOfTheTypedWord()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QString(), 0, 0);
        for (const QChar ch : QStringLiteral("hel")) bridge.tapLetter(QString(ch));   // "Hel" (auto-cap)
        QVERIFY(bridge.suggestions().contains(QStringLiteral("Hello")));
        QVERIFY(!bridge.suggestions().contains(QStringLiteral("hello")));
    }

    void hideButtonCommitsTheWordAndAsksKWinToHide()
    {
        struct FakeHider final : Tastra::KeyboardHider {
            int calls = 0;
            void hideKeyboard() override { ++calls; }
        } hider;
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setKeyboardHider(&hider);
        bridge.setSurroundingText(QStringLiteral("Hi. "), 4, 4);
        for (const QChar ch : QStringLiteral("wor")) bridge.tapLetter(QString(ch));
        QCOMPARE(backend.preedit, QStringLiteral("Wor"));
        bridge.hideKeyboard();
        QCOMPARE(hider.calls, 1);
        QCOMPARE(backend.commits.last(), QStringLiteral("Wor"));     // nothing lost
        QCOMPARE(backend.preedit, QString());
    }

    void gboardSoundAndTrailPreferencesPersist()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        {
            Tastra::KeyboardUiBridge bridge(controller, model);
            QVERIFY(!bridge.keySound());           // Gboard: off by default
            QVERIFY(bridge.glideTrail());          // Gboard: on by default
            bridge.keyFeedback();                  // silent and harmless while off
            bridge.setKeySound(true);
            bridge.setGlideTrail(false);
            bridge.keyFeedback();                  // must not crash without an audio device
        }
        Tastra::KeyboardUiBridge reloaded(controller, model);
        QCOMPARE(reloaded.keySound(), reloaded.canPlayKeySound());   // only kept if playable
        QVERIFY(!reloaded.glideTrail());
    }

    void onlyEnabledLanguagesAreInTheRotation()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        {
            Tastra::KeyboardUiBridge bridge(controller, model);
            QCOMPARE(bridge.languageCodes(), bridge.allLanguageCodes());          // all on by default
            bridge.setLanguage(QStringLiteral("ru"));
            bridge.setLanguageEnabled(QStringLiteral("en"), false);
            bridge.setLanguageEnabled(QStringLiteral("de"), false);
            // Layout order (en, de, uk, ru) is kept.
            QCOMPARE(bridge.languageCodes(), QStringList({QStringLiteral("uk"), QStringLiteral("ru")}));
            bridge.nextLanguage();
            QCOMPARE(bridge.languageCode(), QStringLiteral("uk"));
            bridge.nextLanguage();
            QCOMPARE(bridge.languageCode(), QStringLiteral("ru"));
            // Turning off the current language moves to the next enabled one.
            bridge.setLanguageEnabled(QStringLiteral("ru"), false);
            QCOMPARE(bridge.languageCode(), QStringLiteral("uk"));
            // The last one cannot be turned off.
            bridge.setLanguageEnabled(QStringLiteral("uk"), false);
            QVERIFY(bridge.isLanguageEnabled(QStringLiteral("uk")));
            QCOMPARE(bridge.languageCodes(), QStringList({QStringLiteral("uk")}));
        }
        Tastra::KeyboardUiBridge reloaded(controller, model);
        QCOMPARE(reloaded.languageCodes(), QStringList({QStringLiteral("uk")}));
        QCOMPARE(reloaded.languageLabels().size(), 1);
    }

    void upDownAndLongPressDelayLikeGboard()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        {
            Tastra::KeyboardUiBridge bridge(controller, model);
            bridge.moveUp();
            bridge.moveDown();
            QCOMPARE(backend.events.mid(backend.events.size() - 2), QStringList({QStringLiteral("up"), QStringLiteral("down")}));
            QCOMPARE(bridge.longPressDelay(), 300);              // Gboard default
            bridge.cycleLongPressDelay();
            QCOMPARE(bridge.longPressDelay(), 400);
        }
        Tastra::KeyboardUiBridge reloaded(controller, model);
        QCOMPARE(reloaded.longPressDelay(), 400);
        for (int i = 0; i < 4; ++i) reloaded.cycleLongPressDelay();   // 500, 700, 200, 300
        QCOMPARE(reloaded.longPressDelay(), 300);
    }

    void otherEnabledLanguagesOfTheSameScriptAreCompanions()
    {
        // Gboard's multilingual typing: words of these are never "corrected".
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setLanguage(QStringLiteral("en"));
        QCOMPARE(bridge.companionLanguages(), QStringList{QStringLiteral("de")});
        bridge.setLanguage(QStringLiteral("uk"));
        QCOMPARE(bridge.companionLanguages(), QStringList{QStringLiteral("ru")});
        bridge.setLanguageEnabled(QStringLiteral("ru"), false);
        QVERIFY(bridge.companionLanguages().isEmpty());
    }

    void emoticonsAreATabOfTextFacesLikeGboard()
    {
        // Gboard's emoji keyboard has a ":-)" tab of text emoticons.
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QCOMPARE(bridge.emojiCategories().value(bridge.emojiCategories().size() - 1), QStringLiteral(":-)"));
        const QStringList faces = bridge.emojiSearch(QString(), QStringLiteral(":-)"));
        for (const QString &face : {QStringLiteral(":-)"), QStringLiteral(";-)"), QStringLiteral(":-D"),
                                    QStringLiteral("¯\\_(ツ)_/¯"), QStringLiteral("(╯°□°)╯︵ ┻━┻")}) {
            QVERIFY2(faces.contains(face), qPrintable(face));
        }
        QVERIFY(faces.size() >= 40);
        QCOMPARE(QSet<QString>(faces.cbegin(), faces.cend()).size(), faces.size());   // no duplicates
        // A face goes in exactly as shown and is not an emoji for "Recent".
        const QStringList recentBefore = bridge.recentEmojis();
        bridge.insertEmoticon(QStringLiteral("¯\\_(ツ)_/¯"));
        QCOMPARE(backend.commits.join(QString()), QStringLiteral("¯\\_(ツ)_/¯"));
        QCOMPARE(bridge.recentEmojis(), recentBefore);
    }

    void choosingAWrongLayoutFixSwitchesTheLanguage()
    {
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/frequency-foreign")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
        Tastra::LocalLexicon::preloadForeignWordsForTesting(QStringLiteral("ru"));
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setLanguage(QStringLiteral("en"));
        bridge.setAutoCapitalizationEnabled(false);
        bridge.setSurroundingText(QStringLiteral("Hi. "), 4, 4);
        for (const QChar ch : QStringLiteral("ghbdtn")) bridge.tapLetter(QString(ch));
        QVERIFY2(bridge.suggestions().contains(QStringLiteral("привет")), qPrintable(bridge.suggestions().join(',')));
        bridge.selectSuggestion(QStringLiteral("привет"));
        QVERIFY(backend.commits.contains(QStringLiteral("привет")));
        QCOMPARE(bridge.languageCode(), QStringLiteral("ru"));          // keep typing in Russian
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::clearForeignWordCacheForTesting();
    }

    void tapTextReachesController()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        bridge.tapText(QStringLiteral("a"));

        QCOMPARE(backend.commits, QStringList{QStringLiteral("a")});
    }

    void editingCommandsReachController()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        bridge.moveLeft();
        bridge.moveRight();
        bridge.moveHome();
        bridge.moveEnd();
        bridge.deleteForward();

        QCOMPARE(backend.moveLeftCount, 1);
        QCOMPARE(backend.moveRightCount, 1);
        QCOMPARE(backend.moveHomeCount, 1);
        QCOMPARE(backend.moveEndCount, 1);
        QCOMPARE(backend.deleteForwardCount, 1);
    }


    void ordinaryTypingDoesNotInvalidateWholeKeyboard()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        // Consume the initial sentence-start capitalization with the first key.
        bridge.tapLetter(QStringLiteral("h"));

        QSignalSpy keyboardSpy(&bridge, &Tastra::KeyboardUiBridge::keyboardStateChanged);
        QSignalSpy suggestionSpy(&bridge, &Tastra::KeyboardUiBridge::suggestionsChanged);

        bridge.tapLetter(QStringLiteral("e"));

        QCOMPARE(keyboardSpy.count(), 0);
        QCOMPARE(suggestionSpy.count(), 1);
    }
};

QTEST_APPLESS_MAIN(KeyboardUiBridgeTest)

#include "tst_keyboarduibridge.moc"
