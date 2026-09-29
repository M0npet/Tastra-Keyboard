#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Opt-in offline voice input for V3 Keyboard.
#   v3kbd-voice-setup            install packages + model
#   v3kbd-voice-setup --status   report what is present
#   v3kbd-voice-setup --remove   delete the model (packages are left alone)
#
# Recognition runs locally with whisper.cpp; nothing is sent anywhere. The
# keyboard must be rebuilt (deploy script) after the packages are installed
# for the mic key to appear.
set -euo pipefail

MODEL_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/v3-keyboard/voice"
MODEL="ggml-base-q5_1.bin"
# Official whisper.cpp model repository (MIT); multilingual base, 5-bit.
MODEL_URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/$MODEL"
MODEL_SHA="422f1ae452ade6f30a004d7e5c6a43195e4433bc370bf23fac9cc591f01a8898"
MODEL_SIZE=59707625
PACKAGE="${V3KBD_VOICE_PACKAGE:-whisper-cpp}"   # or whisper-cpp-vulkan for the iGPU

have_pkg() { pacman -Qq "$1" >/dev/null 2>&1; }

status() {
    local rc=0
    if have_pkg whisper-cpp || have_pkg whisper-cpp-vulkan || have_pkg whisper-cpp-rocm; then
        echo "  whisper.cpp   OK"
    else
        echo "  whisper.cpp   MISSING"; rc=1
    fi
    if have_pkg qt6-multimedia; then echo "  qt6-multimedia OK"; else echo "  qt6-multimedia MISSING"; rc=1; fi
    if [[ -f "$MODEL_DIR/$MODEL" ]]; then echo "  model         OK ($MODEL_DIR/$MODEL)"; else echo "  model         MISSING"; rc=1; fi
    return $rc
}

case "${1:-install}" in
    --status)
        echo "V3 Keyboard voice input:"
        status
        exit $?
        ;;
    --remove)
        rm -f "$MODEL_DIR/$MODEL"
        echo "Removed $MODEL_DIR/$MODEL"
        exit 0
        ;;
    install) ;;
    *) echo "usage: v3kbd-voice-setup [--status|--remove]"; exit 2 ;;
esac

command -v pacman >/dev/null 2>&1 || { echo "pacman not found: install whisper.cpp and Qt 6 Multimedia manually"; exit 1; }

packages=()
if ! (have_pkg whisper-cpp || have_pkg whisper-cpp-vulkan || have_pkg whisper-cpp-rocm); then
    packages+=("$PACKAGE")
fi
have_pkg qt6-multimedia || packages+=(qt6-multimedia)
if ((${#packages[@]})); then
    echo "Installing: ${packages[*]}"
    sudo pacman -S --needed "${packages[@]}"
fi

if [[ ! -f "$MODEL_DIR/$MODEL" ]]; then
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    echo "Downloading $MODEL (60 MB, verified by SHA-256)..."
    curl -fL --proto '=https' --tlsv1.2 -o "$tmp/$MODEL" "$MODEL_URL"
    [[ "$(stat -c %s "$tmp/$MODEL")" == "$MODEL_SIZE" ]] || { echo "STOP: unexpected model size"; exit 1; }
    echo "$MODEL_SHA  $tmp/$MODEL" | sha256sum -c --quiet -
    install -d "$MODEL_DIR"
    install -m644 "$tmp/$MODEL" "$MODEL_DIR/$MODEL"
fi

echo "V3 Keyboard voice input:"
status || true
