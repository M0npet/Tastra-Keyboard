# V3 Keyboard 0.4.0 — offline voice input (opt-in)

Cumulative: includes 0.2.6, 0.2.7 and 0.3.0.

## Voice input

- Tap 🎤 in the toolbar, speak, tap again: the text is inserted at the cursor
  (any unfinished word is committed first; sentence case; context updated).
- Fully offline: whisper.cpp (Arch `whisper-cpp`, MIT) with the multilingual
  `ggml-base-q5_1.bin` model (59.7 MB, SHA-256 422f1ae4…a8898, official
  whisper.cpp model repository). Language = current keyboard language.
- Economy: built only if `whisper-cpp` and `qt6-multimedia` are installed;
  model loaded on first use and released after 60 s idle; audio is kept in
  memory only (max 30 s) and zeroed after recognition; recognition runs on a
  worker thread (UI never blocks).
- Setup: the deploy script asks before building; later: `v3kbd-voice-setup`
  then re-run the deploy script. `v3kbd-voice-setup --status|--remove`.
- GPU: `V3KBD_VOICE_PACKAGE=whisper-cpp-vulkan v3kbd-voice-setup` for the
  Radeon iGPU (optional).

## Verification (sandbox)

- 12/12 CTest with voice ON (Qt 6.4 + whisper.cpp v1.9.1 built from source),
  11/11 with voice OFF, 3/3 KWin overlay; real whisper.cpp run exercised with
  the repository's test model (V3KBD_TEST_WHISPER_MODEL).
- Not verifiable in the sandbox: microphone capture (no audio device) and the
  model download (Hugging Face unreachable); both need the live test.

## Live-test checklist (voice)

1. Deploy answers "y" to voice; `v3kbd-voice-setup --status` all OK.
2. 🎤 visible; tap, say a sentence in RU/UK/DE/EN (switch keyboard language
   first), tap: text appears; tapping into another field cancels.
3. `top`/`htop`: memory drops ~1 min after the last dictation.
