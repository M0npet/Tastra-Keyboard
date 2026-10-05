// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace V3Keyboard
{

// Gboard's "hide keyboard" (⌄). The input-method protocol has no such request,
// so on Plasma it is a D-Bus call to KWin (see KWinKeyboardHider).
class KeyboardHider
{
public:
    virtual ~KeyboardHider() = default;
    virtual void hideKeyboard() = 0;
};

// org.kde.KWin /VirtualKeyboard org.kde.kwin.VirtualKeyboard.active = false
// (KWin src/virtualkeyboard_dbus.{h,cpp}: writable "active" -> setActive()).
class KWinKeyboardHider final : public KeyboardHider
{
public:
    void hideKeyboard() override;
};

}
