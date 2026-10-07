// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

struct wl_display;

namespace Tastra::KWin
{

// Qt's own Wayland display (null unless the application runs on Wayland).
wl_display *applicationWaylandDisplay();

}
