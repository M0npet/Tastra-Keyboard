# V3 Keyboard Beta 0.2.4 — composition (preedit) for Firefox/GTK

Built on the device-verified Beta 0.2.3 install (13/13 tests on Qt 6.11.2).

## Root cause of the 0.2.3 live failures in Firefox (primary sources)

1. Firefox answers GTK's `retrieve-surrounding` from its parent-process cache,
   which lags the content process (Firefox `widget/gtk/IMContextWrapper.cpp`,
   `OnRetrieveSurroundingNative`: "selection cache in parent is still old data").
2. GTK 3.24 (`modules/input/imwayland.c`) asks only in `notify_im_change`,
   i.e. immediately after applying our commit.
3. Firefox's later retry goes through `gtk_im_context_set_surrounding`, which
   sends `set_surrounding_text` without `commit`; text-input-v3 state is
   double-buffered, so KWin forwards it only on the next keystroke.

=> In Firefox the keyboard always sees the text one step behind. 0.2.3 treated
those stale echoes as user edits (`TEH`, `JJjjjj`), was never "in sync"
(no `teh -> the`, no double-space period) and fell back to Backspace keys for
suggestions, which GTK applies after already-received commits (`hhello`).
KDE's plasma-keyboard computes replacements from the same stale data and shows
the same class of problems; its source documents multiple shifted echoes per
commit from Firefox/Chromium.

## Fix

- **Composition:** on text-input clients the current word is the client's
  preedit. Space/punctuation/suggestion commit the (corrected) word in one
  `commit_string`, which KWin applies together with clearing the preedit.
  Nothing is ever deleted, so stale echoes and channel ordering are irrelevant.
- **Double space without deletion:** after a word the space is held as preedit;
  a second Space commits `". "` instead.
- **Enter / cursor keys:** the word is committed first, then the key. GTK
  applies text-input commits before queued key events, so the order holds.
- **Focus change:** `preedit_string(..., commit=word)`; KWin commits it itself
  when keyboard focus moves to another surface (`commitPendingText`).
- **Echo history:** echoes of any recently produced state (including preedit
  states) are ignored; only unknown states are adopted as external edits.
- **Never preedit** in password/secret fields or for clients without text-input.
- Settings: "Underline word while typing" (default On) switches back to the
  0.2.3 immediate-commit behaviour.
- QML handlers use explicit parameters (Qt 6.11 deprecation warnings).
- Opt-in file trace (sizes/decisions only, never text).

## Live-test checklist (Firefox textarea and ChatGPT)

1. `teh` Space -> `the ` ; `wnats` Space -> `wants ` ; `knows` stays.
2. `h`, tap `hello` -> `hello ` ; `hi` Space Space -> `hi. ` ; next letter uppercase.
3. Backspace inside a word, then continue typing.
4. Type a word, tap into the middle of existing text, continue typing:
   no duplicated or lost letters (known risk area).
5. Type a word and press Enter in ChatGPT: the whole word is sent.
6. Type a word, switch to another window: the word stays.
7. Password field: no underline, no suggestions.

## Trace

    mkdir -p ~/.local/state/v3-keyboard && touch ~/.local/state/v3-keyboard/trace.enable
    # relaunch the keyboard; log: ~/.local/state/v3-keyboard/trace.log
    rm ~/.local/state/v3-keyboard/trace.enable   # to switch it off
