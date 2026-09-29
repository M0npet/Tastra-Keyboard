# V3 Keyboard Beta 0.2.7 — v0.2 milestone completion

Cumulative on top of Beta 0.2.5 (installed) — includes 0.2.6.

## Included since the last live test (0.2.5)

- **0.2.6 — compositions survive unreliable echoes.** Device trace showed
  claude.ai's empty composer reports a 1-byte placeholder, so echo contents
  never match; the pending space was dropped ("Slovoi"). Echoes can no longer
  abandon a live preedit (KWin's reset ends it); unknown echoes within 150 ms
  of an own operation are self-caused (measured latency p99 6 ms, max 36 ms).
- **Two-thumb typing.** Keys used MouseArea, which only receives the first
  touch point: an overlapping second tap was lost (reproduced with the real
  Main.qml). Keys now use one MultiPointTouchArea point each; the glide
  candidate is bound to the finger that started it (a second finger means
  tapping, not gliding). Mouse input, glide, long-press alternates, key
  popups and the Space/Backspace drag gestures are covered by UI tests.
- **Undo autocorrect with Backspace.** A corrected word is held with its space
  in the preedit until the next key; Backspace right after the correction
  restores exactly what was typed and remembers it (never corrected again).
  Learned words also take priority over the curated typo list.
- **Glide words behave like suggestions** (auto space; a following Space
  confirms it instead of producing ". ").
- **Input-method contexts are destroyed after deactivate** (protocol
  requirement; previously one proxy leaked per focus change). Not unit-testable
  without a compositor; exercised by every focus change on device.

## Live-test checklist (all at once)

In claude.ai / ChatGPT in Firefox, plus the local field page
(`/tmp/v3kbd-fields.html`):

1. Fast typing with both thumbs, overlapping taps: no lost letters.
2. `teh` Space -> `the ` ; then Backspace -> `teh` ; Space -> keeps `teh`.
3. Suggestion tap, then Space: one space, no period.
4. `hi` Space Space -> `hi. ` and the next letter is uppercase.
5. Glide a word, then Space: no period.
6. Long-press `s` (DE) / `г` (UK) / `е` (RU) -> ß / ґ / ё.
7. `slovo` Space, switch window, come back, `i drugoe` -> `Slovo i drugoe`.
8. Cursor via Space-drag mid-sentence, continue typing: lowercase.
9. Word + Enter in a chat: whole word sent.
10. Password / e-mail / URL fields behave as before (no preedit/suggestions).
11. Language switching (Globe) is instant; no freezes while typing.

Trace stays available: `~/.local/state/v3-keyboard/trace.enable` ->
`~/.local/state/v3-keyboard/trace.log` (sizes and decisions only, never text).

## Deferred (need a decision, not part of v0.2)

- Frequency-ranked completions (needs a frequency word list; licensing).
- wlroots backend, floating panel, CLDR emoji keywords, handwriting/voice
  (explicitly deferred by the v0.2 spec).
