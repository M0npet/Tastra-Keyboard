# Tastra

A Gboard-style on-screen keyboard for KDE Plasma 6 on Wayland (KWin), made for
touch tablets such as the Minisforum V3. Native C++ / Qt 6 / QML (no Java,
Electron or browser engine), fully offline, light on memory. English, German,
Ukrainian and Russian. GPL-3.0-or-later (see `COPYING`).

Status: **0.7.0, in development**. The typing core is done and tested; the
remaining work is checking it on the device (`docs/TEST-PLAN.md`). The keyboard
was called "V3 Keyboard" up to 0.6.1; its settings and words move over
automatically.

## Install

On Arch Linux:

    sudo pacman -S --needed git base-devel cmake ninja pkgconf python hunspell \
        qt6-base qt6-declarative qt6-wayland qt6-svg wayland wayland-protocols libxkbcommon
    git clone https://github.com/M0npet/Tastra-Keyboard.git ~/.local/src/tastra
    ~/.local/src/tastra/scripts/tastra-install.sh
    kcmshell6 kcm_virtualkeyboard      # None -> Apply -> Tastra -> Apply

The installer runs a static check, a fresh Release build and the full test
suite, and installs into `~/.local` only when everything is green. The
previous binary is kept for rolling back.

- Update to the latest version: `tastra-update` (or `tastra-update --check`).
- Roll back to the previous build: `tastra-rollback`.
- Dictionaries: `tastra-dictionaries [--status]` (EN/DE/RU from pacman, UK from a
  pinned, checksummed upstream file).
- Offline voice input (optional): `tastra-voice-setup`, then `tastra-update`.
- Uninstall: `tastra-uninstall [--purge]`.

## Typing

- Words are composed underlined and committed by Space, punctuation, a
  suggestion or glide, so autocorrect works reliably even in Firefox.
- Autocorrect: Hunspell + word frequency + where on the key you touched, also
  for several slipped keys in one word, and two words typed without the space
  ("ofthe" -> "of the"). It is conservative, and Backspace right after a
  correction undoes it and remembers your word. Tap back into a corrected word
  to get what you typed back.
- Suggestion strip: your word, the **correction** (what Space inserts) and the
  next best; completions; next-word predictions learned from your own typing;
  emoji suggestions; long-press a suggestion to forget it. Offensive words are
  never suggested.
- Several languages at once: with English and German (or Ukrainian and
  Russian) enabled, words of the other one are left as typed, so `danke` is
  not "corrected" while English is active.
- Typed in the wrong layout? `ghbdtn` offers `привет`; picking it switches the
  language.
- Tap into the middle of a word for suggestions for the whole word.
- Capital letters follow Android's keyboard rules: sentence starts, but not
  after abbreviations such as "e.g."; quotes are handled, and German letters
  and dates too (no capital on the line after "Liebe Anna," or after "3.").
  Double Space types ". ", and a space is added after punctuation.
- Glide typing matched on the shape of the finger's path, with the other
  readings offered in the strip afterwards, one Backspace to erase a glided
  word, Shift or Caps Lock for capitals; a trail (can be turned off);
  two-thumb typing,
  long-press for accents and symbols (ß, ё, ґ, digits) with an adjustable delay
  (200–700 ms), two symbol pages with your currency, number row, key popups,
  optional key click.
- Gestures: drag Space to move the cursor, drag Backspace to delete, hold
  Backspace to repeat, slide from `?123` for one symbol or from Shift for one
  capital.
- Field-aware: number pad in number and phone fields, `@` and `.com` in e-mail
  and URL fields, no autocorrect or learning in passwords, URLs and e-mail.
- Layouts: full, split for thumb typing, and one-handed left/right. Choose
  which languages the globe key cycles through. A button hides the keyboard.
- Personal dictionary: tap an unknown word (shown in quotes), then
  "+ Add to dictionary", or Settings → "+ Add word" with an optional shortcut.
  Manage both in Settings, or add many words at once in
  `~/.config/tastra/dictionary.txt`. Words you type 3 times become yours; a
  single typo is never learned.
- Text shortcuts from Settings or `~/.config/tastra/shortcuts.txt` (`omw = on my way`).
- Emoji panel with search in all four languages (typed on the keyboard itself),
  a `:-)` tab of text faces, recent emoji, skin tones on long-press, and an
  optional emoji row.
- Clipboard: a chip offers what you just copied; history keeps items for an
  hour; long-press to pin an item for good.
- Text-editing panel with arrows (including up/down), Home/End and paste.
- Settings grouped like Gboard; System/Light/Dark/AMOLED themes. The interface
  is in English, German, Russian and Ukrainian (follows the system language).

## Platform limits (KDE Plasma / KWin)

- Select, Select all, Copy and Cut in the text-editing panel are not possible:
  KWin replaces the modifiers of keys sent by an input method, so Ctrl+C or
  Shift+Arrow never reach the application. Paste works (text is inserted).
- KWin decides where the panel goes, so there is no floating keyboard.

## Memory and speed

One language is in memory at a time, and its dictionary loads in the
background. The panels (settings, emoji, clipboard) are built when first
opened, so the keyboard itself starts in under 0.2 s. Resident memory with the dictionary loaded (`tools/bench_memory`):
English and German about 25 MB, Russian about 36 MB, Ukrainian about 62 MB
(Hunspell's Ukrainian dictionary alone is about 39 MB). A keystroke costs
about one million instructions (`tools/bench_keystroke`).

## Privacy

Nothing leaves the device. Settings and learned words are kept in
`~/.config/tastra`. Dictation audio stays in memory only (30 s at most) and is
erased after recognition. The optional trace
(`touch ~/.local/state/tastra/trace.enable` before starting the keyboard)
records sizes and decisions only, never text.

## Third-party data

FrequencyWords and wordfreq (CC BY-SA 4.0), Unicode CLDR annotations (Unicode License V3),
LDNOOBW word lists (CC BY 4.0), Hunspell dictionaries (system packages),
LibreOffice uk_UA dictionary (MPL-1.1), optional whisper.cpp + ggml model
(MIT). Details are in `data/*/ATTRIBUTION.md`, installed to
`~/.local/share/doc/tastra`.

## Development

    cmake -S . -B build -G Ninja && cmake --build build
    QT_QPA_PLATFORM=offscreen ctest --test-dir build
    python3 scripts/verify-static.py
    cmake -S . -B build -DTASTRA_BENCHMARKS=ON    # tools/bench_keystroke, tools/bench_memory

Continuous integration (`.github/workflows/ci.yml`) builds and runs the full
test suite on Arch Linux for every push. History, evidence and decisions are in
`CHANGELOG.md`, `docs/ledger.md` and `docs/TEST-PLAN.md`.
