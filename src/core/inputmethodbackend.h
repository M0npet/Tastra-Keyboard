// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace V3Keyboard
{

class InputMethodBackend
{
public:
    virtual ~InputMethodBackend() = default;

    virtual void commitText(const QString &text) = 0;
    virtual void backspace() = 0;
    virtual void deleteForward() = 0;
    virtual void moveLeft() = 0;
    virtual void moveRight() = 0;
    virtual void moveHome() = 0;
    virtual void moveEnd() = 0;
    virtual void enter() = 0;

    // Deletes `text`, which must be exactly the text before the cursor, via
    // the same channel as commitText(). Returns false when the backend or the
    // current client cannot do that; callers then fall back to backspace().
    virtual bool deleteBeforeCursor(const QString &text)
    {
        Q_UNUSED(text);
        return false;
    }
};

}
