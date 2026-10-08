// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>

#include <functional>

namespace Tastra
{

// Gboard "paste sections of text": the e-mail addresses, web addresses, phone
// numbers, numeric dates and times and other numbers found in a copied text,
// each offered on its own (at most `limit`, in that order, none twice, none
// that is part of an earlier one, never the whole text itself).
QStringList clipboardParts(const QString &text, int limit = 3);

// Clipboard history as on Gboard: items are kept for one hour unless pinned.
// Only pinned items are written to disk; everything else lives in memory.
class ClipboardHistory
{
public:
    ClipboardHistory();

    void setEnabled(bool enabled);
    bool enabled() const;
    void capture(const QString &text);
    // Pinned items first, then the newest unpinned ones.
    QStringList items() const;
    void removeAt(int index);
    void clear();

    bool isPinned(const QString &text) const;
    void setPinned(const QString &text, bool pinned);

    void setClockForTesting(std::function<qint64()> clock);

private:
    struct Item {
        QString text;
        qint64 capturedMs = 0;
        bool pinned = false;
    };
    void load();
    void persist() const;
    void expire() const;
    qint64 now() const;
    QString sanitized(const QString &text) const;

    bool m_enabled = true;
    mutable QList<Item> m_items;
    int m_limit = 20;
    std::function<qint64()> m_clock;
    static constexpr qint64 LifetimeMs = 60 * 60 * 1000;
};

}
