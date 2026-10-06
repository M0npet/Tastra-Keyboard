// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "kwininputmethodv1backend.h"

namespace Tastra::KWin
{

class KWinInputMethodV1Session
{
public:
    explicit KWinInputMethodV1Session(KWinInputMethodV1Backend &backend);

    void activate(InputMethodV1Context &context);
    void deactivate(InputMethodV1Context &context);
    void commitState(quint32 serial);

private:
    KWinInputMethodV1Backend &m_backend;
    InputMethodV1Context *m_activeContext = nullptr;
};

}
