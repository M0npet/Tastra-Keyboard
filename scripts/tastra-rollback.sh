#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Restores the Tastra binary that the deploy script backed up last.
set -euo pipefail

BIN="$HOME/.local/bin/tastra"
BACKUPS="${XDG_DATA_HOME:-$HOME/.local/share}/tastra/backups"

latest=""
if [[ -d "$BACKUPS" ]]; then
    # v3-keyboard.before-*: backups made by the "V3 Keyboard" builds (up to 0.6.1).
    latest="$(find -L "$BACKUPS" -maxdepth 1 -type f \( -name 'tastra.before-*' -o -name 'v3-keyboard.before-*' \) \
        -printf '%T@ %p\n' | sort -rn | head -n1 | cut -d' ' -f2-)"
fi
[[ -n "$latest" ]] || { echo "No backup found in $BACKUPS"; exit 1; }

if [[ -x "$BIN" ]]; then
    cp -a "$BIN" "$BACKUPS/tastra.rolled-back.$(date +%Y%m%d-%H%M%S)"
fi
install -m755 "$latest" "$BIN.new"
mv -f "$BIN.new" "$BIN"
echo "Restored: $latest"
echo "Relaunch: kcmshell6 kcm_virtualkeyboard -> None -> Apply -> Tastra -> Apply"
