# Changelog

Version numbers were reset on 2026-10-05: the project is about half-way to a
finished keyboard, so the builds that were numbered 0.5.0–1.4.0 are now the
internal builds 0.4.1–0.4.13, and the current state is 0.5.0.

## 0.6.0
- Wrong layout: "ghbdtn" typed with English active offers "привет"; tapping it
  also switches to Russian.
- Copied text is offered for pasting in the suggestion strip for a minute.
- Long-press delay setting (300 ms by default, as in Gboard).
- Text-editing panel: Up/Down.

## 0.5.4
- Emoji skin tones: long-press an emoji to choose the tone (Gboard); the grid
  no longer lists every tone variant separately.

## 0.5.3
- Tap into the middle of a word: the strip offers options for the whole word
  and a choice replaces all of it (Gboard). A Space inside a word just splits it.

## 0.5.2
- Split keyboard for thumb typing on the tablet (Settings → Keyboard layout →
  Split), as in Gboard on tablets; the space bar spans the gap.

## 0.5.1
- Settings are grouped into sections like Gboard (Languages, Preferences,
  Theme, Text correction, Glide typing, Clipboard, Dictionary, Advanced).
- Choose which languages the globe switches between.

## 0.5.0
- Clipboard like Gboard: items are kept for one hour unless pinned
  (long-press an item); only pinned items are stored on disk.
- Fix: a tap on an empty part of an open panel (clipboard, emoji, settings)
  no longer types the key underneath.

## 0.4.13 (internal build, was numbered 1.4.0)
- Backspace right after "word ," became "word, " undoes the swap (LatinIME).
- Put the cursor back after an autocorrected word: the strip offers what you
  originally typed; choosing it restores and keeps it (Gboard).
- Settings: Gesture trail on/off, Sound on keypress (off by default).

## 0.4.12 (internal build, was numbered 1.3.0)
- Capitalisation follows Google's LatinIME: no capital after abbreviations
  (e.g., U.S., т.е.) or German dates ("am 3. Oktober"), correct handling of
  quotes, brackets and new paragraphs.
- Hide button (⌄) on the toolbar, as in Gboard.

## 0.4.11 (internal build, was numbered 1.2.1)
- claude.ai/ChatGPT-style editors: the first letter typed into an empty
  paragraph is committed at once (their placeholder re-render broke the
  composition: first letter lost or replaced).
- The keyboard no longer disappears for a moment when a web editor briefly
  re-activates text input, so taps do not fall through onto the page.

## 0.4.10 (internal build, was numbered 1.2.0)
- Touch-aware correction: where your finger lands inside a key decides
  between neighbouring-key slips ("bst" -> "bat" when s was hit on its left
  edge, "bet" when on its upper-right edge).

## 0.4.9 (internal build, was numbered 1.1.3)
- Top-row key previews and pickers always fit inside the keyboard.
- Double-space period follows Google's LatinIME: only for two quick spaces
  (1.1 s) after a word; Backspace right after it leaves a single space.
- The settings list shows a scroll bar.

## 0.4.8 (internal build, was numbered 1.1.2)
- Holding Backspace keeps deleting (and speeds up) until released.
- Clearing the field turns automatic capitalisation back on.

## 0.4.7 (internal build, was numbered 1.1.1)
- Top-row key previews and long-press pickers are no longer hidden under the
  toolbar.
- Suggestions follow the case of the typed word ("Hel" -> "Hello").
- Learning data from older versions is migrated once, so typos typed during
  earlier tests (e.g. "teh") are corrected again.

## 0.4.6 (internal build, was numbered 1.1.0)
- Personal dictionary: tap an unknown typed word, then "+ Add to dictionary";
  manage words in Settings; bulk file ~/.config/v3-keyboard/dictionary.txt.
- A typo committed once is no longer treated as a valid word; unknown words
  become yours after 3 uses, or immediately when you add them or undo an
  autocorrection.

## 0.4.5 (internal build, was numbered 1.0.1)
- "word ," now becomes "word, " (space moves behind punctuation, as on
  Gboard/LatinIME) instead of dropping the space.
- Typing latency: worst keystroke 2.5–4.4 ms (was up to 56 ms in RU/UK on long
  unknown words); Space on long unknown words bounded (DE 122 -> 23 ms).
- Randomised stress test and sanitizer runs added to the verification.

## 0.4.4 (internal build, was numbered 1.0.0)
- Themes: System (follows Plasma light/dark), Light, Dark, AMOLED.
- About in Settings (version, license, data sources); GPL text (COPYING).
- Release build; docs and attributions installed; `v3kbd-rollback`, `v3kbd-uninstall`.

## 0.4.3 (internal build, was numbered 0.7.0)
- `?123` slide for fast symbols, Shift slide for a capital, apostrophe returns
  to letters, auto-space after punctuation (opt-in), emoji fast-access row,
  16 marks on period long-press.

## 0.4.2 (internal build, was numbered 0.6.0)
- Neighbour-key corrections; Gboard-style strip; number pad / @ / / / .com by
  field type; comma long-press for emoji; text shortcuts; recent emoji; glide trail.

## 0.4.1 (internal build, was numbered 0.5.0)
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
