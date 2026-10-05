# Changelog

## 1.2.0
- Touch-aware correction: where your finger lands inside a key decides
  between neighbouring-key slips ("bst" -> "bat" when s was hit on its left
  edge, "bet" when on its upper-right edge).

## 1.1.3
- Top-row key previews and pickers always fit inside the keyboard.
- Double-space period follows Google's LatinIME: only for two quick spaces
  (1.1 s) after a word; Backspace right after it leaves a single space.
- The settings list shows a scroll bar.

## 1.1.2
- Holding Backspace keeps deleting (and speeds up) until released.
- Clearing the field turns automatic capitalisation back on.

## 1.1.1
- Top-row key previews and long-press pickers are no longer hidden under the
  toolbar.
- Suggestions follow the case of the typed word ("Hel" -> "Hello").
- Learning data from older versions is migrated once, so typos typed during
  earlier tests (e.g. "teh") are corrected again.

## 1.1.0
- Personal dictionary: tap an unknown typed word, then "+ Add to dictionary";
  manage words in Settings; bulk file ~/.config/v3-keyboard/dictionary.txt.
- A typo committed once is no longer treated as a valid word; unknown words
  become yours after 3 uses, or immediately when you add them or undo an
  autocorrection.

## 1.0.1
- "word ," now becomes "word, " (space moves behind punctuation, as on
  Gboard/LatinIME) instead of dropping the space.
- Typing latency: worst keystroke 2.5–4.4 ms (was up to 56 ms in RU/UK on long
  unknown words); Space on long unknown words bounded (DE 122 -> 23 ms).
- Randomised stress test and sanitizer runs added to the verification.

## 1.0.0
- Themes: System (follows Plasma light/dark), Light, Dark, AMOLED.
- About in Settings (version, license, data sources); GPL text (COPYING).
- Release build; docs and attributions installed; `v3kbd-rollback`, `v3kbd-uninstall`.

## 0.7.0
- `?123` slide for fast symbols, Shift slide for a capital, apostrophe returns
  to letters, auto-space after punctuation (opt-in), emoji fast-access row,
  16 marks on period long-press.

## 0.6.0
- Neighbour-key corrections; Gboard-style strip; number pad / @ / / / .com by
  field type; comma long-press for emoji; text shortcuts; recent emoji; glide trail.

## 0.5.0
- Long-press symbols with hints and picker; number row; emoji suggestions;
  offensive-word filter; long-press a suggestion to remove it.

## 0.4.0
- Optional offline voice input (whisper.cpp).

## 0.3.0
- Frequency-ranked suggestions; emoji search in EN/DE/RU/UK; compact layout.

## 0.2.3 – 0.2.7
- Composition (preedit) for reliable editing in Firefox/GTK; stale-echo
  handling from a device trace; libhunspell autocorrect; background
  dictionaries; two-thumb typing; undoable autocorrect; context hygiene.
