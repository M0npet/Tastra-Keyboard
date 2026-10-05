// SPDX-License-Identifier: GPL-3.0-or-later
#include "panelvisibility.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcPanel, "v3keyboard.panel", QtWarningMsg)

namespace V3Keyboard
{

PanelVisibility::PanelVisibility(int hideDelayMs, QObject *parent)
    : QObject(parent)
{
    m_hideTimer.setSingleShot(true);
    m_hideTimer.setInterval(hideDelayMs);
    connect(&m_hideTimer, &QTimer::timeout, this, [this] {
        if (!m_visible) return;
        m_visible = false;
        qCInfo(lcPanel) << "hide";
        Q_EMIT visibleChanged(false);
    });
}

void PanelVisibility::setActive(bool active)
{
    qCInfo(lcPanel) << "active" << active << "visible" << m_visible
                    << "pendingHide" << m_hideTimer.isActive();
    if (active) {
        m_hideTimer.stop();
        if (!m_visible) {
            m_visible = true;
            Q_EMIT visibleChanged(true);
        }
        return;
    }
    if (m_visible && !m_hideTimer.isActive()) m_hideTimer.start();
}

bool PanelVisibility::visible() const { return m_visible; }

}
