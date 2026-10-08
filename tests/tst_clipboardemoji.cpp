// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include <QCoreApplication>
#include <QStandardPaths>

#include "core/clipboardhistory.h"
#include "core/emojicatalog.h"

class ClipboardEmojiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        // Never read or write the real user configuration from tests.
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_clipboardemoji"));
    }

    void emojiSearchUnderstandsTheKeyboardLanguage()
    {
        Tastra::EmojiCatalog catalog;
        // English names always work.
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("grinning")).contains(QStringLiteral("😀")));
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("улыб")).isEmpty());

        catalog.setKeywordLanguage(QStringLiteral("ru"));
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("улыб")).contains(QStringLiteral("😀")));
        // Variation selectors do not matter: CLDR lists the heart without U+FE0F.
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("сердце")).contains(QStringLiteral("❤️")));
        catalog.setKeywordLanguage(QStringLiteral("uk"));
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("усміш")).contains(QStringLiteral("😀")));
        catalog.setKeywordLanguage(QStringLiteral("de"));
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("lachen")).contains(QStringLiteral("😀")));
        // Exact keyword matches come first: the red heart is in the first row.
        QVERIFY(catalog.glyphs(QStringLiteral("All"), QStringLiteral("herz")).mid(0, 8).contains(QStringLiteral("❤️")));
    }

    void exactKeywordMapsAWordToItsEmoji()
    {
        Tastra::EmojiCatalog catalog;
        QCOMPARE(catalog.emojiForWord(QStringLiteral("Pizza")), QStringLiteral("🍕"));
        QVERIFY(catalog.emojiForWord(QStringLiteral("piz")).isEmpty());      // exact only
        catalog.setKeywordLanguage(QStringLiteral("ru"));
        QCOMPARE(catalog.emojiForWord(QStringLiteral("пицца")), QStringLiteral("🍕"));
    }

    void recentEmojisComeFirstAndPersist()
    {
        {
            Tastra::EmojiCatalog catalog;
            catalog.clearRecent();
            QVERIFY(!catalog.categories().contains(QStringLiteral("Recent")));
            catalog.noteUsed(QStringLiteral("😀"));
            catalog.noteUsed(QStringLiteral("🍕"));
            catalog.noteUsed(QStringLiteral("😀"));
            QCOMPARE(catalog.categories().value(0), QStringLiteral("Recent"));
            QCOMPARE(catalog.glyphs(QStringLiteral("Recent")), QStringList({QStringLiteral("😀"), QStringLiteral("🍕")}));
        }
        Tastra::EmojiCatalog reloaded;
        QCOMPARE(reloaded.glyphs(QStringLiteral("Recent")), QStringList({QStringLiteral("😀"), QStringLiteral("🍕")}));
    }

    void clipboardHistoryDeduplicatesAndKeepsNewestFirst()
    {
        Tastra::ClipboardHistory history;
        history.clear();
        history.setEnabled(true);
        history.capture(QStringLiteral("one"));
        history.capture(QStringLiteral("two"));
        history.capture(QStringLiteral("one"));

        QCOMPARE(history.items().value(0), QStringLiteral("one"));
        QCOMPARE(history.items().value(1), QStringLiteral("two"));
        QCOMPARE(history.items().size(), 2);
    }

    void partsOfACopiedTextAreFound()
    {
        // Gboard "paste sections of text": e-mail, web address, phone, a
        // numeric date or time, other numbers; never the whole text, nothing
        // twice or inside an earlier part, at most three.
        using Tastra::clipboardParts;
        QCOMPARE(clipboardParts(QStringLiteral("Write to anna.k@example.org or call +49 151 2345 6789.")),
                 (QStringList{QStringLiteral("anna.k@example.org"), QStringLiteral("+49 151 2345 6789")}));
        QCOMPARE(clipboardParts(QStringLiteral("See https://kde.org/plasma-desktop/, (www.archlinux.org).")),
                 (QStringList{QStringLiteral("https://kde.org/plasma-desktop/"), QStringLiteral("www.archlinux.org")}));
        QCOMPARE(clipboardParts(QStringLiteral("Termin am 12.03.2027 um 14:30, Raum 4711")),
                 (QStringList{QStringLiteral("12.03.2027"), QStringLiteral("14:30"), QStringLiteral("4711")}));
        QCOMPARE(clipboardParts(QStringLiteral("Ваш код: 583920")), QStringList{QStringLiteral("583920")});
        QCOMPARE(clipboardParts(QStringLiteral("Order 2026-10-08, total 149.90")),
                 (QStringList{QStringLiteral("2026-10-08"), QStringLiteral("149.90")}));
        // The whole text is the chip itself; short numbers are no parts.
        QVERIFY(clipboardParts(QStringLiteral("anna.k@example.org")).isEmpty());
        QVERIFY(clipboardParts(QStringLiteral("I have 2 cats and 12 fish")).isEmpty());
        QVERIFY(clipboardParts(QString()).isEmpty());
        // Numbers inside a phone number or a web address are not offered again.
        QCOMPARE(clipboardParts(QStringLiteral("Ring 0441 123456 now")), QStringList{QStringLiteral("0441 123456")});
        QCOMPARE(clipboardParts(QStringLiteral("a https://x.org/item/123456 b")),
                 QStringList{QStringLiteral("https://x.org/item/123456")});
        QCOMPARE(clipboardParts(QStringLiteral("1111 2222 3333 4444 5555 x"), 3).size(), 3);
    }

    void unpinnedClipsExpireAfterAnHourPinnedOnesStay()
    {
        // Gboard: clipboard items are kept for an hour unless pinned; only
        // pinned items survive a restart (nothing else touches the disk).
        qint64 now = 1'000'000;
        {
            Tastra::ClipboardHistory history;
            history.setClockForTesting([&now] { return now; });
            history.clear();
            history.setEnabled(true);
            history.capture(QStringLiteral("address"));
            history.capture(QStringLiteral("secret-password"));
            history.setPinned(QStringLiteral("address"), true);
            QVERIFY(history.isPinned(QStringLiteral("address")));
            QCOMPARE(history.items().value(0), QStringLiteral("address"));          // pinned first
            now += 59 * 60 * 1000;
            QCOMPARE(history.items().size(), 2);
            now += 2 * 60 * 1000;                                                // 61 minutes
            QCOMPARE(history.items(), QStringList({QStringLiteral("address")}));
            history.capture(QStringLiteral("fresh"));
        }
        Tastra::ClipboardHistory reloaded;
        reloaded.setClockForTesting([&now] { return now; });
        QCOMPARE(reloaded.items(), QStringList({QStringLiteral("address")}));     // unpinned not on disk
        QVERIFY(reloaded.isPinned(QStringLiteral("address")));
        reloaded.setPinned(QStringLiteral("address"), false);
        Tastra::ClipboardHistory again;
        QVERIFY(again.items().isEmpty());
    }

    void skinTonesHideBehindTheBaseEmoji()
    {
        // Real lines from Unicode's emoji-test.txt (19.0): tone variants are not
        // separate grid entries but long-press choices of their base (Gboard).
        Tastra::EmojiCatalog::setSystemDataPathForTesting(QStringLiteral(TASTRA_TEST_DATA "/emoji-system/emoji-test.txt"));
        Tastra::EmojiCatalog catalog;
        const QStringList all = catalog.glyphs(QStringLiteral("All"), {}, 500);
        QVERIFY(all.contains(QStringLiteral("👍")));
        QVERIFY(!all.contains(QStringLiteral("👍🏻")));
        const QStringList tones = catalog.skinTones(QStringLiteral("👍"));
        QCOMPARE(tones.size(), 5);
        QCOMPARE(tones.first(), QStringLiteral("👍🏻"));
        QCOMPARE(catalog.skinTones(QStringLiteral("☝️")).size(), 5);       // base carries FE0F
        QVERIFY(catalog.skinTones(QStringLiteral("😀")).isEmpty());
        QVERIFY(catalog.skinTones(QStringLiteral("🧑‍🤝‍🧑")).size() <= 5);  // mixed-tone pairs are skipped
        Tastra::EmojiCatalog::setSystemDataPathForTesting(QString());
    }

    void emojiCatalogHasCategoriesAndSearch()
    {
        Tastra::EmojiCatalog catalog;
        QVERIFY(catalog.size() >= 40);
        QVERIFY(catalog.categories().contains(QStringLiteral("Smileys")));
        QVERIFY(catalog.glyphs(QStringLiteral("Smileys"), QStringLiteral("grinning"), 10).contains(QStringLiteral("😀")));
    }
};

QTEST_APPLESS_MAIN(ClipboardEmojiTest)
#include "tst_clipboardemoji.moc"
