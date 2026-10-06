// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "panelmanager.h"
#include "toolbarregistry.h"

#include <QString>
#include <QStringList>

namespace Tastra
{

class ToolbarModel
{
public:
    explicit ToolbarModel(const ToolbarRegistry &registry)
        : m_registry(registry)
    {
    }

    QStringList visibleActionIds() const
    {
        QStringList result;
        for (const auto &action : m_registry.actions()) {
            if (action.visible) {
                result.append(action.id);
            }
        }
        return result;
    }

    QVector<ToolbarAction> visibleActions() const
    {
        QVector<ToolbarAction> result;
        for (const auto &action : m_registry.actions()) {
            if (action.visible) {
                result.append(action);
            }
        }
        return result;
    }

    bool activate(const QString &id, PanelManager &panels) const
    {
        const auto *action = m_registry.actionById(id);
        if (!action || !action->visible || !action->enabled) {
            return false;
        }

        switch (action->kind) {
        case ToolbarActionKind::OpenPanel:
            return panels.openPanel(action->panel);
        case ToolbarActionKind::Command:
            return false;
        }
        return false;
    }

private:
    const ToolbarRegistry &m_registry;
};

}
