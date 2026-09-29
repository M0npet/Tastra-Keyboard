// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "core/clipboardhistory.h"
#include "core/emojicatalog.h"

class ClipboardEmojiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clipboardHistoryDeduplicatesAndKeepsNewestFirst()
    {
        V3Keyboard::ClipboardHistory history;
        history.clear();
        history.setEnabled(true);
        history.capture(QStringLiteral("one"));
        history.capture(QStringLiteral("two"));
        history.capture(QStringLiteral("one"));

        QCOMPARE(history.items().value(0), QStringLiteral("one"));
        QCOMPARE(history.items().value(1), QStringLiteral("two"));
        QCOMPARE(history.items().size(), 2);
    }

    void emojiCatalogHasCategoriesAndSearch()
    {
        V3Keyboard::EmojiCatalog catalog;
        QVERIFY(catalog.size() >= 40);
        QVERIFY(catalog.categories().contains(QStringLiteral("Smileys")));
        QVERIFY(catalog.glyphs(QStringLiteral("Smileys"), QStringLiteral("grinning"), 10).contains(QStringLiteral("😀")));
    }
};

QTEST_APPLESS_MAIN(ClipboardEmojiTest)
#include "tst_clipboardemoji.moc"
