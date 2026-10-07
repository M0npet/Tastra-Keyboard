// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>

namespace Tastra
{

// Key combinations for the text-editing panel (Ctrl+A, Ctrl+C, Shift+Left,
// ...). An input method's own key events reach applications without the
// modifiers (KWin replaces them), so the platform sends these as keyboard
// input instead (KWin's fake-input protocol). Key codes are Linux evdev
// codes (linux/input-event-codes.h), as on a hardware keyboard.
class KeyChordSender
{
public:
    virtual ~KeyChordSender() = default;
    // Presses the modifiers in order, taps `key`, releases the modifiers.
    virtual void send(const QList<int> &modifiers, int key) = 0;
};

namespace EvdevKey
{
constexpr int LeftCtrl = 29;
constexpr int LeftShift = 42;
constexpr int A = 30;
constexpr int C = 46;
constexpr int X = 45;
constexpr int V = 47;
constexpr int Z = 44;
constexpr int Home = 102;
constexpr int Up = 103;
constexpr int Left = 105;
constexpr int Right = 106;
constexpr int End = 107;
constexpr int Down = 108;
}

}
