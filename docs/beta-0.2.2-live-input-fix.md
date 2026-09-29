# V3 Keyboard Beta 0.2.2 — live input repair

This release is a focused live-test repair on top of Beta 0.2.1.

## Fixed

1. **Autocorrect / suggestion replacement in browser fields**
   - Local composition is no longer overwritten by stale `surrounding_text` snapshots generated before the latest virtual-keyboard commit.
   - `teh` can remain a coherent local token until Space triggers correction.
   - Tapping a suggestion retains the word that must be replaced instead of randomly treating it as empty.

2. **Typing latency**
   - Normal character taps no longer emit `keyboardStateChanged` while suggestions are enabled.
   - Only the suggestion strip is invalidated on an ordinary tap.
   - This avoids re-evaluating all row/key bindings on every character in QML.
   - Repeated Backspace refreshes composition only once per batch.

3. **Learning write amplification**
   - Word and bigram learning are persisted in one combined update per finalized word rather than two full settings serializations.

4. **Sentence capitalization**
   - `.`, `!`, and `?` arm auto-capitalization immediately, even before a new surrounding-text snapshot arrives.

## Regression coverage

- stale partial-word surrounding-text echo
- stale pre-space surrounding-text echo
- autocorrect after stale echo
- immediate terminal-punctuation capitalization
- ordinary typing does not invalidate the whole keyboard
- repeated Backspace keeps composition coherent
