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
    }
    static void useNoDictionaries()
    {
        LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
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
        loaded(lexicon, QStringLiteral("de"));   // fixture has no de_DE
        QVERIFY(!lexicon.hasSystemDictionary());
        QCOMPARE(lexicon.language(), QStringLiteral("de"));
        QCOMPARE(lexicon.bestCorrection(QStringLiteral("dsa")), QStringLiteral("das"));
    }
};

QTEST_GUILESS_MAIN(LexiconTest)
#include "tst_lexicon.moc"
