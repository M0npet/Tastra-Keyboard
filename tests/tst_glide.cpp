// SPDX-License-Identifier: GPL-3.0-or-later
//
// Glide typing from the finger's path (SHARK2 / LatinIME gesture style): the
// path is compared with each candidate word's ideal path through its key
// centres, so keys the finger merely crosses cost little. Paths here are
// synthetic, generated from the same key geometry with seeded noise.

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QLineF>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>

#include "core/glidegeometry.h"
#include "core/keyboardmodel.h"
#include "core/locallexicon.h"

using Tastra::LocalLexicon;

#include "glidepaths.h"

using GlidePaths::KeyWidth;
using GlidePaths::centresFor;
using GlidePaths::pathFor;

class GlideTest : public QObject
{
    Q_OBJECT

private:
    static void useBundledLists(LocalLexicon &lexicon, const QString &language)
    {
        // The bundled frequency lists, no Hunspell, no personal words.
        LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        LocalLexicon::setFrequencySearchPaths({QStringLiteral(":/tastra/frequency")});
        LocalLexicon::setBlocklistSearchPaths({QStringLiteral(":/tastra/blocklist")});
        LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
        lexicon.setLanguage(language);
        QVERIFY(lexicon.waitForDictionary(10000));
    }

    static double accuracy(const LocalLexicon &lexicon, const QString &language, const QStringList &words,
                           double jitter, QStringList *misses)
    {
        const QHash<QChar, QPointF> centres = centresFor(language);
        int hits = 0;
        quint32 seed = 7;
        for (const QString &word : words) {
            const QString decoded = lexicon.decodeGlidePath(pathFor(word, centres, jitter, ++seed), centres, KeyWidth);
            if (decoded.compare(word, Qt::CaseInsensitive) == 0) ++hits;
            else misses->append(word + QStringLiteral("->") + decoded);
        }
        return double(hits) / words.size();
    }

    static double humanAccuracy(const LocalLexicon &lexicon, const QString &language, const QStringList &words,
                                double sigma, double cut, QStringList *misses)
    {
        const QHash<QChar, QPointF> centres = centresFor(language);
        int hits = 0;
        quint32 seed = 3;
        for (const QString &word : words) {
            const QString decoded = lexicon.decodeGlidePath(GlidePaths::humanPathFor(word, centres, sigma, cut, ++seed),
                                                            centres, KeyWidth);
            if (decoded.compare(word, Qt::CaseInsensitive) == 0) ++hits;
            else misses->append(word + QStringLiteral("->") + decoded);
        }
        return double(hits) / words.size();
    }

    static QStringList commonEnglishWords()
    {
        return {
            QStringLiteral("hello"), QStringLiteral("world"), QStringLiteral("the"), QStringLiteral("and"),
            QStringLiteral("you"), QStringLiteral("that"), QStringLiteral("have"), QStringLiteral("with"),
            QStringLiteral("this"), QStringLiteral("what"), QStringLiteral("great"), QStringLiteral("thanks"),
            QStringLiteral("please"), QStringLiteral("people"), QStringLiteral("because"), QStringLiteral("really"),
            QStringLiteral("think"), QStringLiteral("about"), QStringLiteral("would"), QStringLiteral("there"),
            QStringLiteral("right"), QStringLiteral("going"), QStringLiteral("little"), QStringLiteral("make"),
            QStringLiteral("time"), QStringLiteral("good"), QStringLiteral("know"), QStringLiteral("just"),
            QStringLiteral("like"), QStringLiteral("want"), QStringLiteral("work"), QStringLiteral("home"),
            QStringLiteral("today"), QStringLiteral("night"), QStringLiteral("love"), QStringLiteral("friend"),
            QStringLiteral("school"), QStringLiteral("water"), QStringLiteral("money"), QStringLiteral("phone"),
        };
    }

    static QStringList commonRussianWords()
    {
        return {
            QStringLiteral("привет"), QStringLiteral("спасибо"), QStringLiteral("хорошо"), QStringLiteral("сейчас"),
            QStringLiteral("сегодня"), QStringLiteral("может"), QStringLiteral("когда"), QStringLiteral("потому"),
            QStringLiteral("работа"), QStringLiteral("человек"), QStringLiteral("время"), QStringLiteral("дома"),
            QStringLiteral("завтра"), QStringLiteral("вопрос"), QStringLiteral("деньги"), QStringLiteral("понимаю"),
        };
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_glide"));
        QSettings().clear();
    }

    void resamplingSpacesPointsEvenly()
    {
        const QVector<QPointF> line = {QPointF(0, 0), QPointF(10, 0), QPointF(10, 10)};
        const QVector<QPointF> sampled = Tastra::Glide::resample(line, 5);
        QCOMPARE(sampled.size(), 5);
        QCOMPARE(sampled.first(), QPointF(0, 0));
        QCOMPARE(sampled.last(), QPointF(10, 10));
        QCOMPARE(sampled.at(2), QPointF(10, 0));             // half of the 20-unit length
        QVERIFY(qAbs(sampled.at(1).x() - 5.0) < 1e-9);
    }

    void idealPathSkipsRepeatsAndApostrophes()
    {
        const QHash<QChar, QPointF> centres = centresFor(QStringLiteral("en"));
        const QVector<QPointF> hello = Tastra::Glide::idealPath(QStringLiteral("hello"), centres);
        QCOMPARE(hello.size(), 4);                                    // h e l o
        QCOMPARE(Tastra::Glide::idealPath(QStringLiteral("don't"), centres).size(), 4);   // d o n t
        // A letter without a key is glided over its base letter's key.
        QCOMPARE(Tastra::Glide::idealPath(QStringLiteral("hällo"), centres), Tastra::Glide::idealPath(QStringLiteral("hallo"), centres));
        QVERIFY(Tastra::Glide::idealPath(QStringLiteral("h2o"), centres).isEmpty());     // no 2 key
    }

