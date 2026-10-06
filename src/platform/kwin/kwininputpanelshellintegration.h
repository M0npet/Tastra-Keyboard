// SPDX-FileCopyrightText: 2017 Jan Arne Petersen
// SPDX-License-Identifier: LGPL-2.1-only

#pragma once

#include <QtWaylandClient/private/qwaylandshellintegration_p.h>

#include <qwayland-input-method-unstable-v1.h>

namespace Tastra::KWin
{

class KWinInputPanelShellIntegration final
    : public QtWaylandClient::QWaylandShellIntegrationTemplate<KWinInputPanelShellIntegration>
    , public QtWayland::zwp_input_panel_v1
{
public:
    KWinInputPanelShellIntegration();

    QtWaylandClient::QWaylandShellSurface *createShellSurface(
        QtWaylandClient::QWaylandWindow *window) override;
};

}
