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
