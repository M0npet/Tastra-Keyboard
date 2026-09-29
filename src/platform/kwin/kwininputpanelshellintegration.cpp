// SPDX-FileCopyrightText: 2017 Jan Arne Petersen
// SPDX-License-Identifier: LGPL-2.1-only

#include "kwininputpanelshellintegration.h"

#include "kwininputpanelsurface.h"

#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace V3Keyboard::KWin
{

KWinInputPanelShellIntegration::KWinInputPanelShellIntegration()
    : QWaylandShellIntegrationTemplate<KWinInputPanelShellIntegration>(1)
{
}

QtWaylandClient::QWaylandShellSurface *
KWinInputPanelShellIntegration::createShellSurface(
    QtWaylandClient::QWaylandWindow *window)
{
    if (!isActive()) {
        return nullptr;
    }

    auto *surface = get_input_panel_surface(window->wlSurface());
    return new KWinInputPanelSurface(surface, window);
}

}
