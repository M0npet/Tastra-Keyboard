// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace V3Keyboard
{

struct EmojiEntry
{
    QString glyph;
    QString name;
    QString category;
};

class EmojiCatalog
{
public:
    EmojiCatalog();

    QStringList categories() const;
    QStringList glyphs(const QString &category = {}, const QString &query = {}, int limit = 240) const;
    // Adds CLDR keywords of the keyboard language (English is always used).
    void setKeywordLanguage(const QString &code);
    // Emoji whose keyword is exactly `word` (keyboard language first, then
    // English), e.g. "pizza" -> 🍕. Empty if none.
    QString emojiForWord(const QString &word) const;
    // Gboard-style "recently used" category (newest first, persisted).
    void noteUsed(const QString &glyph);
    // Gboard: skin-tone variants are long-press choices of their base emoji
    // (light ... dark), not separate grid entries. Empty if none.
    QStringList skinTones(const QString &glyph) const;
    static void setSystemDataPathForTesting(const QString &path);
    void clearRecent();
    int size() const;

private:
    void loadSystemEmojiData();
    void loadFallback();
    void add(const QString &glyph, const QString &name, const QString &category);
    QString categoryForName(const QString &name) const;

    QVector<EmojiEntry> m_entries;
    QStringList m_recent;
    QHash<QString, QStringList> m_tones;   // base (no FE0F) -> tone variants
    QString m_keywordLanguage;
    // glyph without U+FE0F -> keywords joined by '|'; loaded lazily.
    mutable QHash<QString, QString> m_englishKeywords;
    mutable QHash<QString, QString> m_languageKeywords;
    mutable bool m_englishLoaded = false;
    mutable bool m_languageLoaded = false;
    void ensureKeywords() const;
};

}
