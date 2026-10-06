#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Updates Tastra from GitHub: brings the source checkout up to date and runs
# its installer, which installs only when the build and all tests are green.
#
#   tastra-update            latest main
#   tastra-update --check    only report whether an update is available
set -euo pipefail

REPO="${TASTRA_REPO:-https://github.com/M0npet/Tastra-Keyboard.git}"
SRC="${TASTRA_SOURCE:-$HOME/.local/src/tastra}"

command -v git >/dev/null 2>&1 || { echo "STOP: git is missing (sudo pacman -S git)"; exit 1; }

if [[ ! -d "$SRC/.git" ]]; then
    [[ "${1:-}" == "--check" ]] && { echo "No source checkout in $SRC yet: tastra-update will clone it."; exit 0; }
    mkdir -p "$(dirname "$SRC")"
    git clone "$REPO" "$SRC"
else
    [[ -z "$(git -C "$SRC" status --porcelain --untracked-files=no)" ]] \
        || { echo "STOP: $SRC has local changes (inspect with: git -C '$SRC' status)"; exit 1; }
    git -C "$SRC" fetch --quiet origin
    local_head="$(git -C "$SRC" rev-parse HEAD)"
    remote_head="$(git -C "$SRC" rev-parse '@{upstream}')"
    if [[ "$local_head" == "$remote_head" ]]; then
        echo "Tastra is up to date ($(git -C "$SRC" log -1 --format='%h %s'))."
        [[ "${1:-}" == "--check" ]] && exit 0
        [[ -x "$HOME/.local/bin/tastra" ]] && exit 0   # installed and current: nothing to do
    else
        echo "Update available:"
        git -C "$SRC" log --format='  %h %s' "HEAD..@{upstream}"
        [[ "${1:-}" == "--check" ]] && exit 0
        git -C "$SRC" merge --ff-only --quiet '@{upstream}'
    fi
fi

# The installer of the new version does the rest.
exec "$SRC/scripts/tastra-install.sh"
