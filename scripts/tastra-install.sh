#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds this source checkout and installs Tastra into ~/.local, but only when
# the static check, a fresh Release build and the full test suite are all
# green. The previous binary is kept for tastra-rollback. An installation from
# the "V3 Keyboard" days (up to 0.6.1) is carried over: settings, words,
# dictionaries and the KWin keyboard choice.
#
#   scripts/tastra-install.sh             build, test, install
#   scripts/tastra-install.sh --no-build  install the existing build as is
#                                         (development only; skips the tests)
set -euo pipefail

SRC="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${TASTRA_BUILD_DIR:-$SRC/build-release}"
BIN_DIR="$HOME/.local/bin"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
STATE="${XDG_STATE_HOME:-$HOME/.local/state}"
DESKTOP_ID="io.github.m0npet.Tastra.desktop"
DESKTOP="$DATA/applications/$DESKTOP_ID"
LOG="$BUILD.log"

no_build=0
[[ "${1:-}" == "--no-build" ]] && no_build=1

stop() { echo "STOP: $*" >&2; exit 1; }
VERSION="$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' "$SRC/CMakeLists.txt" | head -n1)"
[[ -n "$VERSION" ]] || stop "cannot read the version from CMakeLists.txt"

echo "===== TASTRA $VERSION ====="
echo "Source: $SRC ($(git -C "$SRC" log -1 --format='%h %s' 2>/dev/null || echo 'not a git checkout'))"

for tool in cmake ninja pkg-config python3; do
    command -v "$tool" >/dev/null 2>&1 || stop "missing tool: $tool (sudo pacman -S cmake ninja pkgconf python)"
done
pkg-config --exists hunspell || stop "libhunspell not found (sudo pacman -S hunspell)"

echo
echo "===== FILES FROM V3 KEYBOARD ====="
# The keyboard moves these itself on its first start as well; doing it here
# first means the dictionary step below finds what is already downloaded.
moved=0
for base in "$CONFIG" "$DATA" "$STATE"; do
    if [[ -d "$base/v3-keyboard" && ! -L "$base/v3-keyboard" && ! -e "$base/tastra" ]]; then
        mv "$base/v3-keyboard" "$base/tastra"
        ln -s "$base/tastra" "$base/v3-keyboard"   # a rolled-back 0.6.1 still finds everything
        echo "moved $base/v3-keyboard -> $base/tastra"
        moved=1
    fi
done
if [[ -f "$CONFIG/V3Keyboard/V3 Keyboard.conf" && ! -e "$CONFIG/tastra/tastra.conf" ]]; then
    mkdir -p "$CONFIG/tastra"
    cp -a "$CONFIG/V3Keyboard/V3 Keyboard.conf" "$CONFIG/tastra/tastra.conf"
    echo "copied settings -> $CONFIG/tastra/tastra.conf"
    moved=1
fi
((moved)) || echo "nothing to move"

if ((no_build)); then
    [[ -x "$BUILD/tastra" ]] || stop "--no-build: no binary at $BUILD/tastra"
    echo
    echo "WARNING: --no-build installs $BUILD/tastra without running the tests."
else
    echo
    echo "===== DICTIONARIES ====="
    bash "$SRC/scripts/tastra-dictionaries.sh" || echo "WARN: dictionary provisioning incomplete"

    echo
    echo "===== OFFLINE VOICE INPUT (optional) ====="
    if bash "$SRC/scripts/tastra-voice-setup.sh" --status >/dev/null 2>&1; then
        echo "Already set up: the build will include the mic key."
    else
        choice="${TASTRA_VOICE_SETUP:-}"
        if [[ -z "$choice" && -t 0 ]]; then
            read -r -p "Enable offline voice input? Installs whisper-cpp + qt6-multimedia and a 60 MB model [y/N] " choice
        fi
        if [[ "$choice" =~ ^[Yy] ]]; then
            bash "$SRC/scripts/tastra-voice-setup.sh" || echo "WARN: voice setup incomplete; building without the mic key"
        else
            echo "Skipped (run tastra-voice-setup later, then tastra-update)."
        fi
    fi

    echo
    echo "===== STATIC VERIFY ====="
    python3 "$SRC/scripts/verify-static.py"

    echo
    echo "===== FRESH BUILD + FULL TEST SUITE ====="
    rm -rf "$BUILD"
    (
        set -o pipefail
        cmake -S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release 2>&1 | tee "$LOG"
        cmake --build "$BUILD" --parallel "$(nproc)" 2>&1 | tee -a "$LOG"
        QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD" --output-on-failure 2>&1 | tee -a "$LOG"
    ) || stop "build or tests failed; nothing was installed (log: $LOG)"
