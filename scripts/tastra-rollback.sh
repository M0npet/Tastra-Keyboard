#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Restores the newest backed-up Tastra binary that differs from the one
# installed now; run it again to go further back. tastra-update returns to
# the current version.
set -euo pipefail

BIN="$HOME/.local/bin/tastra"
BACKUPS="${XDG_DATA_HOME:-$HOME/.local/share}/tastra/backups"
checksum() { sha256sum "$1" | awk '{print $1}'; }

current=""
[[ -f "$BIN" ]] && current="$(checksum "$BIN")"
target=""
if [[ -d "$BACKUPS" ]]; then
    # v3-keyboard.before-*: backups made by the "V3 Keyboard" builds (up to 0.6.1).
    while IFS= read -r candidate; do
        [[ -n "$candidate" ]] || continue
        if [[ "$(checksum "$candidate")" != "$current" ]]; then
            target="$candidate"
            break
        fi
    done < <(find -L "$BACKUPS" -maxdepth 1 -type f \( -name 'tastra.before-*' -o -name 'v3-keyboard.before-*' \) \
                 -printf '%T@ %p\n' | sort -rn | cut -d' ' -f2-)
fi
[[ -n "$target" ]] || { echo "No older build in $BACKUPS to go back to."; exit 1; }

if [[ -n "$current" ]]; then
    cp -a "$BIN" "$BACKUPS/tastra.rolled-back.$(date +%Y%m%d-%H%M%S)"
fi
install -m755 "$target" "$BIN.new"
mv -f "$BIN.new" "$BIN"
echo "Restored: $target"
echo "Relaunch: kcmshell6 kcm_virtualkeyboard -> None -> Apply -> Tastra -> Apply"
echo "Back to the current version: tastra-update"
