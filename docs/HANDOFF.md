# Handoff — V3 Keyboard (state at 0.6.0 packaging, 2026-10-03)

Device: Minisforum V3, Arch, Qt 6.11.2, KWin 6.7.5, hunspell 1.7.3,
dictionaries en/de/ru (pacman) + uk (pinned LibreOffice upstream, user dir).
Test browser: Firefox (GTK3, text-input-v3).

Last live-tested install: Beta 0.2.5 (13/13 on device). 0.2.6 and 0.2.7 were
built/tested in a Qt 6.4 sandbox only (panel shell needs Qt >= 6.5, so the
binary itself is built by the deploy script on the device).

Key design facts (read docs/ledger.md for evidence):
- Firefox/GTK surrounding-text echoes are stale by construction; the keyboard
  composes the current word in the preedit and never deletes text there.
- Echoes never abandon a live preedit; KWin's reset ends it; unknown echoes
  within 150 ms of an own operation are self-caused.
- Corrections stay revertible in the preedit until the next key.
- Keys use one touch point each; glide is bound to its finger.
- Privacy-safe opt-in trace: ~/.local/state/v3-keyboard/trace.enable.

User requirements: native only (no Java/Electron/Chromium), maximally
economical, offline at runtime, dictionaries provisioned automatically at
install time, root-cause debugging, TDD, coherent release slices; the user
prefers finishing a slice fully and testing it all at once.

0.3.0 adds bundled frequency lists (CC BY-SA 4.0 -> GPLv3 one-way), CLDR emoji
keywords (Unicode License V3) and a compact left/right layout. Not done by
design: voice/handwriting (heavy offline models), wlroots backend, floating
panel (KWin controls placement).

0.4.0 adds opt-in offline voice (whisper.cpp + Qt Multimedia, compiled only
when both are installed; model via scripts/v3kbd-voice-setup.sh, pinned
SHA-256). Remaining ideas: handwriting (no light offline engine identified),
wlroots backend, floating panel (KWin controls placement).

0.5.0 adds Gboard behaviours (long-press symbols + picker, number row, emoji
suggestions, offensive filter, remove suggestion). Gboard ideas not done yet:
text shortcuts (personal dictionary), emoji fast-access row, fast symbols
gesture from ?123, .com on long-press period in URL fields.

0.6.0: neighbour-key corrections; Gboard strip; field-type layouts; text
shortcuts; recent emoji; gesture trail. Branch wip/latinime-scoring holds an
unfinished LatinIME-normalized scoring that regressed real-data suggestions
(kept for reference, not shipped). Remaining Gboard ideas: fast symbols
gesture from ?123, emoji fast-access row, auto-space after punctuation.
