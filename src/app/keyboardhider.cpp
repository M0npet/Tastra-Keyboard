// SPDX-License-Identifier: GPL-3.0-or-later
#include "keyboardhider.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcHider, "tastra.panel.hide", QtWarningMsg)

namespace Tastra
{

void KWinKeyboardHider::hideKeyboard()
{
    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.KWin"), QStringLiteral("/VirtualKeyboard"),
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Set"));
    call << QStringLiteral("org.kde.kwin.VirtualKeyboard") << QStringLiteral("active")
         << QVariant::fromValue(QDBusVariant(false));
    const bool sent = QDBusConnection::sessionBus().send(call);   // asynchronous: never blocks typing
    qCInfo(lcHider) << "hide requested, sent" << sent;
}

}
