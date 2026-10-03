#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Removes V3 Keyboard from ~/.local. With --purge also deletes settings,
# learned words, downloaded dictionaries/voice model, traces and backups.
# Switch the virtual keyboard to "None" first:
#   kcmshell6 kcm_virtualkeyboard -> None -> Apply
set -euo pipefail

purge=0
[[ "${1:-}" == "--purge" ]] && purge=1

DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}"

rm -f "$HOME/.local/bin/v3-keyboard" \
      "$HOME/.local/bin/v3kbd-dictionaries" \
      "$HOME/.local/bin/v3kbd-voice-setup" \
      "$HOME/.local/bin/v3kbd-rollback" \
      "$HOME/.local/bin/v3kbd-uninstall" \
      "$DATA/applications/org.v3keyboard.desktop"
rm -rf "$DATA/doc/v3-keyboard"
command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 >/dev/null 2>&1 || true
echo "Removed the V3 Keyboard program files."

if ((purge)); then
    rm -rf "$CONFIG/V3Keyboard" "$CONFIG/v3-keyboard" "$DATA/v3-keyboard" "$STATE/v3-keyboard"
    echo "Removed settings, learned words, dictionaries, voice model, traces and backups."
else
    echo "Kept settings and data (use --purge to delete them)."
fi
echo "Source worktrees in ~/.local/src/v3-keyboard-worktrees were left untouched."
