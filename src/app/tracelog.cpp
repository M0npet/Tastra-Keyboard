// SPDX-License-Identifier: GPL-3.0-or-later

#include "tracelog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>

namespace V3Keyboard
{
namespace
{

QFile *g_trace = nullptr;
QMutex g_traceMutex;
QtMessageHandler g_previousHandler = nullptr;

void traceHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (context.category && QByteArray(context.category).startsWith("v3keyboard")) {
        QMutexLocker locker(&g_traceMutex);
        if (g_trace) {
            g_trace->write(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")).toUtf8()
                           + ' ' + context.category + ": " + message.toUtf8() + '\n');
            g_trace->flush();
        }
    }
    if (g_previousHandler) g_previousHandler(type, context, message);
}

}

QString defaultStateDir()
{
    QString base = qEnvironmentVariable("XDG_STATE_HOME");
    if (base.isEmpty()) base = QDir::homePath() + QStringLiteral("/.local/state");
    return base + QStringLiteral("/v3-keyboard");
}

bool enableTraceIfRequested(const QString &stateDir)
{
    if (!QFile::exists(stateDir + QStringLiteral("/trace.enable"))) return false;
    auto *file = new QFile(stateDir + QStringLiteral("/trace.log"));
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        delete file;
        return false;
    }
    {
        QMutexLocker locker(&g_traceMutex);
        g_trace = file;
    }
    QLoggingCategory::setFilterRules(QStringLiteral("v3keyboard.*=true"));
    g_previousHandler = qInstallMessageHandler(traceHandler);
    return true;
}

}
