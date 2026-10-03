# V3 Keyboard 0.6.0 — more Gboard logic

Cumulative since Beta 0.2.5. Behaviour modelled on Gboard's public features;
correction logic follows ideas of Google's open-source AOSP LatinIME
(Apache-2.0). No Gboard code or data used.

- **Neighbour-key corrections** (LatinIME ProximityInfo idea): a slip to an
  adjacent key is the likeliest typo. Real data: tge/thw/yhe -> the,
  hsus -> haus, nivht -> nicht, пртвет -> привет, сппсибо -> спасибо; no
  regression on the 0.5.0 bench. "helo" is now ambiguous (hello/help) and only
  suggested, not auto-corrected.
- **Gboard suggestion strip:** when Space would correct the word, the strip
  shows “typed” (tap keeps it) and the correction in bold in the middle.
- **Field types:** number pad in number/phone fields (+ in phone); `@`
  instead of comma in e-mail fields, `/` in URL fields; long-press the period
  there for .com/.de/.ru/.ua/.org/.net.
- **Long-press comma** opens the emoji panel.
- **Text shortcuts** (Gboard personal dictionary): lines like
  `omw = on my way` or `adp<TAB>Android Police` in
  `~/.config/v3-keyboard/shortcuts.txt`; the expansion appears first in the
  strip. Edits are picked up on the next field focus.
- **Recently used emoji** category (newest first, persisted).
- **Gesture trail** while gliding.

## Live-test checklist

1. `tge` Space -> `the`; strip shows “teh” + **the** while typing `teh`.
2. Phone/number field (e.g. a login form) -> number pad; e-mail -> @.
3. Long-press `.` in the address bar -> .com …
4. Create `~/.config/v3-keyboard/shortcuts.txt` with `omw = on my way`,
   focus a field, type `omw` -> tap the suggestion.
5. Emoji panel -> use two emoji -> "Recent" category.
6. Glide a word: a trail follows the finger.
