# Changelog

Version numbers were reset on 2026-10-05: the project is about half-way to a
finished keyboard, so the builds that were numbered 0.5.0–1.4.0 are now the
internal builds 0.4.1–0.4.13, and the current state is 0.5.0.

## 0.6.9
- Autocorrect fixes words with several slipped keys, as LatinIME's
  proximity search does ("otjerd" -> "others"): before, a word was only
  corrected when a single key was off. With sloppy taps (simulated) the share
  of typos fixed went from 36-72 % to 80-93 %.
- Two words typed without the space ("ofthe", "вобщем", "потомучто"), or
  with a letter above the space bar for it ("thisnis"), become two words on
  Space. German compounds stay one word.
- Typing Ukrainian no longer offers Russian words: the Ukrainian word list
  had 15 500 of them ("что" was its 8th most common word).
- Words that are only in Hunspell's dictionary rank below listed ones, so
  "ohers" is no longer changed to "hoers" ("others" is offered first).

## 0.6.8
- The keyboard starts about twice as fast and uses ~6 MB less memory: its
  panels (settings, emoji, clipboard, text editing) are built when first
  opened instead of at every start.

## 0.6.7
- Settings → Dictionary → "+ Add word", as in Gboard: type the word on the
  keyboard (Shift for capitals), then an optional shortcut; typing the
  shortcut later offers the word. Shortcuts are listed in Settings and removed
  with a tap; `shortcuts.txt` keeps its comments and other lines.

## 0.6.6
- Emoji search like Gboard: tap "Search emoji" and the keyboard's own letters
  type the search (lowercase), with the matching emoji in a row above the
  keys; tap one to insert it, Enter or ✕ to leave. Before, the search field
  needed a hardware keyboard, which an on-screen keyboard's own panel can
  never get.

## 0.6.5
- One Backspace right after a glide erases the whole glided word (Gboard).
- Shift before a glide capitalizes the word, Caps Lock writes it in capitals.
- Settings → Text correction → Next-word suggestions (on by default, as in
  Gboard): turns off the predictions shown after a word.

## 0.6.4
- Glide typing rebuilt the way Gboard and Android's keyboard do it: the
  finger's path is compared with each word's ideal path over the keys
  (position and shape) together with how common the word is. Keys the finger
  only crosses no longer break a word. On synthetic glides of the 200 most
  common words per language: 95–98 % right (the old decoder: 3–6 %).
- After a glide the strip shows the other words the path could mean
  ("to" / "too", "das" / "dass"); tapping one replaces the glided word.
- Letters without their own key are glided over their base key (ё over е,
  ß over s).

## 0.6.3
- Multilingual typing like Gboard: with English and German (or Ukrainian and
  Russian) both enabled, a word of the other language is left as typed —
  `danke` is no longer "corrected" to `dance`, nor `привет` to `привіт` — and
  is not offered for the personal dictionary. A nearly free fix in the active
  language still wins (`пять` -> `п'ять` on the Ukrainian layout).
- Emoticons: the emoji panel has Gboard's `:-)` tab with 58 text faces, from
  `:-)` and `<3` to `¯\_(ツ)_/¯` and `(╯°□°)╯︵ ┻━┻`, on wide tiles. A face goes
  in exactly as shown and does not crowd the recent emoji. (The Japanese
  characters in a few faces need a CJK font: `sudo pacman -S noto-fonts-cjk`.)

## 0.6.2
- New name: **Tastra** (from German *Tastatur*), and a home on GitHub:
  https://github.com/M0npet/Tastra-Keyboard. Settings, learned and added words,
  shortcuts, dictionaries, the voice model and backups move over from the
  "V3 Keyboard" folders automatically on the first start; the old folders
  become links, so rolling back to 0.6.1 still works.
- Install and update with one command: `scripts/tastra-install.sh` builds,
  tests and installs a checkout; `tastra-update` pulls the latest version from
  GitHub and does the same. KWin's keyboard choice is moved to the new name.
- Autocorrect as in Android's keyboard (AOSP LatinIME error model):
  - a left-out apostrophe is almost free: "dont" -> "don't", "im" -> "I'm",
    "isnt" -> "isn't" (was "inst"), "ive" -> "I've" (was "vie"),
    "пять" -> "п'ять", "мясо" -> "м'ясо", "звязок" -> "зв'язок";
  - a letter without its accent too, and German ue/oe/ae and ss: "uber" ->
    "über" (was "aber"), "ueber" -> "über", "strasse" -> "Straße",
    "grusse" -> "Grüße", "ганок" -> "ґанок";
  - the dictionary's spelling: "i" -> "I", "monday" -> "Monday", German
    nouns keep their capital ("hasu" -> "Haus");
  - confidence grows with word length: one wrong letter in a long word is
    corrected ("пожалуйсто", "обьект"), a different letter in a three-letter
    word is not ("щас" no longer becomes "вас");
  - real words stay as typed, but the strip offers the contraction ("cant" ->
    "can't", "ill" -> "I'll").
- English and Ukrainian word lists now contain words with an apostrophe
  (taken from wordfreq), so "don't" and "п'ять" are suggested by frequency.
- Less memory: about a quarter less with a dictionary loaded (Ukrainian 81 ->
  62 MB, Russian 48 -> 36 MB, English 32 -> 25 MB), by handing memory freed
  after loading back to the system and keeping the wrong-layout word list as
  hashes.

## 0.6.1
- Second symbol page "=\<" (< > [ ] { } \ | ~ ^ = % © ® ™ £ € ¥ …), the
  language's currency on the first page, and long-press extras such as « »,
  „ “, – —, ½ and other currencies.

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
