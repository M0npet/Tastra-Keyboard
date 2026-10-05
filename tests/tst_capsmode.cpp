// SPDX-License-Identifier: GPL-3.0-or-later
// Test vectors from AOSP LatinIME CapsModeUtilsTests (Apache-2.0), sentence
// mode only: true = capitalise the next word.
#include <QtTest/QTest>

#include "core/capsmode.h"

using V3Keyboard::CapsMode::sentenceCaps;

class CapsModeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void englishFollowsLatinIME()
    {
        const auto en = [](const QString &s, bool space = false) { return sentenceCaps(s, space, true, false); };
        QVERIFY(en(QString()));
        QVERIFY(!en(QStringLiteral("Word")));
        QVERIFY(!en(QStringLiteral("Word.")));
        QVERIFY(!en(QStringLiteral("Word ")));
        QVERIFY(en(QStringLiteral("Word. ")));
        QVERIFY(!en(QStringLiteral("Word..")));
        QVERIFY(en(QStringLiteral("Word.. ")));
        QVERIFY(en(QStringLiteral("Word... ")));
        QVERIFY(en(QStringLiteral("Word ... ")));
        QVERIFY(!en(QStringLiteral("Word . ")));
        QVERIFY(!en(QStringLiteral("In the U.S ")));
        QVERIFY(!en(QStringLiteral("In the U.S. ")));          // abbreviation
        QVERIFY(!en(QStringLiteral("Some stuff (e.g. ")));
        QVERIFY(en(QStringLiteral("In the U.S.. ")));
        QVERIFY(en(QStringLiteral("\"Word.\" ")));             // American typography
        QVERIFY(en(QStringLiteral("\"Word\". ")));
        QVERIFY(!en(QStringLiteral("\"Word\" ")));
        QVERIFY(!en(QStringLiteral("Word"), true));
        QVERIFY(en(QStringLiteral("Word."), true));
        QVERIFY(en(QStringLiteral("Word\n")));
        QVERIFY(en(QStringLiteral("Word\n"), true));
        QVERIFY(en(QStringLiteral("Word\n "), true));
        QVERIFY(en(QStringLiteral("Word.\n")));
        QVERIFY(en(QStringLiteral("Word? ")));
        QVERIFY(en(QStringLiteral("Word?"), true));
        QVERIFY(!en(QStringLiteral("Word?")));
        QVERIFY(en(QStringLiteral("Word! ")));
        QVERIFY(!en(QStringLiteral("Word; ")));
        QVERIFY(!en(QStringLiteral("Word;"), true));
    }

    void otherLanguagesKeepQuotesOutsideTheSentence()
    {
        const auto fr = [](const QString &s) { return sentenceCaps(s, false, false, false); };
        QVERIFY(!fr(QStringLiteral("\"Word.\" ")));
        QVERIFY(fr(QStringLiteral("\"Word\". ")));
        QVERIFY(!fr(QStringLiteral("\"Word\" ")));
        QVERIFY(fr(QStringLiteral("Liebe Sara,\n")));          // non-German: new line caps
        QVERIFY(fr(QStringLiteral("Liebe Sara,  \n  ")));
    }

    void germanRules()
    {
        const auto de = [](const QString &s, bool space = false) { return sentenceCaps(s, space, false, true); };
        QVERIFY(!de(QStringLiteral("Liebe Sara,\n")));          // letter line after a comma
        QVERIFY(!de(QStringLiteral("Liebe Sara,\n"), true));
        QVERIFY(!de(QStringLiteral("Liebe Sara,  \n  ")));
        QVERIFY(de(QStringLiteral("Liebe Sara  \n  ")));
        QVERIFY(de(QStringLiteral("Liebe Sara.\n  ")));
        QVERIFY(!de(QStringLiteral("am 3. ")));                 // dates: "am 3. Oktober"
        QVERIFY(de(QStringLiteral("Das ist gut. ")));
    }

    void openingPunctuationIsSkipped()
    {
        QVERIFY(sentenceCaps(QStringLiteral("Hi. ("), false, true, false));
        QVERIFY(sentenceCaps(QStringLiteral("«"), false, false, false));
    }
};

QTEST_GUILESS_MAIN(CapsModeTest)
#include "tst_capsmode.moc"
