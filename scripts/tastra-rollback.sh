#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Restores the newest backed-up Tastra binary that is older than the one
# installed now and differs from it; run it again to go further back.
# tastra-update returns to the current version.
set -euo pipefail

BIN="$HOME/.local/bin/tastra"
BACKUPS="${XDG_DATA_HOME:-$HOME/.local/share}/tastra/backups"
checksum() { sha256sum "$1" | awk '{print $1}'; }

current="" current_time=""
if [[ -f "$BIN" ]]; then
    current="$(checksum "$BIN")"
    current_time="$(stat -c %Y "$BIN")"
fi
target=""
if [[ -d "$BACKUPS" ]]; then
    # v3-keyboard.before-*: backups made by the "V3 Keyboard" builds (up to 0.6.1).
    while read -r time candidate; do
        [[ -n "$candidate" ]] || continue
        # A restored backup keeps its time, so the next rollback goes on from it.
        [[ -z "$current_time" || "${time%.*}" -lt "$current_time" ]] || continue
        [[ "$(checksum "$candidate")" != "$current" ]] || continue
        target="$candidate"
        break
    done < <(find -L "$BACKUPS" -maxdepth 1 -type f \( -name 'tastra.before-*' -o -name 'v3-keyboard.before-*' \) \
                 -printf '%T@ %p\n' | sort -rn)
fi
[[ -n "$target" ]] || { echo "No older build in $BACKUPS to go back to."; exit 1; }

if [[ -n "$current" ]]; then
    cp -a "$BIN" "$BACKUPS/tastra.rolled-back.$(date +%Y%m%d-%H%M%S)"
fi
cp -a "$target" "$BIN.new"
chmod 755 "$BIN.new"
mv -f "$BIN.new" "$BIN"
echo "Restored: $target"
echo "Relaunch: kcmshell6 kcm_virtualkeyboard -> None -> Apply -> Tastra -> Apply"
echo "Back to the current version: tastra-update"
