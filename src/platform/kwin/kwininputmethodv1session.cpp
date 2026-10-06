// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwininputmethodv1session.h"

namespace Tastra::KWin
{

KWinInputMethodV1Session::KWinInputMethodV1Session(KWinInputMethodV1Backend &backend)
    : m_backend(backend)
{
}

void KWinInputMethodV1Session::activate(InputMethodV1Context &context)
{
    m_activeContext = &context;
    m_backend.setContext(m_activeContext);
}

void KWinInputMethodV1Session::deactivate(InputMethodV1Context &context)
{
    if (m_activeContext != &context) {
        return;
    }

    m_activeContext = nullptr;
    m_backend.setContext(nullptr);
}

void KWinInputMethodV1Session::commitState(quint32 serial)
{
    if (!m_activeContext) {
        return;
    }

    m_backend.setLatestSerial(serial);
}

}
