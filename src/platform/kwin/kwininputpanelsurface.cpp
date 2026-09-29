// SPDX-FileCopyrightText: 2017 Jan Arne Petersen
// SPDX-License-Identifier: LGPL-2.1-only

#include "kwininputpanelsurface.h"

#include <QtWaylandClient/private/qwaylandscreen_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace V3Keyboard::KWin
{

KWinInputPanelSurface::KWinInputPanelSurface(
    ::zwp_input_panel_surface_v1 *object,
    QtWaylandClient::QWaylandWindow *window)
    : QWaylandShellSurface(window)
    , QtWayland::zwp_input_panel_surface_v1(object)
{
    window->applyConfigureWhenPossible();
}

KWinInputPanelSurface::~KWinInputPanelSurface()
{
    zwp_input_panel_surface_v1_destroy(object());
}

void KWinInputPanelSurface::applyConfigure()
{
    auto *screen = window()->waylandScreen();
    if (!screen) {
        return;
    }

    set_toplevel(screen->output(), position_center_bottom);
    window()->display()->handleWindowActivated(window());
}

}
