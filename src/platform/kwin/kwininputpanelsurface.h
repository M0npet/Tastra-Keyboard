// SPDX-FileCopyrightText: 2017 Jan Arne Petersen
// SPDX-License-Identifier: LGPL-2.1-only

#pragma once

#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

#include <qwayland-input-method-unstable-v1.h>

namespace Tastra::KWin
{

class KWinInputPanelSurface final
    : public QtWaylandClient::QWaylandShellSurface
    , public QtWayland::zwp_input_panel_surface_v1
{
public:
    KWinInputPanelSurface(
        ::zwp_input_panel_surface_v1 *object,
        QtWaylandClient::QWaylandWindow *window);
    ~KWinInputPanelSurface() override;

    void applyConfigure() override;
};

}
