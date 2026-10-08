// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <functional>

class QObject;

namespace Tastra {

// KWin stops its input method with SIGTERM: when another virtual keyboard is
// chosen and when the session ends (InputMethod::stopInputMethod, which waits
// up to 30 s before SIGKILL). After this call, SIGTERM and SIGINT first run
// `save` on the event loop of `parent`'s thread and then end the process as
// the signal would have (no destructors run, as before). Returns false if
// the handler could not be installed.
bool saveBeforeTermination(QObject *parent, std::function<void()> save);

}
