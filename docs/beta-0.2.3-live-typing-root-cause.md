# V3 Keyboard Beta 0.2.3 — live typing root-cause release

Supersedes Beta 0.2.2 (never installed). Built on the device-verified Beta 0.2.1.
Live-test app: Firefox (GTK3, text-input-v3 via KWin 6.7).

## Root causes (with evidence)

1. **Replacement used two channels.** KWin turns `keysym` into fake
   `wl_keyboard` events (press only) while `commit_string` goes through
   text-input-v3 `commit_string + done` (KWin `src/inputmethod.cpp`,
   `keysymReceived`/`commitString`). GTK 3.24 applies text-input `done`
   synchronously inside the Wayland listener (`modules/input/imwayland.c`,
   `text_input_done`), but keyboard events are queued `GdkEvent`s. In one
   read batch the commit therefore overtakes the Backspaces: autocorrect,
   suggestion taps and double-space period could produce mangled text.
   The input-method-v1 protocol itself says key events must not be used for
   text operations. **Fix:** on text-input clients, delete via
   `delete_surrounding_text` (UTF-8 bytes; KWin maps it to v3 before/after +
   done) followed by `commit_string`, both on the same text-input object.
   Clients without text-input keep the key-event path (there KWin converts
   commits to key events as well, so ordering is preserved).
2. **0.2.2 stale-echo guard could stick.** After local typing, any client
   state that did not match was ignored, including real cursor moves; a later
   suggestion tap would replace the wrong text. Proven by
   `externalCursorMoveAdoptsClientState` (RED on 0.2.2). **Fix:** the engine
   records every text state it produces; an echo matching one of them is
   stale/confirming, anything else is an external edit and is adopted.
   Rewrites only happen when the client has confirmed the word; unconfirmed
   autocorrect is skipped instead of risking wrong deletions.
3. **Autocorrect had no validity oracle and no prior.** With a real en_US
   dictionary `teh -> meh`, `walked -> walker`, `wants -> cants`,
   `knows -> known`, `machte -> dachte`, `gehst -> geist` (ties broken
   alphabetically; affix rules ignored). Without dictionaries the same logic
   would rewrite any valid word one edit away from the 61 core words.
   **Fix:** libhunspell validity (affix-aware), ranking prior (core
   vocabulary, personal learning, transposition/repeated-letter edits),
   autocorrect only when confident; without a dictionary only a curated list
   of never-valid typos is autocorrected.
4. **Dictionary loading blocked the UI.** Measured: 1.2 s (EN, twice at
   start), 2.1 s (RU), 5.0 s (UK) on every Globe tap; first-letter
   suggestions 22 ms (RU) / 52 ms (UK). **Fix:** background loading,
   one language in memory, compact sorted index; measured 0.1–0.7 s off-thread,
   `setLanguage()` returns immediately, suggestions ≤ 0.5 ms, RAM roughly halved.
5. **Content purpose enum.** KWin sends text-input-v1 enums and maps PIN to
   password (8); 9 is *date*. Date fields were treated as secret. URL / email /
   number / phone / date / time / terminal fields now get no autocorrect,
   capitalization, suggestions or learning; the `lowercase` hint disables
   auto-capitalization.
6. **QML suggestion delegate touched its context after self-destruction**
   (`ReferenceError: root is not defined` on every suggestion tap).
7. **Non-hermetic tests.** Results depended on host dictionaries and wrote to
   real `~/.config`. Tests now use fixtures and QStandardPaths test mode.

## Refuted hypothesis

Repeater does *not* recreate letter keys on case-only changes (Qt 6.4,
positive-controlled test `caseChangesDoNotRecreateLetterKeys`). Not verified
on Qt 6.11; the test runs on the device as part of CTest.

## Dictionaries

`v3kbd-dictionaries` (installed to `~/.local/bin`) provisions dictionaries for
EN/DE/RU via pacman and UK from LibreOffice upstream (MPL-1.1, pinned commit
`32b006a2c22a4ac7e8ed3f03346f7b3d85a970a4`, SHA-256 verified) into
`~/.local/share/v3-keyboard/dictionaries`. The keyboard never uses the network.

## Live-test checklist (Firefox)

1. Latency while typing normally, and when tapping Globe (no freeze).
2. `teh` + Space -> `the ` ; `wnats` + Space -> `wants ` ; `knows` stays.
3. Type `hel`, tap suggestion `hello` -> `hello ` (no leftovers).
4. `hi` + Space + Space -> `hi. ` ; `.` then a letter -> uppercase.
5. Tap into the middle of existing text, then tap a suggestion: replaces the
   word at the new cursor.
6. URL bar / e-mail field: no capitalization, no suggestions.

## Protocol trace (privacy-safe: sizes/offsets only, never text)

    mkdir -p ~/.config/QtProject
    printf '[Rules]\nv3keyboard.*.debug=true\n' >> ~/.config/QtProject/qtlogging.ini
    # relaunch the keyboard, then:
    journalctl --user -b -f | grep -i v3keyboard

## Not in this release / next candidates

- Preedit-based composition (Gboard-style underlined current word) would
  remove deletions entirely; changes the approved immediate-commit design,
  needs approval.
- Multi-touch: keys use MouseArea; whether overlapping two-thumb taps are
  dropped on Qt 6.11 is unverified; measure before changing input handling.
- Completions come from dictionary stems + learned words (no frequency list).
