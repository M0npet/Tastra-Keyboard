# V3 Keyboard 0.5 (in development)

A Gboard-like on-screen keyboard for KDE Plasma 6 on Wayland (KWin), made for
the Minisforum V3 tablet. Native C++/Qt 6/QML — no Java, Electron or browser
engine — fully offline, economical (one language in memory, data loaded in the
background). GPL-3.0-or-later (see COPYING).

## Install / update

From a release bundle:

    mkdir -p ~/Downloads/v3kbd && tar --zstd -xf v3-keyboard-*-bundle.tar.zst -C ~/Downloads/v3kbd
    cd ~/Downloads/v3kbd && sha256sum -c SHA256SUMS && ./v3kbd-*-deploy.sh
    kcmshell6 kcm_virtualkeyboard      # None -> Apply -> V3 Keyboard -> Apply

The deploy script builds in an isolated git worktree, runs the full test suite
and installs only when everything is green (the previous binary is backed up).

- Dictionaries: `v3kbd-dictionaries [--status]` (EN/DE/RU via pacman, UK from a
  pinned, checksummed upstream file).
- Voice (optional): `v3kbd-voice-setup [--status|--remove]`, then re-run deploy.
- Roll back to the previous build: `v3kbd-rollback`.
- Uninstall: `v3kbd-uninstall [--purge]`.

## Typing

- Words are composed underlined (preedit) and committed by Space, punctuation,
  a suggestion or glide — reliable autocorrect even in Firefox.
- Autocorrect (libhunspell + word frequency + neighbouring keys), conservative;
  Backspace right after a correction undoes it and remembers the word.
- Suggestion strip: “typed” word, **correction** (what Space inserts), next
  best; first-letter completions; next-word predictions from your own typing;
  emoji suggestions; long-press a suggestion to remove it; offensive words are
  never suggested (Settings).
- Glide typing with trail; two-thumb typing; long-press for symbols with key
  hints and a picker (accents, ß/ё/ґ, digits); period long-press for 16 marks.
- Gestures: Space drag moves the cursor, Backspace drag deletes, `?123` slide
  for a quick symbol, Shift slide for one capital, double-space for ". ".
- Field-aware: number pad in number/phone fields, `@`/`/` and `.com` in
  e-mail/URL fields, no autocorrect/learning in passwords, URLs, e-mail.
- Personal dictionary: tap an unknown typed word (shown in quotes), then
  “+ Add to dictionary”; manage in Settings; bulk: one word per line in
  `~/.config/v3-keyboard/dictionary.txt`. Unknown words you type become yours
  after 3 uses; a single typo is never learned as a word.
- Text shortcuts: `~/.config/v3-keyboard/shortcuts.txt` (`omw = on my way`).
- Emoji panel with search in EN/DE/RU/UK, recent emoji, optional fast-access row.
- Clipboard history, text-editing panel, compact (one-handed) layout, number
  row, themes (System/Light/Dark/AMOLED), offline voice input (optional).

## Privacy

Nothing leaves the device. Learned words stay in `~/.config/V3Keyboard`.
Dictation audio stays in memory only (max 30 s) and is erased after
recognition. The optional trace (`touch ~/.local/state/v3-keyboard/trace.enable`)
records sizes and decisions only, never text.

## Third-party data

FrequencyWords (CC BY-SA 4.0), Unicode CLDR annotations (Unicode License V3),
LDNOOBW word lists (CC BY 4.0), Hunspell dictionaries (system packages),
LibreOffice uk_UA dictionary (MPL-1.1), optional whisper.cpp + ggml model
(MIT). Details: `data/*/ATTRIBUTION.md`, installed to
`~/.local/share/doc/v3-keyboard`.

## Development

Continuous integration (`.github/workflows/ci.yml`) builds and runs the full
test suite on Arch Linux for every push.

    cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
    python3 scripts/verify-static.py

History, evidence and decisions: `docs/ledger.md`, `CHANGELOG.md`, `docs/HANDOFF.md`.
