// SPDX-License-Identifier: GPL-3.0-or-later
//
// Hermetic lexicon tests: dictionaries come only from tests/data, learning
// goes to QStandardPaths test locations, never to the user's real config.

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "core/locallexicon.h"

using V3Keyboard::LocalLexicon;

class LexiconTest : public QObject
{
    Q_OBJECT

private:
    static void useFixtureDictionaries()
    {
        LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/hunspell")});
        LocalLexicon::setFrequencySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        LocalLexicon::setBlocklistSearchPaths({QStringLiteral(V3KBD_TEST_DATA "/blocklist")});
        LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/empty/none.txt"));
    }
    static void useNoDictionaries()
    {
        LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        LocalLexicon::setFrequencySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        LocalLexicon::setBlocklistSearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
    }
    static void useFrequencies()
    {
        LocalLexicon::setFrequencySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/frequency")});
    }
    static void loaded(LocalLexicon &lexicon, const QString &language)
    {
        lexicon.setLanguage(language);
        QVERIFY(lexicon.waitForDictionary(5000));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("V3KeyboardTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_lexicon"));
        QSettings().clear();
    }

    void init() { QSettings().clear(); }

    void constructionDoesNotLoadDictionaries()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        QVERIFY(!lexicon.hasSystemDictionary());
    }

    void fixtureDictionaryLoadsInBackground()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QVERIFY(lexicon.hasSystemDictionary());
    }

    void transpositionTypoPrefersCommonWordOverAlphabeticalNeighbours()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        // Previously ties were broken alphabetically: teh -> meh.
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("adn")), QStringLiteral("and"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("thsi")), QStringLiteral("this"));
    }

    void inflectedFormsAreValidAndNeverRewritten()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        for (const QString &word : {QStringLiteral("wants"), QStringLiteral("walked"), QStringLiteral("knows"),
                                    QStringLiteral("times"), QStringLiteral("typed"), QStringLiteral("form"),
                                    QStringLiteral("meh")}) {
            QVERIFY2(lexicon.hasWord(word), qPrintable(word));
            QVERIFY2(lexicon.bestCorrection(word).isEmpty(), qPrintable(word));
        }
    }

    void affixAwareCorrectionOfTransposedInflection()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("wnats")), QStringLiteral("wants"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("helo")), QStringLiteral("hello"));
    }

    void ambiguousTypoIsSuggestedButNotAutocorrected()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        // "tex" is one substitution away from several valid rare words; no
        // candidate is common or a transposition, so do not rewrite.
        QVERIFY(lexicon.bestCorrection(QStringLiteral("tex")).isEmpty());
        QVERIFY(!lexicon.suggestions(QStringLiteral("tex"), {}).isEmpty());
    }

    void withoutDictionaryOnlyCuratedTyposAreAutocorrected()
    {
        useNoDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QVERIFY(!lexicon.hasSystemDictionary());
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
        // No validity oracle: valid-but-unknown words must be left alone.
        QVERIFY(lexicon.bestCorrection(QStringLiteral("knows")).isEmpty());
        QVERIFY(lexicon.bestCorrection(QStringLiteral("wants")).isEmpty());
        QVERIFY(lexicon.bestCorrection(QStringLiteral("form")).isEmpty());
    }

    void acceptedWordsBeatTheCuratedTypoList()
    {
        useNoDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
        lexicon.promoteWord(QStringLiteral("teh"));            // e.g. the user undid the correction
        QVERIFY(lexicon.bestCorrection(QStringLiteral("teh")).isEmpty());
    }

    void oneAccidentalCommitDoesNotLegitimiseATypo()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        lexicon.learnWordWithContext(QStringLiteral("teh"), {});       // slipped through once
        lexicon.learnWordWithContext(QStringLiteral("zorgle"), {});
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
        QVERIFY(!lexicon.hasWord(QStringLiteral("zorgle")));
        QVERIFY(!lexicon.suggestions(QStringLiteral("zor"), {}, 5).contains(QStringLiteral("zorgle")));
        // Used three times: now it is the user's word.
        lexicon.learnWordWithContext(QStringLiteral("zorgle"), {});
        lexicon.learnWordWithContext(QStringLiteral("zorgle"), {});
        QVERIFY(lexicon.hasWord(QStringLiteral("zorgle")));
        QVERIFY(lexicon.suggestions(QStringLiteral("zor"), {}, 5).contains(QStringLiteral("zorgle")));
    }

    void personalDictionaryKeepsCaseAndPersists()
    {
        useFixtureDictionaries();
        LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/empty/none.txt"));
        {
            LocalLexicon lexicon;
            loaded(lexicon, QStringLiteral("en"));
            QVERIFY(lexicon.addUserWord(QStringLiteral("Zelenograd")));
            QVERIFY(!lexicon.addUserWord(QStringLiteral(" ")));             // rejected
            QCOMPARE(lexicon.userWords(), QStringList({QStringLiteral("Zelenograd")}));
        }
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.suggestions(QStringLiteral("zel"), {}).value(0), QStringLiteral("Zelenograd"));
        QVERIFY(lexicon.bestCorrection(QStringLiteral("zelenograd")).isEmpty());
        lexicon.removeUserWord(QStringLiteral("zelenograd"));
        QVERIFY(lexicon.userWords().isEmpty());
        QVERIFY(!lexicon.suggestions(QStringLiteral("zel"), {}, 5).contains(QStringLiteral("Zelenograd")));
    }

    void dictionaryFileAddsWordsForAllLanguages()
    {
        useFixtureDictionaries();
        LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/userdict/dictionary.txt"));
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.suggestions(QStringLiteral("minis"), {}).value(0), QStringLiteral("Minisforum"));
        loaded(lexicon, QStringLiteral("ru"));
        QVERIFY(lexicon.hasWord(QStringLiteral("kyiv")));
        LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/empty/none.txt"));
    }

    void forgettingAlsoRemovesFromThePersonalDictionary()
    {
        useFixtureDictionaries();
        LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/empty/none.txt"));
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        lexicon.addUserWord(QStringLiteral("Quokka"));
        lexicon.forgetWord(QStringLiteral("quokka"));
        QVERIFY(lexicon.userWords().isEmpty());
        QVERIFY(!lexicon.suggestions(QStringLiteral("quo"), {}, 5).contains(QStringLiteral("Quokka")));
        lexicon.addUserWord(QStringLiteral("Quokka"));          // explicit add brings it back
        QVERIFY(lexicon.suggestions(QStringLiteral("quo"), {}, 5).contains(QStringLiteral("Quokka")));
    }


    void frequencyRanksCompletionsFromTheFirstLetter()
    {
        useFixtureDictionaries();
        useFrequencies();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        // "help" is more frequent than "hello" in the fixture list.
        QCOMPARE(lexicon.suggestions(QStringLiteral("hel"), {}).value(0), QStringLiteral("help"));
        // Single letters now give real completions, most frequent first.
        QCOMPARE(lexicon.suggestions(QStringLiteral("t"), {}).value(0), QStringLiteral("the"));
        // Inflected forms come from the list; junk not in the dictionary is dropped.
        QVERIFY(lexicon.suggestions(QStringLiteral("wan"), {}).contains(QStringLiteral("wants")));
        QVERIFY(!lexicon.suggestions(QStringLiteral("qz"), {}).contains(QStringLiteral("qzxjunk")));
    }

    void germanNounsFromFrequencyKeepTheirCapital()
    {
        useFixtureDictionaries();
        useFrequencies();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("de"));
        QCOMPARE(lexicon.suggestions(QStringLiteral("ha"), {}).value(0), QStringLiteral("Haus"));
        QVERIFY(lexicon.suggestions(QStringLiteral("hau"), {}).contains(QStringLiteral("Hause")));
    }

    void offensiveWordsAreNeverSuggestedButStayTypable()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QVERIFY(!lexicon.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("hell")));
        // Exactly what the user typed is still offered.
        QVERIFY(lexicon.suggestions(QStringLiteral("hell"), {}, 5).contains(QStringLiteral("hell")));
        // Never an autocorrection target ("hlel" is a transposition of "hell").
        QVERIFY(lexicon.bestCorrection(QStringLiteral("hlel")).isEmpty());
        lexicon.setBlockOffensive(false);
        QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("hell")));
    }

    void forgettingASuggestionRemovesItForGood()
    {
        useFixtureDictionaries();
        {
            LocalLexicon lexicon;
            loaded(lexicon, QStringLiteral("en"));
            for (int i = 0; i < 3; ++i) lexicon.learnWordWithContext(QStringLiteral("helsinki"), {});
            QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}).contains(QStringLiteral("helsinki")));
            lexicon.forgetWord(QStringLiteral("helsinki"));
            lexicon.forgetWord(QStringLiteral("hello"));            // a dictionary word
            QVERIFY(!lexicon.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("helsinki")));
            QVERIFY(!lexicon.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("hello")));
        }
        LocalLexicon reloaded;
        loaded(reloaded, QStringLiteral("en"));
        QVERIFY(!reloaded.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("hello")));
        QVERIFY(!reloaded.suggestions(QStringLiteral("hel"), {}, 5).contains(QStringLiteral("helsinki")));
    }

    void neighbouringKeysMakeTheCheapestCorrection()
    {
        // AOSP LatinIME: substituting a neighbouring key costs 0.0694, a
        // distant one 0.3806. "tge": g is next to h on QWERTY, far from i/o.
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        lexicon.setKeyboardRows({QStringLiteral("qwertyuiop"), QStringLiteral("asdfghjkl"), QStringLiteral("zxcvbnm")});
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("tge")), QStringLiteral("the"));
        QCOMPARE(lexicon.suggestions(QStringLiteral("tge"), {}, 3).value(0), QStringLiteral("the"));
        // Geometry decides between equally rare words: p is next to o, far
        // from i, so "toe" outranks "tie" (alphabetical order would not).
        const QStringList list = lexicon.suggestions(QStringLiteral("tpe"), {}, 6);
        QVERIFY2(list.contains(QStringLiteral("toe")) && list.contains(QStringLiteral("tie")), qPrintable(list.join(',')));
        QVERIFY2(list.indexOf(QStringLiteral("toe")) < list.indexOf(QStringLiteral("tie")), qPrintable(list.join(',')));
        lexicon.setKeyboardRows({});                               // no geometry -> no preference
        const QStringList flat = lexicon.suggestions(QStringLiteral("tpe"), {}, 6);
        QVERIFY2(flat.indexOf(QStringLiteral("tie")) < flat.indexOf(QStringLiteral("toe")), qPrintable(flat.join(',')));
    }

    void latinImeThresholdGatesAutocorrection()
    {
        useFixtureDictionaries();
        useFrequencies();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        lexicon.setKeyboardRows({QStringLiteral("qwertyuiop"), QStringLiteral("asdfghjkl"), QStringLiteral("zxcvbnm")});
        // Clear transposition of a frequent word: corrected.
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("wnats")), QStringLiteral("wants"));
        // Distant substitution towards rare words: below the modest threshold.
        QVERIFY(lexicon.bestCorrection(QStringLiteral("tex")).isEmpty());
    }

    void perKeystrokePreviewIsCheapButSpaceStillUsesHunspell()
    {
        useFixtureDictionaries();
        useFrequencies();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        // Preview (per keystroke): frequency list / stems / learned / core only.
        QCOMPARE(lexicon.correctionPreview(QStringLiteral("teh")), QStringLiteral("the"));
        QCOMPARE(lexicon.correctionPreview(QStringLiteral("wnats")), QStringLiteral("wants"));   // in the list
        // An inflection only Hunspell knows is found at Space, not in the preview.
        QVERIFY(lexicon.correctionPreview(QStringLiteral("hleped")).isEmpty());
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("hleped")), QStringLiteral("helped"));
    }

    void legacyLearningDoesNotPromoteOldTypos()
    {
        // Data written by <= 1.0.1, where every commit counted as "learned":
        // typos typed many times during testing must not become valid words.
        useFixtureDictionaries();
        {
            QSettings settings;
            QVariantMap words;
            words.insert(QStringLiteral("teh"), 7);
            words.insert(QStringLiteral("hello"), 5);
            settings.setValue(QStringLiteral("learning/en/words"), words);
        }
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("teh")), QStringLiteral("the"));
        QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}).contains(QStringLiteral("hello")));
        // The migration runs once; new explicit signals still work afterwards.
        lexicon.promoteWord(QStringLiteral("teh"));
        LocalLexicon reloaded;
        loaded(reloaded, QStringLiteral("en"));
        QVERIFY(reloaded.bestCorrection(QStringLiteral("teh")).isEmpty());
    }

    void prefixCompletionsUseDictionaryAndCoreWords()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QVERIFY(lexicon.suggestions(QStringLiteral("hel"), {}).contains(QStringLiteral("hello")));
        // Acronyms and comment lines are not completion candidates.
        QVERIFY(!lexicon.suggestions(QStringLiteral("na"), {}).contains(QStringLiteral("nasa")));
        QVERIFY(!lexicon.suggestions(QStringLiteral("ind"), {}).contains(QStringLiteral("indented")));
    }

    void learnedWordsRankFirstAndSurviveReload()
    {
        useFixtureDictionaries();
        {
            LocalLexicon lexicon;
            loaded(lexicon, QStringLiteral("en"));
            for (int i = 0; i < 3; ++i) lexicon.learnWordWithContext(QStringLiteral("helsinki"), {});
            lexicon.flushLearning();
        }
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QCOMPARE(lexicon.suggestions(QStringLiteral("hel"), {}).value(0), QStringLiteral("helsinki"));
    }

    void switchingLanguageKeepsOnlyOneDictionary()
    {
        useFixtureDictionaries();
        LocalLexicon lexicon;
        loaded(lexicon, QStringLiteral("en"));
        QVERIFY(lexicon.hasSystemDictionary());
        loaded(lexicon, QStringLiteral("ru"));   // fixture has no ru_RU
        QVERIFY(!lexicon.hasSystemDictionary());
        QCOMPARE(lexicon.language(), QStringLiteral("ru"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("првиет")), QStringLiteral("привет"));
    }
};

QTEST_GUILESS_MAIN(LexiconTest)
#include "tst_lexicon.moc"
