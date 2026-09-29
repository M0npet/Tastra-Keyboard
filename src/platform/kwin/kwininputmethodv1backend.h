// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/inputmethodbackend.h"

#include <QtGlobal>

namespace V3Keyboard::KWin
{

class InputMethodV1Context
{
public:
    virtual ~InputMethodV1Context() = default;

    virtual void commitString(quint32 serial, const QString &text) = 0;

    virtual void keySym(
        quint32 serial,
        quint32 time,
        quint32 sym,
        quint32 state,
        quint32 modifiers) = 0;
};

class KWinInputMethodV1Backend final : public V3Keyboard::InputMethodBackend
{
public:
    void setContext(InputMethodV1Context *context);
    void setLatestSerial(quint32 serial);

    void commitText(const QString &text) override;
    void backspace() override;
    void deleteForward() override;
    void moveLeft() override;
    void moveRight() override;
    void moveHome() override;
    void moveEnd() override;
    void enter() override;

private:
    void sendKeySym(quint32 sym);
    InputMethodV1Context *m_context = nullptr;
    quint32 m_latestSerial = 0;
};

}
