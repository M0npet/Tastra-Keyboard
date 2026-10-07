// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>

namespace Tastra
{

// The desktop clipboard as the keyboard sees it. On KWin the Wayland
// clipboard is only offered to the window with keyboard focus, which an
// input panel never has, so the platform layer provides a data-control
// implementation; QClipboard is the fallback (and what tests use).
class SystemClipboard : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual QString text() const = 0;
    // Owns the clipboard with `text` (Copy/Cut in the text-editing panel).
    virtual void setText(const QString &text) = 0;
    virtual void clear() = 0;

Q_SIGNALS:
    // The clipboard holds something new (text() may be empty).
    void changed();
};

}