fi

echo
echo "===== INSTALL ====="
mkdir -p "$BIN_DIR" "$DATA/tastra/backups" "$DATA/applications"
stamp="$(date +%Y%m%d-%H%M%S)"
# The binary that runs now: Tastra's, or the old name's (a real file, not our link).
if [[ -f "$BIN_DIR/tastra" && ! -L "$BIN_DIR/tastra" ]]; then
    cp -a "$BIN_DIR/tastra" "$DATA/tastra/backups/tastra.before-$VERSION.$stamp"
elif [[ -f "$BIN_DIR/v3-keyboard" && ! -L "$BIN_DIR/v3-keyboard" ]]; then
    cp -a "$BIN_DIR/v3-keyboard" "$DATA/tastra/backups/v3-keyboard.before-$VERSION.$stamp"
fi
install -m755 "$BUILD/tastra" "$BIN_DIR/tastra.new"
mv -f "$BIN_DIR/tastra.new" "$BIN_DIR/tastra"
for helper in dictionaries voice-setup rollback uninstall update; do
    install -m755 "$SRC/scripts/tastra-$helper.sh" "$BIN_DIR/tastra-$helper"
done
# tastra-install needs its source tree; the installed copy only forwards.
cat > "$BIN_DIR/tastra-install" <<EOF
#!/usr/bin/env bash
exec "$SRC/scripts/tastra-install.sh" "\$@"
EOF
chmod 755 "$BIN_DIR/tastra-install"

sed "s|^Exec=.*|Exec=$BIN_DIR/tastra|" "$SRC/data/$DESKTOP_ID" > "$DESKTOP.new"
mv -f "$DESKTOP.new" "$DESKTOP"

DOC_DIR="$DATA/doc/tastra"
mkdir -p "$DOC_DIR"
install -m644 "$SRC/COPYING" "$SRC/README.md" "$SRC/CHANGELOG.md" "$DOC_DIR/"
for attribution in "$SRC"/data/*/ATTRIBUTION.md; do
    install -m644 "$attribution" "$DOC_DIR/ATTRIBUTION-$(basename "$(dirname "$attribution")").md"
done

# The old name: KWin may restart the keyboard with the old command until the
# next login, so that command now starts Tastra. Old helpers and docs go.
ln -sfn "$BIN_DIR/tastra" "$BIN_DIR/v3-keyboard"
rm -f "$BIN_DIR/v3kbd-dictionaries" "$BIN_DIR/v3kbd-voice-setup" "$BIN_DIR/v3kbd-rollback" \
      "$BIN_DIR/v3kbd-uninstall" "$DATA/applications/org.v3keyboard.desktop"
rm -rf "$DATA/doc/v3-keyboard"

# KWin remembers the chosen keyboard as the path of its .desktop file.
kwin_note=""
if command -v kreadconfig6 >/dev/null 2>&1 && command -v kwriteconfig6 >/dev/null 2>&1; then
    current="$(kreadconfig6 --file kwinrc --group Wayland --key InputMethod 2>/dev/null || true)"
    if [[ "$current" == *v3keyboard* ]]; then
        kwriteconfig6 --file kwinrc --group Wayland --key InputMethod "$DESKTOP"
        kwin_note="KWin keyboard choice moved from $current to $DESKTOP"
    fi
fi
command -v kbuildsycoca6 >/dev/null 2>&1 && kbuildsycoca6 >/dev/null 2>&1 || true

[[ "$(sha256sum "$BUILD/tastra" | awk '{print $1}')" == "$(sha256sum "$BIN_DIR/tastra" | awk '{print $1}')" ]] \
    || stop "installed binary checksum mismatch"

echo
echo "===== TASTRA $VERSION INSTALLED ====="
echo "binary:     $BIN_DIR/tastra ($(sha256sum "$BIN_DIR/tastra" | awk '{print $1}'))"
[[ -n "$kwin_note" ]] && echo "$kwin_note"
((no_build)) || echo "build log:  $LOG"
echo "Test plan:  $SRC/docs/TEST-PLAN.md"
echo "Relaunch:   kcmshell6 kcm_virtualkeyboard -> None -> Apply -> Tastra -> Apply"
echo "Update:     tastra-update     Roll back: tastra-rollback"
