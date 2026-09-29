// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwininputmethodv1backend.h"

#include <chrono>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace V3Keyboard::KWin
{
namespace
{

quint32 monotonicMilliseconds()
{
    using namespace std::chrono;

    return static_cast<quint32>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

}

void KWinInputMethodV1Backend::setContext(InputMethodV1Context *context)
{
    m_context = context;
}

void KWinInputMethodV1Backend::setLatestSerial(quint32 serial)
{
    m_latestSerial = serial;
}

void KWinInputMethodV1Backend::commitText(const QString &text)
{
    if (!m_context) {
        return;
    }

    m_context->commitString(m_latestSerial, text);
}

void KWinInputMethodV1Backend::backspace()
{
    sendKeySym(XKB_KEY_BackSpace);
}

void KWinInputMethodV1Backend::deleteForward()
{
    sendKeySym(XKB_KEY_Delete);
}

void KWinInputMethodV1Backend::moveLeft()
{
    sendKeySym(XKB_KEY_Left);
}

void KWinInputMethodV1Backend::moveRight()
{
    sendKeySym(XKB_KEY_Right);
}

void KWinInputMethodV1Backend::moveHome()
{
    sendKeySym(XKB_KEY_Home);
}

void KWinInputMethodV1Backend::moveEnd()
{
    sendKeySym(XKB_KEY_End);
}

void KWinInputMethodV1Backend::enter()
{
    sendKeySym(XKB_KEY_Return);
}

void KWinInputMethodV1Backend::sendKeySym(quint32 sym)
{
    if (!m_context) {
        return;
    }

    const quint32 time = monotonicMilliseconds();
    constexpr quint32 released = 0;
    constexpr quint32 pressed = 1;
    constexpr quint32 modifiers = 0;

    m_context->keySym(m_latestSerial, time, sym, pressed, modifiers);
    m_context->keySym(m_latestSerial, time, sym, released, modifiers);
}

}
