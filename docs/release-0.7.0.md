# V3 Keyboard 0.7.0 — Gboard gestures and shortcuts

Cumulative since Beta 0.2.5. Behaviours as documented for Gboard
(Computerworld, 9to5Google, HelpDeskGeek, Hongkiat).

- **Fast symbols:** touch `?123`, slide onto a symbol, release — the symbol is
  typed and the letters come back.
- **Quick capital:** touch Shift, slide onto a letter, release — one capital.
- **Apostrophe returns to letters** on the symbols layer (Gboard 16.7 default),
  so "don't"/"it's" need no extra tap.
- **Auto-space after punctuation** (Settings; off by default, as on Gboard):
  `,` `.` `!` `?` `;` `:` after a word add a space before the next letter;
  numbers such as `3.5` stay intact.
- **Emoji fast-access row** (Settings; off by default): recent emoji above
  the keys.
- **Period long-press** now offers 16 marks including `%`, `/`, `#`, `*`.

## Live-test checklist

1. `?123` → slide to `5` → release: `5`, back on letters.
2. Shift → slide to `d`: `D`, then lowercase again.
3. Symbols layer: tap `'` → letters again.
4. Settings: Auto-space after punctuation On; `hi,there` → `hi, there`; `3.5` stays.
5. Settings: Emoji fast-access row On (after using an emoji) → row appears.
