// SPDX-License-Identifier: GPL-3.0-or-later

#include "emojicatalog.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace V3Keyboard
{
namespace
{

QString glyphFromCodepoints(const QString &sequence)
{
    QString result;
    const QStringList parts = sequence.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok = false;
        const uint cp = part.toUInt(&ok, 16);
        if (!ok) return {};
        const char32_t cp32 = static_cast<char32_t>(cp);
        result.append(QString::fromUcs4(&cp32, 1));
    }
    return result;
}

}

EmojiCatalog::EmojiCatalog()
{
    loadSystemEmojiData();
    if (m_entries.size() < 50) loadFallback();
}

void EmojiCatalog::add(const QString &glyph, const QString &name, const QString &category)
{
    if (glyph.isEmpty()) return;
    for (const auto &entry : m_entries) {
        if (entry.glyph == glyph) return;
    }
    m_entries.append({glyph, name, category});
}

QString EmojiCatalog::categoryForName(const QString &name) const
{
    const QString n = name.toLower();
    if (n.contains(QStringLiteral("face")) || n.contains(QStringLiteral("smil")) || n.contains(QStringLiteral("heart"))) return QStringLiteral("Smileys");
    if (n.contains(QStringLiteral("hand")) || n.contains(QStringLiteral("person")) || n.contains(QStringLiteral("man")) || n.contains(QStringLiteral("woman"))) return QStringLiteral("People");
    if (n.contains(QStringLiteral("animal")) || n.contains(QStringLiteral("cat")) || n.contains(QStringLiteral("dog")) || n.contains(QStringLiteral("plant")) || n.contains(QStringLiteral("flower"))) return QStringLiteral("Nature");
    if (n.contains(QStringLiteral("food")) || n.contains(QStringLiteral("drink")) || n.contains(QStringLiteral("fruit"))) return QStringLiteral("Food");
    if (n.contains(QStringLiteral("car")) || n.contains(QStringLiteral("train")) || n.contains(QStringLiteral("airplane")) || n.contains(QStringLiteral("travel"))) return QStringLiteral("Travel");
    if (n.contains(QStringLiteral("ball")) || n.contains(QStringLiteral("sport")) || n.contains(QStringLiteral("game"))) return QStringLiteral("Activities");
    if (n.contains(QStringLiteral("flag"))) return QStringLiteral("Flags");
    return QStringLiteral("Symbols");
}

void EmojiCatalog::loadSystemEmojiData()
{
    const QStringList candidates = {
        QStringLiteral("/usr/share/unicode/emoji/emoji-test.txt"),
        QStringLiteral("/usr/share/unicode/emoji-test.txt"),
        QStringLiteral("/usr/share/emoji/emoji-test.txt"),
    };
    QString path;
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) { path = candidate; break; }
    }
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
        if (!line.contains(QStringLiteral("; fully-qualified"))) continue;
        const int semi = line.indexOf(QLatin1Char(';'));
        const int hash = line.indexOf(QLatin1Char('#'));
        if (semi <= 0 || hash <= semi) continue;
        const QString glyph = glyphFromCodepoints(line.left(semi));
        QString annotation = line.mid(hash + 1).trimmed();
        // emoji glyph + E-version + CLDR-style short name
        const QRegularExpression re(QStringLiteral("^\\S+\\s+E[0-9.]+\\s+(.+)$"));
        const auto match = re.match(annotation);
        const QString name = match.hasMatch() ? match.captured(1) : annotation;
        add(glyph, name, categoryForName(name));
    }
}

