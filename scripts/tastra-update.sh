#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Updates Tastra from GitHub: brings the source checkout up to date and runs
# its installer, which installs only when the build and all tests are green.
# It also rebuilds when the source is current but the installed build is not
# what it would build now: Qt was updated (the keyboard uses Qt's private
# Wayland classes, which change between Qt versions), voice input or key
# sounds were set up, or the last install failed or was rolled back.
#
#   tastra-update            latest main
#   tastra-update --check    only report whether an update is available
#   tastra-update --force    rebuild and reinstall even if nothing changed
set -euo pipefail

DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
STAMP="$DATA/tastra/installed"
REPO="${TASTRA_REPO:-https://github.com/M0npet/Tastra-Keyboard.git}"
stamp_value() { [[ -f "$STAMP" ]] && sed -n "s/^$1=//p" "$STAMP" | head -n1 || true; }
# The checkout the installed build came from, unless told otherwise.
SRC="${TASTRA_SOURCE:-$(stamp_value source)}"
SRC="${SRC:-$HOME/.local/src/tastra}"

mode="${1:-}"
case "$mode" in
    ""|--check|--force) ;;
    *) echo "usage: tastra-update [--check|--force]"; exit 2 ;;
esac

command -v git >/dev/null 2>&1 || { echo "STOP: git is missing (sudo pacman -S git)"; exit 1; }

# Why the installed build differs from what this checkout builds now
# (empty: it does not).
rebuild_reason() {
    local bin="$HOME/.local/bin/tastra"
    [[ -x "$bin" ]] || { echo "Tastra is not installed"; return; }
    [[ -f "$STAMP" ]] || { echo "no record of the installed build"; return; }
    [[ "$(stamp_value commit)" == "$(git -C "$SRC" rev-parse HEAD)" ]] \
        || { echo "the installed build is not this version (a failed update or a rollback)"; return; }
    local qt
    qt="$(pkg-config --modversion Qt6Core 2>/dev/null || echo unknown)"
    [[ "$(stamp_value qt)" == "$qt" ]] || { echo "Qt changed ($(stamp_value qt) -> $qt)"; return; }
    local voice
    voice="$(bash "$SRC/scripts/tastra-voice-setup.sh" --status >/dev/null 2>&1 && echo yes || echo no)"
    [[ "$(stamp_value voice)" == "$voice" ]] || { echo "voice input was set up or removed"; return; }
    local sound
    sound="$(pkg-config --exists Qt6Multimedia 2>/dev/null && echo yes || echo no)"
    [[ "$(stamp_value sound)" == "$sound" ]] || { echo "Qt Multimedia (key sounds) was installed or removed"; return; }
    [[ "$(sha256sum "$bin" | awk '{print $1}')" == "$(stamp_value binary)" ]] \
        || { echo "the installed binary is not the one built (a rollback)"; return; }
}

if [[ ! -d "$SRC/.git" ]]; then
    [[ "$mode" == "--check" ]] && { echo "No source checkout in $SRC yet: tastra-update will clone it."; exit 0; }
    mkdir -p "$(dirname "$SRC")"
    git clone "$REPO" "$SRC"
else
    [[ -z "$(git -C "$SRC" status --porcelain --untracked-files=no)" ]] \
        || { echo "STOP: $SRC has local changes (inspect with: git -C '$SRC' status)"; exit 1; }
    git -C "$SRC" fetch --quiet origin
    local_head="$(git -C "$SRC" rev-parse HEAD)"
    remote_head="$(git -C "$SRC" rev-parse '@{upstream}')"
    if [[ "$local_head" == "$remote_head" ]]; then
        echo "Tastra's source is up to date ($(git -C "$SRC" log -1 --format='%h %s'))."
        reason="$(rebuild_reason)"
        if [[ "$mode" == "--check" ]]; then
            [[ -n "$reason" ]] && echo "A rebuild is due: $reason."
            exit 0
        fi
        if [[ "$mode" != "--force" ]]; then
            [[ -z "$reason" ]] && { echo "The installed build matches it: nothing to do (--force rebuilds anyway)."; exit 0; }
            echo "Rebuilding: $reason."
        fi
    else
        echo "Update available:"
        git -C "$SRC" log --format='  %h %s' "HEAD..@{upstream}"
        [[ "$mode" == "--check" ]] && exit 0
        git -C "$SRC" merge --ff-only --quiet '@{upstream}'
    fi
fi

# The installer of the new version does the rest.
exec "$SRC/scripts/tastra-install.sh"
