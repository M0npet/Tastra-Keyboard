# V3 Keyboard 0.3.0

Cumulative on top of the installed Beta 0.2.5 (includes 0.2.6 and 0.2.7).
0.2.x items: see docs/beta-0.2.7-completion.md.

## New in 0.3.0

- **Frequency-ranked suggestions.** Bundled 50k word-frequency lists for
  EN/DE/RU/UK (FrequencyWords / OpenSubtitles 2018, CC BY-SA 4.0, one-way
  compatible with GPLv3; see data/frequency/ATTRIBUTION.md). Words are
  filtered through Hunspell, so junk disappears and German nouns keep their
  capital ("ha" -> Haus). Suggestions start from the first letter
  ("t" -> the/to/that, "дя" -> дякую), cost ~0.03 ms per keystroke, and
  autocorrect gains a frequency prior (woudl->would, definately->definitely,
  хорошл->хорошо, wiel->weil) while valid words stay untouched.
- **Emoji search in the keyboard language.** CLDR keywords for EN/DE/RU/UK
  (Unicode License V3; data/emoji/ATTRIBUTION.md): "улыб", "усміш", "lachen",
  "сердце" find the right emoji; exact keyword > prefix > substring.
- **Compact layout.** Settings -> "Keyboard layout": Full width / Compact left
  / Compact right (62% width, docked to one side) for one-handed use.

Economy: all data is compiled into the binary as resources (~1.2 MB
compressed); only the active language is loaded, in the background.

## Not included (separate decisions)

- Voice and handwriting input need offline ML models (hundreds of MB, heavy
  CPU) — conflicts with "maximally economical" unless strictly opt-in.
- wlroots backend: not usable/testable on Plasma.
- Truly floating panel: KWin decides input-panel placement.

## Live-test checklist

Everything in docs/beta-0.2.7-completion.md, plus:
1. First-letter suggestions in all four languages; German noun capitals.
2. Emoji panel search in RU/UK/DE.
3. Settings -> Keyboard layout: Compact left/right, then back to Full width.
