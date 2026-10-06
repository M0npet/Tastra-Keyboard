// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardhistory.h"

#include <QDateTime>
#include <algorithm>
#include <QSettings>

namespace Tastra
{

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
