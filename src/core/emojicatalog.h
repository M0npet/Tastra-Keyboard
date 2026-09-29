// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
    int size() const;

private:
    void loadSystemEmojiData();
    void loadFallback();
    void add(const QString &glyph, const QString &name, const QString &category);
    QString categoryForName(const QString &name) const;

    QVector<EmojiEntry> m_entries;
};

}
