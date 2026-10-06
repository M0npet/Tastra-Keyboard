// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace Tastra
{

// Privacy-safe diagnostics: if <stateDir>/trace.enable exists, all
// tastra.* logging categories are enabled and written (with timestamps)
// to <stateDir>/trace.log. The categories log sizes and decisions only, never
// typed text. Returns true when tracing was enabled.
bool enableTraceIfRequested(const QString &stateDir);

// $XDG_STATE_HOME/tastra, defaulting to ~/.local/state/tastra.
QString defaultStateDir();

}
