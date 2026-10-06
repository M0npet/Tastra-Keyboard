// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace Tastra
{

// XDG base folders ($XDG_CONFIG_HOME, $XDG_DATA_HOME, $XDG_STATE_HOME).
struct MigrationRoots
{
    QString config;
    QString data;
    QString state;
};

MigrationRoots defaultMigrationRoots();

// The keyboard was called "V3 Keyboard" up to 0.6.1. Moves its files once:
//   <config>/v3-keyboard/            -> <config>/tastra/  (dictionary.txt, shortcuts.txt)
//   <config>/V3Keyboard/V3 Keyboard.conf -> <config>/tastra/tastra.conf (settings, copied)
//   <data>/v3-keyboard/              -> <data>/tastra/    (dictionaries, voice model, backups)
//   <state>/v3-keyboard/             -> <state>/tastra/   (trace switch)
// Never overwrites a file Tastra already has; a clashing old file stays where
// it was. A fully moved old folder becomes a link to the new one, so a
// rolled-back 0.6.1 binary keeps working on the same files. Returns one line
// per change (empty when there was nothing to do).
QStringList migrateLegacyPaths(const MigrationRoots &roots);

}
