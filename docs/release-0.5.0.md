# V3 Keyboard 0.5.0 — Gboard behaviours

Cumulative since Beta 0.2.5. Behaviours modelled on Gboard's documented
features (Gboard is closed source; no code was copied).

- **Long-press for symbols** (Gboard "Long press for symbols"): top row ->
  digits 1–0 (QWERTY, QWERTZ and ЙЦУКЕН by position), second row ->
  `@ # $ _ & - + ( )`, third row -> `* " ' : ; ! ?`, with hints drawn in
  the key corners (Settings: "Long-press symbols (key hints)").
- **Picker:** when a key has several choices (EN accents, DE ß + #,
  RU ё + 5, UK ґ + 7, period punctuation) a picker opens; slide to choose,
  release to insert. Language letters stay preselected, so long-press
  е -> ё still works without sliding.
- **Long-press the period** for punctuation.
- **Number row** (Settings: "Number row"; off by default as on Gboard).
- **Emoji suggestions:** a word that is exactly an emoji keyword in the
  keyboard language or English ("pizza", "пицца") shows the emoji in the
  strip; tapping it adds the emoji after the word.
- **Block offensive words** (on by default, as on Gboard): words from the
  LDNOOBW lists (CC BY 4.0; data/blocklist/ATTRIBUTION.md) are never
  suggested or used as autocorrect targets; typing them is never blocked.
  Measured need: 20 such EN words were within the top 3000 of the bundled
  frequency list (first at rank 277).
- **Long-press a suggestion to remove it** (unlearns it and never suggests it
  again; typing it yourself later brings it back).

Already present before and Gboard-like: glide typing, voice typing,
undo autocorrect on backspace, space-bar cursor, backspace swipe, long-press
space/Globe for languages, one-handed (compact) layout, clipboard, emoji search.

## Live-test checklist

1. Long-press q/w/e… in EN, DE, RU, UK: digits; hints visible; picker slide.
2. Long-press е (RU) -> ё directly; slide for 5.
3. Long-press `.` -> punctuation picker.
4. Settings -> Number row on: digits row appears, panel grows.
5. Type "pizza"/"пицца" -> 🍕 in the strip.
6. First-letter suggestions contain no profanity; Settings toggle works.
7. Long-press a suggestion -> it disappears and is not inserted.
