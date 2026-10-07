// SPDX-License-Identifier: GPL-3.0-or-later

#include "qtsystemclipboard.h"

#include <QClipboard>
#include <QGuiApplication>

namespace Tastra
{

QtSystemClipboard::QtSystemClipboard(QObject *parent)
    : SystemClipboard(parent)
{
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) return;
    m_clipboard = QGuiApplication::clipboard();
    connect(m_clipboard, &QClipboard::dataChanged, this, &SystemClipboard::changed);
}

QString QtSystemClipboard::text() const { return m_clipboard ? m_clipboard->text() : QString(); }

void QtSystemClipboard::setText(const QString &text)
{
    if (m_clipboard) m_clipboard->setText(text);
}

void QtSystemClipboard::clear()
{
    if (m_clipboard) m_clipboard->clear();
}

}
