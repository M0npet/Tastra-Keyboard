// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwininputpanelintegration.h"

#include "kwininputpanelshellintegration.h"

#include <QWindow>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace Tastra::KWin
{

bool initializeInputPanel(QWindow *window)
{
    if (!window) {
        return false;
    }

    window->create();

    auto *waylandWindow =
        dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle());

    if (!waylandWindow) {
        return false;
    }

    static KWinInputPanelShellIntegration *integration = nullptr;

    if (!integration) {
        integration = new KWinInputPanelShellIntegration();

        if (!integration->initialize(waylandWindow->display())) {
            delete integration;
            integration = nullptr;
            return false;
        }
    }

    waylandWindow->setShellIntegration(integration);
    return true;
}

}
