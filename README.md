# V3 Keyboard

A privacy-first, highly customizable virtual keyboard for Wayland.

## Goals

- First-class KDE Plasma / KWin integration
- Portable Wayland compositor backends
- Gboard-class customization
- Offline prediction and personalization
- No Google services
- No telemetry
- No mandatory network access

## Architecture

Plasma-first, Wayland-native, compositor-agnostic core.

First development milestone:

touch key -> keyboard model -> KWin input-method-v1 -> real Wayland text field

## Alpha 0.1 live-test surface

The Alpha 0.1 integration adds a compact tools row (Clipboard, Emoji, Text editing,
Settings), persistent language/appearance preferences, language-specific long-press
alternates, and editing navigation while keeping language switching on Globe/Space.
See `docs/alpha-0.1.md` for the exact scope.

## Current state (Beta 0.2.7)

- Words are composed in the client's preedit (underlined while typing); Space,
  punctuation, suggestions and glide commit them in one step. This is what
  makes autocorrect reliable in Firefox/GTK. Can be switched off in Settings
  ("Underline word while typing").
- Autocorrect uses libhunspell (affix-aware) and is conservative; Backspace
  right after a correction undoes it and remembers the word.
- Two-thumb typing: each key tracks its own touch point.
- Dictionaries: `v3kbd-dictionaries` (installed by the deploy script)
  provisions EN/DE/RU via pacman and UK from a pinned, checksummed upstream
  file. The keyboard itself never uses the network.
- Diagnostics: `touch ~/.local/state/v3-keyboard/trace.enable`, relaunch,
  read `~/.local/state/v3-keyboard/trace.log` (sizes/decisions only).

See docs/beta-0.2.7-completion.md and docs/HANDOFF.md.

## 0.3.0 additions

- Frequency-ranked suggestions and autocorrect prior (bundled word lists,
  filtered through Hunspell; attribution in data/frequency/ATTRIBUTION.md).
- Emoji search with CLDR keywords in EN/DE/RU/UK (data/emoji/ATTRIBUTION.md).
- Compact layout (left/right) in Settings.

## 0.4.0 addition: offline voice input (opt-in)

`v3kbd-voice-setup` installs whisper.cpp + Qt Multimedia and a 60 MB model;
the keyboard then shows a 🎤 key. Everything runs locally. See docs/release-0.4.0.md.

## 0.5.0: Gboard behaviours

Long-press symbols with key hints and a slide picker, period punctuation,
number row, emoji suggestions, offensive-word filter (on by default),
long-press a suggestion to remove it. See docs/release-0.5.0.md.
