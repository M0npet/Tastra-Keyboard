// SPDX-License-Identifier: GPL-3.0-or-later

#include "clipboardhistory.h"

#include <QSettings>

namespace V3Keyboard
{

ClipboardHistory::ClipboardHistory()
{
    load();
}

void ClipboardHistory::load()
{
    QSettings settings;
    m_enabled = settings.value(QStringLiteral("clipboardHistoryEnabled"), true).toBool();
    m_items = settings.value(QStringLiteral("clipboardHistory")).toStringList();
    while (m_items.size() > m_limit) m_items.removeLast();
}

void ClipboardHistory::persist() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("clipboardHistoryEnabled"), m_enabled);
    settings.setValue(QStringLiteral("clipboardHistory"), m_items);
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
    persist();
}

bool ClipboardHistory::enabled() const { return m_enabled; }

void ClipboardHistory::capture(const QString &text)
{
    if (!m_enabled) return;
    const QString value = sanitized(text);
    if (value.isEmpty()) return;
    m_items.removeAll(value);
    m_items.prepend(value);
    while (m_items.size() > m_limit) m_items.removeLast();
    persist();
}

QStringList ClipboardHistory::items() const { return m_items; }

void ClipboardHistory::removeAt(int index)
{
    if (index < 0 || index >= m_items.size()) return;
    m_items.removeAt(index);
    persist();
}

void ClipboardHistory::clear()
{
    if (m_items.isEmpty()) return;
    m_items.clear();
    persist();
}

}