void EmojiCatalog::loadFallback()
{
    const QVector<EmojiEntry> fallback = {
        {QStringLiteral("😀"), QStringLiteral("grinning face"), QStringLiteral("Smileys")},
        {QStringLiteral("😃"), QStringLiteral("smiling face"), QStringLiteral("Smileys")},
        {QStringLiteral("😄"), QStringLiteral("smile"), QStringLiteral("Smileys")},
        {QStringLiteral("😁"), QStringLiteral("beaming face"), QStringLiteral("Smileys")},
        {QStringLiteral("😂"), QStringLiteral("tears of joy"), QStringLiteral("Smileys")},
        {QStringLiteral("🤣"), QStringLiteral("rolling laughing"), QStringLiteral("Smileys")},
        {QStringLiteral("🥹"), QStringLiteral("holding back tears"), QStringLiteral("Smileys")},
        {QStringLiteral("😊"), QStringLiteral("smiling eyes"), QStringLiteral("Smileys")},
        {QStringLiteral("😍"), QStringLiteral("heart eyes"), QStringLiteral("Smileys")},
        {QStringLiteral("🥰"), QStringLiteral("smiling hearts"), QStringLiteral("Smileys")},
        {QStringLiteral("😎"), QStringLiteral("sunglasses"), QStringLiteral("Smileys")},
        {QStringLiteral("🤔"), QStringLiteral("thinking face"), QStringLiteral("Smileys")},
        {QStringLiteral("😴"), QStringLiteral("sleeping face"), QStringLiteral("Smileys")},
        {QStringLiteral("😭"), QStringLiteral("crying face"), QStringLiteral("Smileys")},
        {QStringLiteral("😡"), QStringLiteral("angry face"), QStringLiteral("Smileys")},
        {QStringLiteral("❤️"), QStringLiteral("red heart"), QStringLiteral("Smileys")},
        {QStringLiteral("🧡"), QStringLiteral("orange heart"), QStringLiteral("Smileys")},
        {QStringLiteral("💛"), QStringLiteral("yellow heart"), QStringLiteral("Smileys")},
        {QStringLiteral("💚"), QStringLiteral("green heart"), QStringLiteral("Smileys")},
        {QStringLiteral("💙"), QStringLiteral("blue heart"), QStringLiteral("Smileys")},
        {QStringLiteral("💜"), QStringLiteral("purple heart"), QStringLiteral("Smileys")},
        {QStringLiteral("👍"), QStringLiteral("thumbs up"), QStringLiteral("People")},
        {QStringLiteral("👎"), QStringLiteral("thumbs down"), QStringLiteral("People")},
        {QStringLiteral("👏"), QStringLiteral("clapping hands"), QStringLiteral("People")},
        {QStringLiteral("🙏"), QStringLiteral("folded hands"), QStringLiteral("People")},
        {QStringLiteral("💪"), QStringLiteral("flexed biceps"), QStringLiteral("People")},
        {QStringLiteral("👀"), QStringLiteral("eyes"), QStringLiteral("People")},
        {QStringLiteral("🐶"), QStringLiteral("dog face"), QStringLiteral("Nature")},
        {QStringLiteral("🐱"), QStringLiteral("cat face"), QStringLiteral("Nature")},
        {QStringLiteral("🐭"), QStringLiteral("mouse face"), QStringLiteral("Nature")},
        {QStringLiteral("🦊"), QStringLiteral("fox"), QStringLiteral("Nature")},
        {QStringLiteral("🐻"), QStringLiteral("bear"), QStringLiteral("Nature")},
        {QStringLiteral("🌸"), QStringLiteral("flower blossom"), QStringLiteral("Nature")},
        {QStringLiteral("🌻"), QStringLiteral("sunflower"), QStringLiteral("Nature")},
        {QStringLiteral("🌲"), QStringLiteral("tree"), QStringLiteral("Nature")},
        {QStringLiteral("🍎"), QStringLiteral("red apple"), QStringLiteral("Food")},
        {QStringLiteral("🍌"), QStringLiteral("banana"), QStringLiteral("Food")},
        {QStringLiteral("🍕"), QStringLiteral("pizza"), QStringLiteral("Food")},
        {QStringLiteral("🍔"), QStringLiteral("hamburger"), QStringLiteral("Food")},
        {QStringLiteral("🍟"), QStringLiteral("fries"), QStringLiteral("Food")},
        {QStringLiteral("☕"), QStringLiteral("coffee"), QStringLiteral("Food")},
        {QStringLiteral("🚗"), QStringLiteral("car"), QStringLiteral("Travel")},
        {QStringLiteral("🚆"), QStringLiteral("train"), QStringLiteral("Travel")},
        {QStringLiteral("✈️"), QStringLiteral("airplane"), QStringLiteral("Travel")},
        {QStringLiteral("🚀"), QStringLiteral("rocket"), QStringLiteral("Travel")},
        {QStringLiteral("⚽"), QStringLiteral("soccer ball"), QStringLiteral("Activities")},
        {QStringLiteral("🏀"), QStringLiteral("basketball"), QStringLiteral("Activities")},
        {QStringLiteral("🎮"), QStringLiteral("video game"), QStringLiteral("Activities")},
        {QStringLiteral("🎉"), QStringLiteral("party popper"), QStringLiteral("Activities")},
        {QStringLiteral("🔥"), QStringLiteral("fire"), QStringLiteral("Symbols")},
        {QStringLiteral("💯"), QStringLiteral("hundred points"), QStringLiteral("Symbols")},
        {QStringLiteral("✨"), QStringLiteral("sparkles"), QStringLiteral("Symbols")},
        {QStringLiteral("✅"), QStringLiteral("check mark"), QStringLiteral("Symbols")},
        {QStringLiteral("❌"), QStringLiteral("cross mark"), QStringLiteral("Symbols")},
        {QStringLiteral("⚡"), QStringLiteral("high voltage"), QStringLiteral("Symbols")},
    };
    for (const auto &entry : fallback) add(entry.glyph, entry.name, entry.category);
}

QStringList EmojiCatalog::categories() const
{
    QStringList order = {QStringLiteral("All"), QStringLiteral("Smileys"), QStringLiteral("People"),
        QStringLiteral("Nature"), QStringLiteral("Food"), QStringLiteral("Travel"),
        QStringLiteral("Activities"), QStringLiteral("Symbols"), QStringLiteral("Flags")};
    QStringList result;
    for (const QString &category : order) {
        if (category == QStringLiteral("All")) { result.append(category); continue; }
        for (const auto &entry : m_entries) {
            if (entry.category == category) { result.append(category); break; }
        }
    }
    return result;
}

QStringList EmojiCatalog::glyphs(const QString &category, const QString &query, int limit) const
{
    const QString q = query.trimmed().toLower();
    const QString cat = category.isEmpty() ? QStringLiteral("All") : category;
    QStringList result;
    for (const auto &entry : m_entries) {
        if (cat != QStringLiteral("All") && entry.category != cat) continue;
        if (!q.isEmpty() && !entry.name.toLower().contains(q) && !entry.glyph.contains(q)) continue;
        result.append(entry.glyph);
        if (result.size() >= limit) break;
    }
    return result;
}

int EmojiCatalog::size() const { return m_entries.size(); }

}
