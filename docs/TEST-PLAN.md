# Tastra — live test checklist (most important first)

Before: `touch ~/.local/state/tastra/trace.enable`, then relaunch the
keyboard. On any problem send `~/.local/state/tastra/trace.log`.

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
18. Text shortcut: `~/.config/tastra/shortcuts.txt` with `omw = on my way`.
19. Voice (if installed): 🎤, speak, 🎤 -> text in the keyboard language.
20. `tastra-dictionaries --status`, `tastra-voice-setup --status`.
21. Settings → Keyboard layout → Split (landscape): halves apart, thumbs reach the space bar; typing works on both halves.
22. Tap into the middle of a misspelt word (e.g. "wrold"): the strip offers "world"; tapping it replaces the whole word.
23. English active, type "ghbdtn": strip offers "привет"; tap -> inserted, keyboard switches to Russian.
24. Copy text in an app: a 📋 chip appears in the strip; tap pastes it.
25. Settings → Long-press delay; long-press symbols react at the chosen speed.
26. ?123 → currency of the language; "=\\<" → second page with < > { } [ ] \\ |; long-press " → « » „ “.

## E. Tastra name and install (0.6.2)
27. After installing over 0.6.1: `ls -la ~/.config/tastra ~/.local/share/tastra` show your dictionary.txt, shortcuts.txt, tastra.conf and dictionaries; `~/.config/v3-keyboard` is a link.
28. Your theme, pinned clipboard items and added words are still there.
29. `kreadconfig6 --file kwinrc --group Wayland --key InputMethod` names `io.github.m0npet.Tastra.desktop`; after relogin the keyboard still appears.
30. `tastra-update --check` says up to date; `tastra-rollback` brings the previous build back (then `tastra-update` again).


## F. Autocorrect like Android's keyboard (0.6.2)
31. English: `dont` Space -> `don't`; `im` -> `I'm`; `i` -> `I`; `ive` -> `I've`; `cant` stays, strip offers `can't`.
32. Ukrainian: `пять` -> `п'ять`, `звязок` -> `зв'язок`; Russian: `обьект` -> `объект`, `щас` stays `щас`.
33. German: `uber` -> `über`, `strasse` -> `Straße`, `naturlich` -> `natürlich`; Backspace right after undoes it.
