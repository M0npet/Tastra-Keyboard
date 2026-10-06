// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwininputmethodv1backend.h"

#include <QLoggingCategory>

#include <chrono>
#include <xkbcommon/xkbcommon-keysyms.h>

Q_LOGGING_CATEGORY(lcKWinBackend, "v3keyboard.input.backend", QtWarningMsg)

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

    // Trace lengths only: typed text never reaches the log.
    qCDebug(lcKWinBackend) << "commit_string serial" << m_latestSerial << "utf8 bytes" << text.toUtf8().size();
    m_context->commitString(m_latestSerial, text);
}

bool KWinInputMethodV1Backend::deleteBeforeCursor(const QString &text)
{
    if (!m_context || text.isEmpty()) {
        return false;
    }

    const quint32 bytes = static_cast<quint32>(text.toUtf8().size());
    // KWin maps (index, length) to text-input-v2/v3 (before, after) as
    // before = -index, after = index + length; this deletes `bytes` before
    // the cursor and is applied by the client together with the following
    // commit_string on the same text-input object.
    qCDebug(lcKWinBackend) << "delete_surrounding_text bytes before cursor" << bytes;
    m_context->deleteSurroundingText(-static_cast<qint32>(bytes), bytes);
    return true;
}

bool KWinInputMethodV1Backend::deleteAroundCursor(const QString &before, const QString &after)
{
    if (!m_context || (before.isEmpty() && after.isEmpty())) return false;
    const quint32 b = static_cast<quint32>(before.toUtf8().size());
    const quint32 a = static_cast<quint32>(after.toUtf8().size());
    // KWin: before = -index, after = index + length.
    qCDebug(lcKWinBackend) << "delete_surrounding_text bytes around cursor" << b << a;
    m_context->deleteSurroundingText(-static_cast<qint32>(b), b + a);
    return true;
}

bool KWinInputMethodV1Backend::setPreedit(const QString &text)
{
    if (!m_context) {
        return false;
    }
    // KWin applies the cursor on the next preedit_string (bytes). The commit
    // argument is committed by KWin itself if keyboard focus moves to another
    // surface while the word is still unfinished.
    qCDebug(lcKWinBackend) << "preedit_string utf8 bytes" << text.toUtf8().size();
    m_context->preeditCursor(static_cast<qint32>(text.toUtf8().size()));
    m_context->preeditString(m_latestSerial, text, text);
    return true;
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

    qCDebug(lcKWinBackend) << "keysym" << Qt::hex << sym;
    const quint32 time = monotonicMilliseconds();
    constexpr quint32 released = 0;
    constexpr quint32 pressed = 1;
    constexpr quint32 modifiers = 0;

    m_context->keySym(m_latestSerial, time, sym, pressed, modifiers);
    m_context->keySym(m_latestSerial, time, sym, released, modifiers);
}

}
