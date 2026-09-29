#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Installs the Hunspell dictionaries for the V3 Keyboard languages.
# Runs at install time only; the keyboard process itself never uses the network.
#
#   v3kbd-dictionaries            install whatever is missing
#   v3kbd-dictionaries --status   only report what is found
#
# Override the language list with V3KBD_LANGUAGES="en de uk ru".
set -euo pipefail

LANGS="${V3KBD_LANGUAGES:-en de uk ru}"
USER_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/v3-keyboard/dictionaries"
IFS=: read -r -a SYSTEM_DIRS <<< "${V3KBD_SYSTEM_DICT_DIRS:-/usr/share/hunspell:/usr/share/myspell/dicts}"

# Ukrainian is not packaged in the official Arch repositories. Use the
# LibreOffice upstream dictionary (MPL-1.1, based on brown-uk/dict_uk),
# pinned to one commit and verified by SHA-256.
UK_COMMIT="32b006a2c22a4ac7e8ed3f03346f7b3d85a970a4"
UK_BASE="https://raw.githubusercontent.com/LibreOffice/dictionaries/${UK_COMMIT}/uk_UA"
UK_AFF_SHA="2219dd15e9802adebc45722c60943b1472640260491af38dd3e43b07e75585e6"
UK_DIC_SHA="2e5a9e67be63bdb089b3459addb5d71113319d13768e277bcae20f3cc1ad5a93"

names_for() {
    case "$1" in
        en) echo "en_US en_GB" ;;
        de) echo "de_DE de_DE_frami de_AT de_CH" ;;
        uk) echo "uk_UA" ;;
        ru) echo "ru_RU" ;;
        *)  echo "" ;;
    esac
}

package_for() {
    case "$1" in
        en) echo "hunspell-en_us" ;;
        de) echo "hunspell-de" ;;
        ru) echo "hunspell-ru" ;;
        *)  echo "" ;;
    esac
}

find_dict() {
    local lang="$1" dir name
    for name in $(names_for "$lang"); do
        for dir in "$USER_DIR" "${SYSTEM_DIRS[@]}"; do
            if [[ -f "$dir/$name.dic" && -f "$dir/$name.aff" ]]; then
                echo "$dir/$name"
                return 0
            fi
        done
    done
    return 1
}

status() {
    local lang path rc=0
    for lang in $LANGS; do
        if path="$(find_dict "$lang")"; then
            printf '  %-3s OK       %s\n' "$lang" "$path"
        else
            printf '  %-3s MISSING  (autocorrect limited to built-in typo list)\n' "$lang"
            rc=1
        fi
    done
    return $rc
}

install_uk() {
    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN
    echo "Downloading pinned Ukrainian dictionary (LibreOffice ${UK_COMMIT:0:12}, MPL-1.1)..."
    curl -fsSL --proto '=https' --tlsv1.2 -o "$tmp/uk_UA.aff" "$UK_BASE/uk_UA.aff"
    curl -fsSL --proto '=https' --tlsv1.2 -o "$tmp/uk_UA.dic" "$UK_BASE/uk_UA.dic"
    echo "$UK_AFF_SHA  $tmp/uk_UA.aff" | sha256sum -c --quiet -
    echo "$UK_DIC_SHA  $tmp/uk_UA.dic" | sha256sum -c --quiet -
    install -d "$USER_DIR"
    install -m644 "$tmp/uk_UA.aff" "$tmp/uk_UA.dic" "$USER_DIR/"
    echo "Installed: $USER_DIR/uk_UA.{aff,dic}"
}

if [[ "${1:-}" == "--status" ]]; then
    echo "V3 Keyboard dictionaries:"
    status
    exit $?
fi

packages=()
need_uk=0
for lang in $LANGS; do
    find_dict "$lang" >/dev/null && continue
    pkg="$(package_for "$lang")"
    if [[ -n "$pkg" ]]; then
        packages+=("$pkg")
    elif [[ "$lang" == "uk" ]]; then
        need_uk=1
    else
        echo "WARN: no known dictionary source for language '$lang'"
    fi
done

if ((${#packages[@]})); then
    if command -v pacman >/dev/null 2>&1; then
        echo "Installing: ${packages[*]}"
        sudo pacman -S --needed "${packages[@]}"
    else
        echo "WARN: pacman not found; install these with your package manager: ${packages[*]}"
    fi
fi
if ((need_uk)); then
    install_uk
fi

echo "V3 Keyboard dictionaries:"
status || true
