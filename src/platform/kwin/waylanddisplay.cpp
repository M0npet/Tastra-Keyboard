// SPDX-License-Identifier: GPL-3.0-or-later

#include "waylanddisplay.h"

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

namespace Tastra::KWin
{

wl_display *applicationWaylandDisplay()
{
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) return nullptr;
    if (!QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) return nullptr;
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    if (!native) return nullptr;
    return static_cast<wl_display *>(native->nativeResourceForIntegration(QByteArrayLiteral("wl_display")));
}

}
