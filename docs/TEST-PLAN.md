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
    symbol hints, auto-space, emoji row; About shows the installed version.

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

## G. Several languages at once (0.6.3)
34. Settings → Languages: English + German on. English active: `danke` Space stays `danke`, `teh` still -> `the`; no "+ Add to dictionary" for `danke`.
35. Ukrainian + Russian on, Ukrainian active: `привет` stays; `пять` -> `п'ять`.
36. Emoji panel → last chip `:-)`: faces on wide tiles; tap `¯\_(ツ)_/¯` -> inserted as shown (boxes instead of ツ: install `noto-fonts-cjk`); it does not appear under Recent.

## H. Glide typing (0.6.4)
37. Glide `hello`, `keyboard`, `привет`, `спасибо` quickly and with sloppy curves: the right word appears.
38. Glide `too` (the same path as `to`): if `to` appears, the strip shows `too`; tap it -> `to` is replaced by `too`, the space stays.
39. Glide `всё` over е: `всё`/`все` appear (one inserted, the other in the strip).
40. Glide a word, then Backspace once: the whole word (and its space) is gone. Shift, then glide: `Hello`; Caps Lock: `HELLO`.
41. Settings → Next-word suggestions Off: after a word the strip stays empty; completions while typing still appear.
42. Emoji panel → "Search emoji": the panel closes, letters (lowercase) type into the search bar, matching emoji appear; tap one -> inserted, normal typing again; ✕ leaves without typing anything.
43. Settings → Dictionary → "+ Add word": Shift + `oldenburg` -> `Oldenburg`, ✓, shortcut `olb`, ✓ -> back in Settings, word and `olb → Oldenburg` listed; typing `olb` offers `Oldenburg`; tap the shortcut chip -> removed.

## I. Autocorrect (0.6.9)
44. Type fast and sloppily: `otjerd` + Space -> `others`; Russian `ппивеи` + Space -> `привет` (two keys off).
45. `ofthe` + Space -> `of the`; `вобщем` -> `в общем`; `thisnis` -> `this is`; Backspace right after gives back what was typed. German `Haustür` typed as `haustür` is not split.
46. Ukrainian layout: `ка` offers `казав`/`каже`…, never `как`; `зн` never offers `знаешь`.
47. Settings → Interface language: with Plasma in German the button shows `System (Deutsch)` and the settings are German; tap it through English, Deutsch, Русский, Українська: the open panel changes at once; after a restart the choice is kept.
48. Type `окей` (Russian), `naja` (German), `honour` (English) + Space: they stay; `thats` still becomes `that's`.
49. Copy text in Firefox (Ctrl+C or the context menu), open the keyboard: the chip offers it, the clipboard panel lists it, Paste inserts it. (Before 0.7.1 the keyboard could not see the clipboard at all under KWin.)
50. Select a word by touch in a text field, open the text-editing panel: Copy and Cut are enabled; Copy, then paste elsewhere with Ctrl+V on a hardware keyboard or the panel's Paste: the word arrives; Cut removes it from the field. In a password field Copy/Cut stay disabled.
51. Text-editing panel in Firefox and Kate: Select all selects the whole field; Select (button lights up), then ← → Home End extend the selection; Copy, then Paste elsewhere inserts it; Cut removes it. The keyboard stays open throughout.


## J. Predictions and human glides (0.7.3)
52. Settings → Clear learned words, then type `thank` + Space: the strip offers `you`; tap it -> `thank you ` and the strip offers the next word. German `ich` + Space -> `bin`, `habe`…; Russian `я` -> `не`…; Ukrainian `я` -> `не`, `хочу`…
53. After `.`, `!` or `?` and Space, after a double-space period, and after Return: the strip shows the toolbar, not predictions from the sentence before.
54. Text-editing panel: Undo and Redo in Kate and Firefox undo and redo the last edit; in a password field they are hidden.
55. Glide fast with rounded corners, without stopping on the letters: `the`, `world`, `people`, `привет`, `дякую` come out right; when a word misses, it is in the strip.
