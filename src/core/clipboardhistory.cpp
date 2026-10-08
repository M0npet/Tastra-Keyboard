// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardhistory.h"

#include <QDateTime>
#include <algorithm>
#include <QRegularExpression>
#include <QSettings>

namespace Tastra
{

QStringList clipboardParts(const QString &text, int limit)
{
    QStringList parts;
    const QString whole = text.trimmed();
    if (whole.isEmpty() || limit <= 0) return parts;
    // A pasted book is searched only at its start.
    const QString scanned = whole.left(20000);
    auto consider = [&](QString part) {
        part = part.trimmed();
        if (part.size() < 3 || part == whole || parts.size() >= limit) return;
        for (const QString &kept : std::as_const(parts)) {
            if (kept.contains(part)) return;
        }
        parts.append(part);
    };
    auto each = [&](const QRegularExpression &pattern, const std::function<void(QString)> &take) {
        auto it = pattern.globalMatch(scanned);
        while (it.hasNext() && parts.size() < limit) take(it.next().captured(0));
    };
    static const QRegularExpression email(QStringLiteral(R"([\w.%+-]+@[\w-]+(?:\.[\w-]+)*\.\p{L}{2,})"),
                                          QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression url(QStringLiteral(R"((?:https?://|www\.)[^\s<>"'«»]+)"),
                                        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression phone(QStringLiteral(R"((?<![\w+])\+?\d[\d ()./-]{5,}\d(?!\w))"));
    static const QRegularExpression date(
        QStringLiteral(R"((?<!\d)(?:\d{4}-\d{1,2}-\d{1,2}|\d{1,2}[./-]\d{1,2}[./-](?:\d{4}|\d{2}))(?!\d))"));
    static const QRegularExpression wholeDate(QStringLiteral("^(?:%1)$").arg(date.pattern()));
    static const QRegularExpression time(QStringLiteral(R"((?<!\d)(?:[01]?\d|2[0-3]):[0-5]\d(?!\d))"));
    static const QRegularExpression number(QStringLiteral(R"((?<![\w.,])\d{3,}(?:[.,]\d+)?(?![\w]))"));

    each(email, consider);
    each(url, [&](QString found) {
        while (!found.isEmpty() && QStringLiteral(".,;:!?)]}").contains(found.back())) found.chop(1);
        consider(found);
    });
    each(phone, [&](QString found) {
        found = found.trimmed();
        const qsizetype digits = std::count_if(found.cbegin(), found.cend(), [](QChar c) { return c.isDigit(); });
        if (digits < 7 || digits > 15 || wholeDate.match(found).hasMatch()) return;
        consider(found);
    });
    each(date, consider);
    each(time, consider);
    each(number, consider);
    return parts;
}

ClipboardHistory::ClipboardHistory()
{
    load();
}

qint64 ClipboardHistory::now() const
{
    return m_clock ? m_clock() : QDateTime::currentMSecsSinceEpoch();
}

void ClipboardHistory::setClockForTesting(std::function<qint64()> clock) { m_clock = std::move(clock); }

void ClipboardHistory::load()
{
    QSettings settings;
    m_enabled = settings.value(QStringLiteral("clipboardHistoryEnabled"), true).toBool();
    // Before 0.5 the whole history was stored; from now on only pins are.
    settings.remove(QStringLiteral("clipboardHistory"));
    m_items.clear();
    for (const QString &text : settings.value(QStringLiteral("clipboardPinned")).toStringList()) {
        if (!text.isEmpty()) m_items.append({text, 0, true});
    }
}

void ClipboardHistory::persist() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("clipboardHistoryEnabled"), m_enabled);
    QStringList pinned;
    for (const Item &item : m_items) {
        if (item.pinned) pinned.append(item.text);
    }
    settings.setValue(QStringLiteral("clipboardPinned"), pinned);
}

void ClipboardHistory::expire() const
{
    const qint64 cutoff = now() - LifetimeMs;
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [cutoff](const Item &item) {
        return !item.pinned && item.capturedMs < cutoff;
    }), m_items.end());
}

QString ClipboardHistory::sanitized(const QString &text) const
{
    QString value = text;
    value.replace(QChar(0), QChar::ReplacementCharacter);
    value = value.trimmed();
    if (value.size() > 4000) value = value.left(4000);
    return value;
}

void ClipboardHistory::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!enabled) {
        m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [](const Item &i) { return !i.pinned; }),
                      m_items.end());
    }
    persist();
}

bool ClipboardHistory::enabled() const { return m_enabled; }

void ClipboardHistory::capture(const QString &text)
{
    if (!m_enabled) return;
    const QString value = sanitized(text);
    if (value.isEmpty()) return;
    bool pinned = false;
    for (qsizetype i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).text == value) {
            pinned = m_items.at(i).pinned;
            m_items.removeAt(i);
            break;
        }
    }
    m_items.prepend({value, now(), pinned});
    expire();
    int unpinned = 0;
    for (qsizetype i = 0; i < m_items.size();) {
        if (!m_items.at(i).pinned && ++unpinned > m_limit) m_items.removeAt(i);
        else ++i;
    }
    if (pinned) persist();
}

QStringList ClipboardHistory::items() const
{
    expire();
    QStringList pinned, recent;
    for (const Item &item : m_items) (item.pinned ? pinned : recent).append(item.text);
    return pinned + recent;
}

void ClipboardHistory::removeAt(int index)
{
    const QStringList visible = items();
    if (index < 0 || index >= visible.size()) return;
    const QString text = visible.at(index);
    bool wasPinned = false;
    for (qsizetype i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).text == text) {
            wasPinned = m_items.at(i).pinned;
            m_items.removeAt(i);
            break;
        }
    }
    if (wasPinned) persist();
}

void ClipboardHistory::clear()
{
    if (m_items.isEmpty()) return;
    m_items.clear();
    persist();
}

bool ClipboardHistory::isPinned(const QString &text) const
{
    for (const Item &item : m_items) {
        if (item.text == text) return item.pinned;
    }
    return false;
}

void ClipboardHistory::setPinned(const QString &text, bool pinned)
{
    for (Item &item : m_items) {
        if (item.text == text && item.pinned != pinned) {
            item.pinned = pinned;
            item.capturedMs = now();               // unpinning restarts the hour
            persist();
            return;
        }
    }
}

}
