# V3 Keyboard — live test checklist (most important first)

Before: `touch ~/.local/state/v3-keyboard/trace.enable`, then relaunch the
keyboard. On any problem send `~/.local/state/v3-keyboard/trace.log`.

## A. Core typing (Firefox: claude.ai / ChatGPT, and a KDE app e.g. Kate)
1. Fast two-thumb typing with overlapping taps: no lost letters, no lag.
2. `teh` Space -> `the `; Backspace right after -> `teh` (kept next time).
3. `tge` / `thw` -> `the`; strip shows “teh” + **the** while typing `teh`.
4. Suggestion tap -> word + one space; Space after it -> no period.
5. `hi` Space Space -> `hi. ` and the next letter is uppercase.
6. Word + Enter in a chat: whole word sent.
7. Type a word, switch window, come back, continue: nothing lost.
8. Tap into the middle of text, continue typing: no duplicated letters.

## B. Keys and gestures
9. Long-press q -> 1; e (EN) -> picker (slide to è); е (RU) -> ё; `.` -> 16 marks.
10. `?123` slide to a symbol; Shift slide to a letter; `'` on symbols -> letters.
11. Space drag moves the cursor; Backspace drag deletes; long-press Space -> languages.
12. Glide a word: trail visible, word inserted with one space.

## C. Fields and panels
13. Password field: no underline, no suggestions. E-mail: `@`; URL: `/`, `.com`.
14. Phone/number field: number pad.
15. Emoji: search `улыб`/`lachen`, recent category; long-press comma opens emoji.
16. Settings: theme System/Light/Dark/AMOLED, number row, compact layout,
    symbol hints, auto-space, emoji row; About shows 1.0.0.

## D. Languages and extras
17. Globe: EN/DE/RU/UK switch instantly; first-letter suggestions per language.
18. Text shortcut: `~/.config/v3-keyboard/shortcuts.txt` with `omw = on my way`.
19. Voice (if installed): 🎤, speak, 🎤 -> text in the keyboard language.
20. `v3kbd-dictionaries --status`, `v3kbd-voice-setup --status`.
21. Settings → Keyboard layout → Split (landscape): halves apart, thumbs reach the space bar; typing works on both halves.
22. Tap into the middle of a misspelt word (e.g. "wrold"): the strip offers "world"; tapping it replaces the whole word.
23. English active, type "ghbdtn": strip offers "привет"; tap -> inserted, keyboard switches to Russian.
24. Copy text in an app: a 📋 chip appears in the strip; tap pastes it.
25. Settings → Long-press delay; long-press symbols react at the chosen speed.
