// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace V3Keyboard
{

enum class PanelId {
    Typing,
    Language,
    Clipboard,
    Emoji,
    TextEditing,
    Settings,
};

inline QString panelIdName(const PanelId panel)
{
    switch (panel) {
    case PanelId::Typing:
        return QStringLiteral("typing");
    case PanelId::Language:
        return QStringLiteral("language");
    case PanelId::Clipboard:
        return QStringLiteral("clipboard");
    case PanelId::Emoji:
        return QStringLiteral("emoji");
    case PanelId::TextEditing:
        return QStringLiteral("text-editing");
    case PanelId::Settings:
        return QStringLiteral("settings");
    }
    return QStringLiteral("typing");
}

class PanelManager
{
public:
    PanelId activePanel() const { return m_activePanel; }

    bool openPanel(const PanelId panel)
    {
        if (m_activePanel == panel) {
            return false;
        }
        m_activePanel = panel;
        return true;
    }

    bool closePanel() { return returnToTyping(); }
    bool returnToTyping() { return openPanel(PanelId::Typing); }

private:
    PanelId m_activePanel = PanelId::Typing;
};

}
