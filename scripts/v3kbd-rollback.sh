#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Restores the V3 Keyboard binary that the deploy script backed up last.
set -euo pipefail

BIN="$HOME/.local/bin/v3-keyboard"
BACKUPS="$HOME/.local/share/v3-keyboard/backups"

latest=""
if [[ -d "$BACKUPS" ]]; then
    latest="$(find "$BACKUPS" -maxdepth 1 -type f -name 'v3-keyboard.before-*' -printf '%T@ %p\n' | sort -rn | head -n1 | cut -d' ' -f2-)"
fi
[[ -n "$latest" ]] || { echo "No backup found in $BACKUPS"; exit 1; }

if [[ -x "$BIN" ]]; then
    cp -a "$BIN" "$BACKUPS/v3-keyboard.rolled-back.$(date +%Y%m%d-%H%M%S)"
fi
install -m755 "$latest" "$BIN.new"
mv -f "$BIN.new" "$BIN"
echo "Restored: $latest"
echo "Relaunch: kcmshell6 kcm_virtualkeyboard -> None -> Apply -> V3 Keyboard -> Apply"