    void crossingExtraKeysDoesNotLoseTheWord()
    {
        // The path that failed with the key-sequence decoder: straight
        // strokes h -> e -> l -> o cross g, r, t, j and k on the way.
        LocalLexicon lexicon;
        useBundledLists(lexicon, QStringLiteral("en"));
        const QHash<QChar, QPointF> centres = centresFor(QStringLiteral("en"));
        QCOMPARE(lexicon.decodeGlidePath(pathFor(QStringLiteral("hello"), centres, 0.0, 1), centres, KeyWidth),
                 QStringLiteral("hello"));
        // A start or end slightly off the key still counts.
        QVector<QPointF> sloppy = pathFor(QStringLiteral("hello"), centres, 0.0, 1);
        sloppy.first() += QPointF(0.4 * KeyWidth, -0.3 * KeyWidth);
        sloppy.last() += QPointF(-0.4 * KeyWidth, 0.3 * KeyWidth);
        QCOMPARE(lexicon.decodeGlidePath(sloppy, centres, KeyWidth), QStringLiteral("hello"));
    }

    void commonEnglishWordsFromCleanAndShakyPaths()
    {
        LocalLexicon lexicon;
        useBundledLists(lexicon, QStringLiteral("en"));
        const QStringList words = commonEnglishWords();
        QStringList misses;
        const double clean = accuracy(lexicon, QStringLiteral("en"), words, 0.12, &misses);
        QVERIFY2(clean >= 0.95, qPrintable(QString::number(clean) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));
        misses.clear();
        const double shaky = accuracy(lexicon, QStringLiteral("en"), words, 0.32, &misses);
        QVERIFY2(shaky >= 0.85, qPrintable(QString::number(shaky) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));
    }

    void cyrillicWordsToo()
    {
        LocalLexicon lexicon;
        useBundledLists(lexicon, QStringLiteral("ru"));
        const QStringList words = commonRussianWords();
        QStringList misses;
        const double rate = accuracy(lexicon, QStringLiteral("ru"), words, 0.25, &misses);
        QVERIFY2(rate >= 0.85, qPrintable(QString::number(rate) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));
    }

    void cornersCutAsRealFingersDo()
    {
        // A finger turns before it reaches the middle keys and draws a curve,
        // not straight strokes: the keys of "the" or "привет" can lie most
        // of a key away from the path. Such words used to be dropped before
        // scoring (a key more than one key width off), or lost to their
        // letters' distance alone.
        LocalLexicon en;
        useBundledLists(en, QStringLiteral("en"));
        QStringList misses;
        const double tidy = humanAccuracy(en, QStringLiteral("en"), commonEnglishWords(), 0.15, 0.3, &misses);
        QVERIFY2(tidy >= 0.95, qPrintable(QString::number(tidy) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));
        misses.clear();
        const double sloppy = humanAccuracy(en, QStringLiteral("en"), commonEnglishWords(), 0.25, 0.5, &misses);
        QVERIFY2(sloppy >= 0.82, qPrintable(QString::number(sloppy) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));

        LocalLexicon ru;
        useBundledLists(ru, QStringLiteral("ru"));
        misses.clear();
        const double russian = humanAccuracy(ru, QStringLiteral("ru"), commonRussianWords(), 0.25, 0.5, &misses);
        QVERIFY2(russian >= 0.95, qPrintable(QString::number(russian) + QLatin1Char(' ') + misses.join(QLatin1Char(' '))));
    }

    void aShortSlideIsNotALongWord()
    {
        // A slide from one key to its neighbour, as a sloppy tap can be: its
        // keys lie near the slide, but a word whose path runs far longer
        // ("jk" -> "junk", "ва" -> "вывеска") was not glided.
        for (const QString &language : {QStringLiteral("en"), QStringLiteral("de"), QStringLiteral("ru"), QStringLiteral("uk")}) {
            LocalLexicon lexicon;
            useBundledLists(lexicon, language);
            const QHash<QChar, QPointF> centres = centresFor(language);
            const QStringList rows = Tastra::KeyboardModel::rowsForLanguage(language);
            for (int r = 0; r < 3; ++r) {
                for (int i = 0; i + 1 < rows.at(r).size(); ++i) {
                    const QPointF a = centres.value(rows.at(r).at(i)), b = centres.value(rows.at(r).at(i + 1));
                    QVector<QPointF> path;
                    for (int step = 0; step <= 10; ++step) path.append(a + (b - a) * (step / 10.0));
                    const QString decoded = lexicon.decodeGlidePath(path, centres, KeyWidth);
                    QVERIFY2(decoded.size() <= 3, qPrintable(language + QLatin1Char(' ') + rows.at(r).mid(i, 2) + QStringLiteral(" -> ") + decoded));
                }
            }
        }
    }

    void nothingForATinyOrMissingPath()
    {
        LocalLexicon lexicon;
        useBundledLists(lexicon, QStringLiteral("en"));
        const QHash<QChar, QPointF> centres = centresFor(QStringLiteral("en"));
        QVERIFY(lexicon.decodeGlidePath({}, centres, KeyWidth).isEmpty());
        QVERIFY(lexicon.decodeGlidePath({centres.value(QLatin1Char('h'))}, centres, KeyWidth).isEmpty());
        QVERIFY(lexicon.decodeGlidePath(pathFor(QStringLiteral("hello"), centres, 0.0, 1), {}, KeyWidth).isEmpty());
    }
};

QTEST_GUILESS_MAIN(GlideTest)
#include "tst_glide.moc"
