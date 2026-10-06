// SPDX-License-Identifier: GPL-3.0-or-later

#include "legacymigration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace Tastra
{
namespace
{

QString xdgBase(const char *variable, const QString &fallback)
{
    const QString value = qEnvironmentVariable(variable);
    // The XDG spec ignores relative paths.
    return !value.isEmpty() && QDir::isAbsolutePath(value) ? value : QDir::homePath() + fallback;
}

// Moves the entries of `from` that `to` lacks; recurses into folders both
// have. Returns true when `from` ended up empty.
bool mergeInto(const QString &from, const QString &to, QStringList &log)
{
    QDir().mkpath(to);
    bool emptied = true;
    const QFileInfoList entries =
        QDir(from).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        const QString target = to + QLatin1Char('/') + entry.fileName();
        const QFileInfo existing(target);
        if (!existing.exists() && !existing.isSymLink()) {
            if (QFile::rename(entry.filePath(), target)) {
                log << QStringLiteral("moved %1 -> %2").arg(entry.filePath(), target);
                continue;
            }
            emptied = false;
            continue;
        }
        if (entry.isDir() && !entry.isSymLink() && existing.isDir()) {
            if (mergeInto(entry.filePath(), target, log)) {
                QDir().rmdir(entry.filePath());
                continue;
            }
        }
        // Tastra's own file wins; the old one stays where it was.
        emptied = false;
    }
    return emptied;
}

void migrateFolder(const QString &oldPath, const QString &newPath, QStringList &log)
{
    const QFileInfo old(oldPath);
    if (old.isSymLink() || !old.isDir()) return; // already migrated, or nothing there
    if (!QFileInfo::exists(newPath)) {
        QDir().mkpath(QFileInfo(newPath).absolutePath());
        if (QDir().rename(oldPath, newPath)) {
            log << QStringLiteral("moved %1 -> %2").arg(oldPath, newPath);
        } else if (!mergeInto(oldPath, newPath, log)) {
            return; // e.g. another file system and a file failed to move: keep the rest
        } else {
            QDir().rmdir(oldPath);
        }
    } else if (mergeInto(oldPath, newPath, log)) {
        QDir().rmdir(oldPath);
    } else {
        return;
    }
    if (QFile::link(newPath, oldPath)) log << QStringLiteral("linked %1 -> %2").arg(oldPath, newPath);
}

}

MigrationRoots defaultMigrationRoots()
{
    return {xdgBase("XDG_CONFIG_HOME", QStringLiteral("/.config")),
            xdgBase("XDG_DATA_HOME", QStringLiteral("/.local/share")),
            xdgBase("XDG_STATE_HOME", QStringLiteral("/.local/state"))};
}

QStringList migrateLegacyPaths(const MigrationRoots &roots)
{
    QStringList log;
    const QString oldName = QStringLiteral("/v3-keyboard");
    const QString newName = QStringLiteral("/tastra");
    for (const QString &base : {roots.config, roots.data, roots.state}) {
        if (base.isEmpty()) continue;
        migrateFolder(base + oldName, base + newName, log);
    }

    // Settings were kept by QSettings under the old organisation/application
    // names. Copied, not moved: a rolled-back binary still reads the old file.
    if (!roots.config.isEmpty()) {
        const QString oldSettings = roots.config + QStringLiteral("/V3Keyboard/V3 Keyboard.conf");
        const QString newSettings = roots.config + QStringLiteral("/tastra/tastra.conf");
        if (QFileInfo::exists(oldSettings) && !QFileInfo::exists(newSettings)) {
            QDir().mkpath(QFileInfo(newSettings).absolutePath());
            if (QFile::copy(oldSettings, newSettings)) {
                log << QStringLiteral("copied %1 -> %2").arg(oldSettings, newSettings);
            }
        }
    }
    return log;
}

}
