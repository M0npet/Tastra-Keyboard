// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace V3Keyboard
{

class ClipboardHistory
{
public:
    ClipboardHistory();

    void setEnabled(bool enabled);
    bool enabled() const;

    void capture(const QString &text);
    QStringList items() const;
    void removeAt(int index);
    void clear();

private:
    void load();
    void persist() const;
    QString sanitized(const QString &text) const;

    bool m_enabled = true;
    QStringList m_items;
    int m_limit = 20;
};

}
