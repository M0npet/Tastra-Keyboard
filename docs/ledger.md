# Development Ledger

## 2026-09-24 — Baseline

Status:
- Specification v0.1 approved
- Implementation plan approved
- Execution started
- Repository initialized

Current milestone:
- KWin vertical slice

Architecture invariant:
- compositor-specific code stays outside core/model

## 2026-09-24 — RED: basic text commit

Test:
- KeyboardControllerTest::tappingTextKeyCommitsExactlyOnce

Required behavior:
- tapping textual key "a"
- calls backend commitText("a")
- exactly once

RED evidence:
- production InputMethodBackend / KeyboardController do not exist yet

Next:
- implement the minimum core classes required to make this test green

## 2026-09-24 — GREEN: basic text commit

Implementation:
- InputMethodBackend interface added
- KeyboardController added
- tapText() forwards text to commitText()
- core is built as a compositor-independent static library

Verification:
- full build required
- full CTest suite required

## 2026-09-24 — REFACTOR: backend lifetime invariant

Change:
- KeyboardController now requires InputMethodBackend by reference.
- Null backend state is impossible by construction.

Verification:
- full build passed
- full CTest suite passed
- git diff --check passed

## 2026-09-24 — RED: KWin input-method-v1 commit seam

Required behavior:
- active v1 context receives commit text
- commit uses the latest compositor commit_state serial
- no active context means commit is ignored
- deactivated context receives no commits

Design boundary:
- protocol context is abstracted behind InputMethodV1Context
- actual QtWayland protocol object will be added after this seam is green

## 2026-09-24 — GREEN: KWin input-method-v1 commit seam

Implementation:
- KWinInputMethodV1Backend implements the compositor-independent InputMethodBackend interface
- protocol context is isolated behind InputMethodV1Context
- latest commit_state serial is stored explicitly
- commit without an active context is ignored
- deactivation is represented by clearing the active context

Verification:
- full build required
- full CTest suite required
- git diff --check required

Next:
- add the real QtWayland input-method-v1 protocol adapter behind InputMethodV1Context

## 2026-09-24 — RED: KWin input-method-v1 session lifecycle

Required behavior:
- activation binds the active protocol context
- commit_state updates the serial used for commit_string
- deactivation clears only the matching active context
- stale deactivation must not clear a newer context

## 2026-09-24 — GREEN: KWin input-method-v1 session lifecycle

Implementation:
- session owns protocol lifecycle state, not protocol objects
- backend is updated on activation/deactivation
- latest serial is accepted only while a context is active
- stale deactivation is ignored

Next:
- connect this tested session to the real QtWayland input-method-v1 callbacks

## 2026-09-24 — RED: real QtWayland input-method-v1 adapter

Goal:
- compile a concrete KWin input-method-v1 client extension
- keep protocol objects behind the tested backend/session boundary
- use generated QtWayland client bindings from the system wayland-protocols XML

## 2026-09-24 — GREEN: real QtWayland input-method-v1 adapter

Implementation:
- Qt 6 WaylandClient generates client bindings from wayland-protocols
- KWinInputMethodV1Connection implements zwp_input_method_v1
- protocol activation creates a concrete context adapter
- protocol deactivation clears only the matching context
- commit_state is forwarded into the tested session/backend path
- commitText ultimately maps to zwp_input_method_context_v1.commit_string

Boundary:
- generated/protocol-specific types stay in platform/kwin
- core/model remains compositor-independent

Next:
- expose activation state to the application layer
- add input-panel-v1 surface integration
- then build the first minimal QML keyboard executable

### Stage 8 build repair

The input-method-v1 XML references the core `wl_keyboard` interface.
Qt 6.11 generates extension C bindings with `wayland-client-core.h` by
default, which does not declare `wl_keyboard_interface`.

For this protocol target we use `NO_INCLUDE_CORE_ONLY`, causing the generated
binding to include the full Wayland client protocol declarations required by
the referenced core interface.

### Stage 8 build repair #2

The generated `qwayland-input-method-unstable-v1.h` is part of the build tree.
`kwininputmethodv1connection.h` includes it, so build-tree consumers of
`v3keyboard-platform-kwin` also need the binary directory on their include path.

Added `${CMAKE_CURRENT_BINARY_DIR}` as a BUILD_INTERFACE include directory.
This is intentionally build-only; an installed public API should later hide
generated protocol types behind a private implementation boundary.

## 2026-09-24 — RED: QML input bridge

Required behavior:
- QML-facing bridge accepts text taps
- bridge forwards text into KeyboardController
- UI remains independent of KWin protocol types

## 2026-09-24 — GREEN: KWin input panel + minimal QML surface

Implementation:
- QML-facing KeyboardUiBridge added
- KWin input-panel-v1 shell integration added
- input panel surface uses center-bottom toplevel role
- real v3-keyboard executable added
- context activation controls keyboard visibility
- first QML key commits "a" through the tested controller/backend path
- Plasma virtual-keyboard desktop metadata added, not installed yet

Source provenance:
- low-level QtWayland input-panel shell/surface structure is adapted from KDE plasma-keyboard
- original Jan Arne Petersen LGPL-2.1-only attribution retained in adapted files

Safety:
- this stage is build/test only
- executable is NOT launched against the live Plasma session
- desktop entry is NOT installed yet

Next:
- local user install
- select V3 Keyboard in Plasma System Settings
- manual E2E smoke: focus field -> panel appears -> tap A -> "a" committed -> focus leaves -> panel hides

### Stage 9 build repair #2

The input-panel integration intentionally uses Qt private Wayland/QPA headers.
On Arch Linux, the private imported CMake targets are packaged separately from
the public module discovery and must be loaded explicitly.

Added:
- Qt6GuiPrivate
- Qt6WaylandClientPrivate

This keeps the private-API dependency isolated inside the KWin platform module.

### Stage 9 build repair #3

`qt_generate_wayland_protocol_client_sources()` generates both C and C++
sources. The generated C source defines protocol interface symbols such as
`zwp_input_method_v1_interface` and `zwp_input_panel_v1_interface`.

The project initially enabled only CXX, so the generated C protocol object was
not part of the link, leaving those interface symbols undefined.

Enabled both C and CXX at project level.

## 2026-09-24 — Milestone 1 manually verified on Plasma/KWin

Verified in the live user session:
- KWin discovered and launched V3 Keyboard
- focusing a real text field activated input-method-v1
- the input-panel-v1 QML surface appeared
- tapping the test A key committed "a" into the real application
- removing focus hid the keyboard

Milestone 1 status: complete.

## 2026-09-24 — RED: basic keyboard model

Required behavior:
- lowercase by default
- first Shift enables one-shot uppercase
- one-shot Shift is consumed by a letter
- second Shift enables Caps Lock
- Caps Lock survives typed letters
- pressing Shift while Caps Lock is active returns to lowercase

## 2026-09-24 — GREEN: QWERTY + Shift/Caps + Space

Implemented:
- English QWERTY letter rows
- one-shot Shift
- Caps Lock by pressing Shift twice
- Space
- UI reads case state from KeyboardModel
- UI remains independent of KWin/Wayland protocol types

Next:
- semantic Backspace and Enter actions
- map those actions to input-method-v1 custom keysyms

## 2026-09-24 — RED: semantic Backspace and Enter

Required behavior:
- KeyboardController exposes Backspace and Enter as semantic actions
- compositor-specific key generation stays behind InputMethodBackend

## 2026-09-24 — GREEN: semantic Backspace and Enter

Implemented:
- InputMethodBackend now exposes Backspace and Enter semantics
- KeyboardController forwards those actions without compositor knowledge
- KWin input-method-v1 backend maps them to custom keysyms
- both press and release events are emitted with the latest serial
- QML now exposes Backspace and Enter keys

Protocol rationale:
- normal text continues to use commit_string
- custom key events use input-method-v1 keysym, as specified by the protocol

## 2026-09-24 — Milestone 2 manually verified on Plasma/KWin

Verified in the live user session:
- English QWERTY input works
- one-shot Shift works
- Caps Lock works
- Space works
- Backspace works
- Enter works

Milestone 2 status: complete.
This commit is the known-good checkpoint before mobile layout work.

## 2026-09-24 — RED: alphabet/symbol layer state

Required behavior:
- keyboard starts in alphabet layer
- ?123 switches to symbol layer
- switching layer clears Shift/Caps state
- ABC returns to alphabet layer

## 2026-09-24 — GREEN: mobile layout foundation

Implemented:
- responsive panel width based on the active screen
- separate portrait/landscape geometry constants
- centered content with a maximum ergonomic width
- alphabet and ?123 symbol layers
- comma and period on the primary bottom row
- ABC return key on symbol layer
- wider Space, Shift, Backspace and Enter keys
- symbol-layer switching resets Shift/Caps state

Architecture:
- layout state lives in KeyboardModel
- QML is responsible for visual geometry only
- KWin/Wayland integration is unchanged from the known-good checkpoint

## 2026-09-24 — RED: tablet mobile-layout metrics

Visual goals derived from live V3 portrait/landscape screenshots:
- landscape should be materially more compact vertically
- landscape should use more horizontal space than the previous prototype
- portrait should stay close to full width
- geometry must remain explicit and independently testable

## 2026-09-24 — GREEN: Gboard-inspired geometry and interaction pass

Implemented from live V3 screenshots:
- landscape panel reduced to a compact tablet height
- landscape keyboard uses a wider centered content area
- portrait remains near full width
- row geometry now uses consistent 10-key base slots
- second row is naturally centered without arbitrary percentage width
- Shift and Backspace use wider functional-key geometry
- bottom row proportions are closer to a modern mobile keyboard
- original vector icons added for Shift, Caps Lock, Backspace and Enter
- Enter receives an accent treatment
- Space displays the current first language label
- key press animation added
- letter/symbol press popup added

Design note:
- this is Gboard-inspired geometry/interaction, not Google branding or copied assets
- suggestion strip, emoji/language switching and toolbar remain intentionally absent until functional

## 2026-09-24 — Hotfix: QML key-popup anchor

Root cause:
- Stage 16 popup used `bottom: parent.top`
- Rectangle has no standalone `bottom` property
- QML component creation therefore failed and the keyboard process exited before
  input-panel initialization

Fix:
- use the valid anchor property `anchors.bottom: parent.top`

## 2026-09-24 — RED: multi-language layout state on refined tablet UI

Required behavior:
- English QWERTY
- German QWERTZ with Ü/Ö/Ä
- Ukrainian layout with Ї/І/Є
- Russian layout with Ъ/Ы/Э
- deterministic EN -> DE -> UK -> RU cycle
- changing language clears Shift/Caps
- refined tablet geometry and key popup remain untouched

## 2026-09-24 — GREEN: multi-language layouts on refined tablet UI

Implemented:
- English QWERTY
- German QWERTZ with Ü/Ö/Ä
- Ukrainian with Ї/І/Є
- Russian with Ъ/Ы/Э
- globe key cycles EN -> DE -> UK -> RU
- active language is shown on Space
- language rows come from KeyboardModel

Preserved:
- tablet portrait/landscape metrics
- refined key visuals
- corrected key popup anchoring
- KWin/Wayland backend unchanged

Next:
- Gboard-style language chooser popup
- toolbar foundation

## 2026-09-26 — RED: toolbar and primary-panel model foundation

Specified:
- toolbar action IDs must be unique
- default action order is deterministic
- hidden actions are excluded from the visible toolbar model
- disabled actions cannot activate
- language action targets the Language panel
- PanelManager starts in Typing and can return to Typing

Constraints:
- no KWin/Wayland transport changes
- no visible QML toolbar yet
- no dead placeholder buttons exposed

## 2026-09-26 — GREEN: toolbar and primary-panel model foundation

Implemented:
- PanelId + PanelManager
- ToolbarAction + ToolbarActionKind
- ToolbarRegistry with duplicate-ID protection
- ToolbarModel visible-order filtering and activation
- initial registered actions: language, clipboard, emoji, text-editing, settings
- only language is visible at this milestone; unfinished panel actions stay hidden
- KeyboardUiBridge exposes active panel and visible toolbar action IDs

Preserved:
- existing globe/language UI
- existing keyboard QML
- KWin/Wayland transport
- all live typing behavior

Architecture:
- toolbar identity/order/state now lives in C++, not QML
- primary panel state now lives in C++, not QML

## 2026-09-26 — RED: toolbar renderer and language chooser data

Specified:
- ToolbarModel exposes visible action descriptors for the UI
- KeyboardModel exposes supported language codes and labels
- QML must consume those model surfaces instead of duplicating product state

## 2026-09-26 — GREEN: rendered toolbar and language chooser

Implemented:
- toolbar renders C++ ToolbarModel descriptors
- Language is the only visible toolbar action at this milestone
- toolbar Language action opens the Language panel
- bottom globe opens the same Language panel
- chooser renders EN / DE / UK / RU from KeyboardModel data
- selecting a language returns to Typing
- current language remains visible on Space

Preserved:
- existing tablet layout metrics
- key popup behavior
- KWin/Wayland transport
- unfinished toolbar actions remain hidden

## 2026-09-26 — RED: Gboard-aligned language switching UX

Specified:
- Language remains registered as a panel capability but is not a visible toolbar action
- unfinished default toolbar actions stay hidden
- language switching belongs to the bottom-row Globe/Space interaction

## 2026-09-26 — GREEN: compact bottom-row language chooser

Changed:
- Language removed from visible top toolbar
- empty Stage 19 toolbar removed from QML
- tap Globe cycles languages
- long-press Globe opens compact language popup
- long-press Space opens the same popup
- popup overlays the keyboard instead of replacing it
- keyboard stays visible while choosing
- oversized Languages page and ABC close button removed
- popup closes with × or after selection

Preserved:
- ToolbarRegistry / ToolbarModel architecture
- EN / DE / UK / RU layouts
- current language on Space
- tablet geometry and key popups
- KWin/Wayland platform layer

## 2026-09-26 — V3 Keyboard Alpha 0.1 integrated pass

Goal:
- stop shipping isolated UI experiments and produce one cohesive daily-test alpha
- preserve the known-good KWin input-method path while making the keyboard useful enough for real tablet testing

Integrated:
- compact top tool row with working Clipboard, Emoji, Text editing and Settings actions
- Language remains exclusively on the bottom Globe / Space interaction; no duplicated language toolbar control
- compact long-press language chooser for EN / DE / UK / RU
- persisted active language via QSettings
- German ß / ẞ, Ukrainian ґ / Ґ and Russian ё / Ё long-press alternates
- current system clipboard preview, paste and clear
- basic emoji panel
- text-editing panel with Left / Right / Home / End / Delete / Backspace
- persisted Dark / AMOLED appearance
- persisted key-size, key-border and key-popup preferences
- panel state returns to Typing when the input context closes
- tablet portrait / landscape geometry preserved and key-height scaling made live

Still deliberately deferred from Alpha 0.1:
- autocorrect and suggestions
- language model / prediction engine
- glide typing
- clipboard history database
- full emoji catalog/search
- split/floating keyboard modes
- LayoutEngine / KeyboardGeometrySnapshot refactor

## 2026-09-26 — Beta 0.2 smart typing and daily-use completion pass

Added a privacy-first local typing engine on top of the Alpha 0.1 shell.

Core:
- LocalLexicon loads system Hunspell dictionaries from `/usr/share/hunspell` when present
- built-in fallback vocabulary keeps suggestions functional without extra packages
- prefix suggestions and one-edit typo correction
- per-language local word learning and previous-word bigram ranking in QSettings
- TypingEngine tracks the current word while preserving the existing immediate-commit KWin transport
- suggestion replacement, autocorrect-on-space, auto-capitalization, double-space period
- local glide decoder API with dictionary/personal/bigram scoring

UI/UX:
- suggestion strip takes over the top row while typing; tool row remains one tap away
- settings toggles for suggestions, autocorrect, local learning, auto-capitalization, double-space period and glide
- swipe Space horizontally for cursor movement
- long-press/drag Backspace for accelerated deletion
- letter-key glide gesture plumbing into the local decoder

Clipboard / emoji:
- persistent local clipboard history with dedupe, paste, per-item remove and clear-history
- clipboard history can be disabled
- emoji catalog loads Unicode `emoji-test.txt` from the system when available
- curated fallback emoji set when system data is absent
- emoji categories and name search surface

Privacy:
- no network calls
- no telemetry
- no Google services
- learning and clipboard history remain local in QSettings

Still outside the KWin functionality-complete target:
- wlroots backend and GNOME bridge portability work
- true compositor-positioned floating keyboard window
- handwriting and voice input

## 2026-09-26 — Input-context awareness and privacy guard

Added:
- KWin input-method-v1 surrounding-text synchronization
- content hint/purpose propagation into the typing engine
- password/PIN/sensitive-field detection with suggestion and learning suppression
- preferred-language propagation from the client context
- composition reset on client reset/deactivation

This closes the main privacy gap in smart typing: secure fields do not expose candidates or feed local learning.

## 2026-09-26 — Beta 0.2.1 autocorrect determinism fix

Root cause from the first full on-device CTest run:
- `doubleSpaceProducesPeriodSpace` observed 3 backspaces instead of 1.
- `hi` was not guaranteed to exist in the fallback dictionary; on systems without a matching Hunspell entry, one-edit autocorrect rewrote the 2-character token before the second Space.

Fix:
- automatic correction now requires at least 3 characters; 2-character tokens are never automatically rewritten
- added a dedicated regression test for short-token preservation
- keeps `teh -> the` autocorrect and double-space punctuation behavior intact
- makes the test suite independent of which system Hunspell dictionaries happen to be installed

## 2026-09-26 — Beta 0.2.2 live-input repair

Live V3 testing exposed three coupled issues in smart typing:
- stale `surrounding_text` echoes from browser/Qt clients could overwrite the local word currently being typed, breaking autocorrect and suggestion replacement
- normal letter taps emitted `keyboardStateChanged`, invalidating all key-row bindings on every character and causing visible lag in Chromium/ChatGPT fields
- learning serialized its full settings maps twice per committed word

Fixes:
- local composition is authoritative while a local edit is awaiting a matching surrounding-text echo
- stale pre-space and partial-word echoes are ignored instead of clobbering composition
- surrounding-text updates that do not change state no longer trigger UI invalidation
- ordinary taps emit only `suggestionsChanged`; full keyboard invalidation is reserved for real case/layout changes
- `.`, `!`, `?` arm capitalization immediately
- word+bigram learning persists once per finalized word
- repeated backspace updates composition/suggestions once per batch
- regression tests cover stale echoes, direct punctuation capitalization, signal hot-path behavior, and batched backspace

## 2026-09-29 — Beta 0.2.3 root-cause pass (supersedes 0.2.2)

Baseline:
- handoff archive checksums verified; `project/` identical to the 0.2.2 payload
- evidence file named 0.2.1 actually contains the 0.2-final run (10/11)
- sandbox: Qt 6.4.2; KWin panel shell needs Qt >= 6.5, so the executable and
  KWin tests are built on-device (KWin input-method layer verified via overlay)

RED (each observed failing before the fix):
- real en_US Hunspell: `teh -> meh` (3 smarttyping failures on 0.2.2)
- `externalCursorMoveAdoptsClientState`: 0.2.2 kept "hel" after a cursor move
- `autocorrectUsesTextChannelOnceClientConfirmedWord`,
  `unconfirmedWordIsNotRewrittenOnTextInputClients`,
  `suggestionAndDoubleSpaceUseTextChannel`: Backspace keysyms used
- bridge: date purpose treated as secret; url/email capitalized; lowercase hint ignored
- QML: `ReferenceError: root is not defined` on suggestion tap
- lexicon fixture tests: inflections rewritten, no background load

GREEN:
- same-channel replacement via delete_surrounding_text (KWin backend, UTF-8 bytes)
- predicted-state echo reconciliation replaces the 0.2.2 pending flag
- libhunspell-backed lexicon, background load, compact index, batched learning
- purpose/hint mapping per text-input-v1 as sent by KWin
- hermetic tests (fixtures + QStandardPaths test mode)
- install-time dictionary provisioning with pinned UK dictionary

Design note:
- 0.2.2 test `staleSurroundingEchoDoesNotClobberLocalWord` now includes the
  confirming echoes: rewriting an unconfirmed word is deliberately skipped.

Verification (sandbox, Qt 6.4.2): 10/10 CTest + 3/3 KWin tests via overlay,
static verify OK. On-device (Qt 6.11.2) build/test/live test pending.

## 2026-09-29 — Beta 0.2.4 composition (preedit)

Live result of 0.2.3 in Firefox: "TEH hhello hi  job" (13/13 tests on device).
Root cause from Firefox/GTK/KWin sources: surrounding-text echoes are one step
stale by construction (see docs/beta-0.2.4-composition.md).

RED:
- `oneStepStaleEchoesDoNotFlipCaseOrWord` (0.2.3 adopted stale echoes)
- composition tests: autocorrect without deletions, double space, suggestion,
  backspace in preedit, Enter order, secure fields, non-text-input clients
- bridge: commit composition before cursor moves / language switch; toggle
- KWin backend: preedit cursor in bytes + commit fallback (compile-RED)
- QML: fail on "is not declared" (only observable on Qt >= 6.11)

GREEN: composition mode, history-based echo reconciliation, explicit QML
handler parameters, opt-in file trace.

Design decision: immediate-commit replaced by composition on text-input
clients (user: "продолжай" after the A/B proposal); immediate mode kept as a
setting.

Verification (sandbox Qt 6.4.2): 10/10 CTest + 3/3 KWin overlay, static OK.

## 2026-09-29 — Beta 0.2.5 suggestion-space fix

Live result of 0.2.4 in Firefox: "the hello. Hi. Hello" — teh->the and
double-space work; tapping a suggestion then Space produced "hello. ".
Root cause: the space a suggestion inserts was flagged as a user Space
(m_lastActionWasSpace), so the next Space triggered the double-space period.
RED: spaceAfterSuggestionIsNotADoubleSpace; 0.2.3 test
suggestionAndDoubleSpaceUseTextChannel encoded the same wrong expectation and
was corrected. GREEN: the first Space after a suggestion only confirms its
space; a further Space makes ". ". Sandbox: all suites green.

## 2026-09-29 — Beta 0.2.6 echo handling from device trace

Live result of 0.2.5: "Slovoi  drugoe okno" after word + Space + window switch.
Trace (privacy-safe, 1133 lines) showed:
  31.265 COMMIT 5 + PRE 1 (pending space)
  31.268 echo 5 bytes -> ADOPT, composition dropped (client still showed it)
  31.541 COMMIT 1 ("i"): KWin's commit clears the preedit -> space lost
Cause: claude.ai's empty composer reports a 1-byte placeholder that vanishes
once text exists, so echo contents never match the keyboard's model.
Measured first-echo latency after own operations: p50 2 ms, p99 6 ms,
max 36 ms (78 samples). When the user clicked "send" mid-word, Firefox
committed the composition itself and KWin sent reset (message arrived intact).
RED: placeholderEchoDuringCompositionKeepsPendingSpace (reproduces "Slovoi"),
selfCausedEchoesAreIgnoredButLaterExternalEditsAdopted.
GREEN: echoes never abandon a live preedit (reset ends it); unknown echoes
within 150 ms of an own operation are self-caused; later ones are adopted.
Design note: externalCursorMoveAdoptsClientState now advances an injected
clock (external edits happen at human speed).
Also live-verified on 0.2.5: teh->the, suggestion without period, double-space
period, Enter keeps last word, translit untouched, email/url no auto-cap,
normal field auto-cap, window switch keeps the word.
Regression caught before release: with the settle window, the echo after a
keyboard cursor move (2-6 ms after committing the composition) was ignored,
leaving stale context (bridge test cursorMovesAndLanguageSwitch... RED: "N"
instead of "n"). Fix: forgetting the text state (cursor move, paste, client
reset) disables the settle window for the next echo. Process note: the first
0.2.6 commit was made while that test was red; it was amended after GREEN.

## 2026-09-29 — Beta 0.2.7 v0.2 completion slice

User: "finish the project, then we test". Scope = make the approved v0.2
milestone solid on the tablet; deferred spec items untouched.
RED (observed before each fix):
- overlappingTwoThumbTapsAreBothCommitted: 1 of 2 letters (MouseArea sees only
  the first touch); after switching to MultiPointTouchArea: 0 of 2 — the glide
  candidate treated two fingers as a swipe; fixed by binding glide to its
  finger. singleFingerGlideAndLongPressStillWork guards the gestures (test
  itself needed a fix: stale EN delegate lookup after a language switch).
- compositionCommitsCorrectedWordWithoutDeletions (updated design),
  backspaceRightAfterAutocorrectRevertsAndRemembers,
  correctionThenDoubleSpaceOrEnterOrPunctuation; revert exposed that the
  curated typo list overrode learned words (learnedWordsBeatTheCuratedTypoList).
- glidedWordBehavesLikeASuggestion.
- Context destroy after deactivate: compile-verified only (no compositor in
  the sandbox); exercised on device by every focus change.
Sandbox: 10/10 CTest + 3/3 KWin overlay, static verify OK.

## 2026-09-29 — 0.3.0 (user: "do the whole project, then we test")

Frequency data: license verified (FrequencyWords content CC BY-SA 4.0; CC's
2015 declaration of one-way compatibility with GPLv3). CLDR: Unicode License V3.
RED: frequencyRanksCompletionsFromTheFirstLetter (core-list bonus outranked real
frequencies -> core prior only without frequency data),
germanNounsFromFrequencyKeepTheirCapital, emojiSearchUnderstandsTheKeyboardLanguage
(ranking tiers), compactModeDocksTheKeysLeftOrRight (maxContentWidth cap; the
first version of the test passed vacuously because labels were uppercase —
made strict: keys must be found, span > 300 px, root follows the view).
Real-data bench (sandbox): suggestions 0.02–0.04 ms/keystroke after rank-first
selection (3–8 ms before); background load EN 0.3 s, RU 0.5 s, DE/UK 1.3–1.5 s.
Sandbox: 10/10 CTest + 3/3 KWin overlay, static verify OK.

## 2026-09-29 — 0.4.0 offline voice input (user: "davai dalshe" after the proposal)

Sources verified: Arch extra whisper-cpp 1.9.1 (MIT); model ggml-base-q5_1.bin
SHA-256 from the official Hugging Face LFS metadata. Built whisper.cpp v1.9.1
in the sandbox (exports CMake config + whisper.pc; pkg-config fallback added).
RED/GREEN: tst_voice (state machine with fake recorder/recognizer),
dictationCommitsPendingWordThenSentenceCasedText (engine-level expectation
corrected: capitalizing typed letters is the bridge's job),
dictationFlowsFromVoiceIntoTheField (bridge), micButtonAppearsOnlyWithVoice...
(UI; first failure was the test clicking before the Row polished its layout),
tst_voiceadapters (PCM conversion, missing model, real whisper run with the
repository test model). Both builds green: voice ON 12/12, voice OFF 11/11.

## 2026-09-29 — 0.5.0 Gboard behaviours (user: "continue, take the logic from Gboard")

Gboard behaviours taken from public descriptions (Computerworld, Android
Authority, MakeTechEasier, Gboard store listing), not code.
Licenses: LDNOOBW CC BY 4.0 (FSF: compatible with all GPL versions).
Evidence for the offensive filter: 20 EN list words within the top 3000 of the
frequency data (ranks 277, 291, 329, ...), DE 9, RU 3.
RED->GREEN: offensiveWordsAreNeverSuggestedButStayTypable,
forgettingASuggestionRemovesItForGood, exactKeywordMapsAWordToItsEmoji,
emojiSuggestionFollowsAnExactWordAndInsertsAfterIt,
longPressAlternatesFollowGboardPositions (compile-RED).
The UI test gboardLongPressPickerNumberRowHintsAndForget was written after the
QML (not observed RED); its assertions are specific (commit "è", hint "1"
disappears, panel height grows, commit "7", no commit on suggestion forget).
A scripted edit aborted on a stale line number (no file written); redone with
content-based positioning.
Sandbox: voice ON 12/12, voice OFF 11/11, KWin 3/3, static verify OK.

## 2026-10-03 — Neighbour-key correction (LatinIME ProximityInfo idea)

Found uncommitted work in the tree (13:22–13:27, not visible in my current
context; most likely an interrupted earlier turn): LatinIME-style normalized
scoring, a re-scaled frequency format and proximity. Its own tests passed, but
the real-data bench regressed badly (sch -> ich/ach/sich, ha -> ja/da/hi,
дя -> до/де/ця, привте -> приюте, wiel -> will). Preserved untouched on branch
wip/latinime-scoring; not shipped.
Re-implemented on the verified 0.5.0 ranking: key centres from the layout
rows; a substitution by a neighbouring key (<= 1.5 key widths) gets the same
bonus as a repeated letter. The two spec tests from the WIP were kept
(neighbouringKeysMakeTheCheapestCorrection, latinImeThresholdGatesAutocorrection).
Acceptance bench (real dictionaries + frequency lists): no regression vs 0.5.0;
new fixes tge/thw/yhe->the, hsus->haus, nivht->nicht, пртвет->привет,
сппсибо->спасибо. Behaviour change: "helo" is no longer auto-corrected (hello
and help are now equally likely: o neighbours p); both are suggested.

## 2026-10-03 — 0.6.0 more Gboard logic (user: "logic and functions from Gboard")

Declined decompiling the Gboard APK (proprietary; terms forbid reverse
engineering; its code/data cannot enter a GPL project). Used Gboard's public
feature descriptions and AOSP LatinIME (Apache-2.0) ideas instead.
RED->GREEN: stripShowsTypedWordAndWhatSpaceWillInsert, fieldTypeDrivesLayoutLikeGboard,
textShortcutsExpandFromTheStrip, recentEmojisComeFirstAndPersist (compile-RED);
UI tests stripQuotesTypedWordAndBoldsTheCorrection, numberFieldsShowANumpadEmailFieldsAnAt,
gesture-trail assertions in the glide test (written with the QML, not observed RED).
Process: one test edit referenced a non-existent fake counter; the "15 passed"
seen at that moment was a stale binary — re-run after the fix: 16/16.
Sandbox: voice ON 12/12, voice OFF 11/11, KWin 3/3, static verify OK.

## 2026-10-03 — 0.7.0 Gboard gestures and shortcuts

Sources: Computerworld (fast symbols from ?123, Shift slide, emoji fast-access
row), 9to5Google (auto-space after punctuation off by default; apostrophe
auto-switch on by default since 16.7), HelpDeskGeek/Hongkiat (16 period marks).
RED->GREEN: autoSpaceAfterPunctuationIsOptInLikeGboard (compile-RED),
apostropheOnSymbolsReturnsToLetters (a first version asserted period choices
while the symbols layer was active — test bug, fixed). UI test
gboardSlideGesturesAndEmojiRow written with the QML; review found its emoji-row
assertion vacuous (insertEmoji had already committed the glyph) — fixed to
assert the click's own commit. A QML property typo (glyphValue vs glideValue)
was caught by reading the diff before running.
Sandbox: voice ON 12/12, voice OFF 11/11, KWin 3/3, static verify OK.

## 2026-10-03 — 1.0.0 (user: "push to 1.0, then test")

1.0 = ready for daily use rather than new features:
- Themes: added System/Light (Gboard-like Material palette) next to Dark and
  AMOLED; ~10 hard-coded colours moved onto the palette; pre-1.0 "amoled"
  setting migrated. System follows QStyleHints::colorScheme (Qt >= 6.5; Qt 6.4
  falls back to dark).
- Found: the repository had no license text despite SPDX headers; added the
  official GPL-3.0-or-later text from SPDX's license-list-data (COPYING).
- About block in Settings; docs + attributions installed; Release build.
- v3kbd-rollback / v3kbd-uninstall [--purge], exercised against a fake home.
- Settings panel was checked for overflow: it already scrolls (Flickable).
RED->GREEN: themesFollowGboardIncludingSystem (compile-RED), palette/about UI
test. A scripted QML edit aborted on a wrong occurrence count (no file
written); redone with occurrence-agnostic replacement scoped to the body.
Process note: the 1.0 commit was made before the final full run; the run afterwards (Release, voice ON 12/12, OFF 11/11, KWin 3/3, static OK) confirmed it.

## 2026-10-03 — 1.0.1 hardening (user: "continue" before the 1.0 live test)

No new features; verification that does not need the tablet:
- Randomised stress test (240 sequences x 250 ops, immediate and composition
  modes, none/fresh/one-step-stale echoes): field text must equal typed text.
  Found: in composition mode "word Space ," dropped the space ("word,") and the
  next word glued on. LatinIME/Gboard swap it ("word, "). Fixed (swap only for
  a space typed right after a word; the next Space is absorbed). A unit test
  from 0.2.7 encoded the old drop and was corrected.
- ASan+UBSan: 13/13 suites clean (incl. a real whisper.cpp run); KWin layer
  clean. TSan: only Qt-internal QWaitCondition reports at process exit
  (uninstrumented libQt6Core), no frames in our code.
- Latency through the full bridge on real dictionaries (1 vCPU sandbox):
  max per keystroke EN 14.5 / DE 19.5 / RU 55.9 / UK 46.2 ms -> 2.5 / 3.7 /
  4.4 / 4.3 ms. Root cause (located by slowest-keystroke context, after two
  refuted hypotheses: disk flush, background-load contention): since 0.6.0 the
  per-keystroke autocorrect target enumerated ~700 variants through Hunspell
  for long unknown words (expensive for invalid words, 5630 SFX rules in uk).
  Fix: per-keystroke preview checks variants against frequency/stems/learned
  /core only; Space keeps Hunspell, with likelihood-ordered variants and a
  250-call budget (capital retry only for German). Space on long unknown
  words: UK 17 -> 3.7, RU 8.9 -> 3.0, EN 7.3 -> 2.6, DE 122 -> 23 ms.
  Real-data quality probes unchanged. Benchmark: tools/bench_keystroke.cpp
  (-DV3KBD_BENCHMARKS=ON).

## 2026-10-03 — 1.1.0 personal dictionary (user: "proper logic for adding custom words")

Problem in the existing logic: any word committed once counted as learned and
was then treated as valid forever (never corrected, offered as a suggestion),
so a single accidental typo was legitimised.
New logic (LatinIME/Gboard model):
- explicit personal dictionary per language (case kept, always valid,
  suggested with a high prior, persisted in settings);
- implicit learning promotes an unknown word only after 3 uses; known words
  are still learned for ranking from the first use;
- undoing an autocorrection promotes the word (explicit signal);
- strip shows an unknown typed word in quotes; tapping it keeps it and offers
  "+ Add to dictionary" (LatinIME "touch again to save");
- ~/.config/v3-keyboard/dictionary.txt adds words for all languages,
  re-read on every field focus;
- forgetting (long-press a suggestion) also removes the explicit entry.
RED->GREEN: oneAccidentalCommitDoesNotLegitimiseATypo, personalDictionaryKeepsCaseAndPersists,
dictionaryFileAddsWordsForAllLanguages, forgettingAlsoRemovesFromThePersonalDictionary,
acceptedWordsBeatTheCuratedTypoList (replaces learnedWordsBeatTheCuratedTypoList,
which encoded the old "one commit protects" rule), engine test
backspaceRightAfterAutocorrectRevertsAndRemembers went RED under the new rule
and GREEN once undo promotes; bridge tappingAnUnknownTypedWordOffersToSaveIt;
UI savingAndRemovingPersonalWordsThroughTheUi (first failure was the test
clicking a chip outside the scrolled settings viewport).

## 2026-10-03 — 1.1.1 first live test of 1.1.0 (Firefox, claude.ai)

Reports and findings:
- Top-row key previews were cut off (screenshot: a sliver above "г"):
  keyboardRows had z 0 under the toolbar (z 120). Raised to 130.
- Suggestions ignored the typed case ("Клево" -> suggestion "клево" would
  lowercase the sentence start). Now follow initial capital / ALL CAPS.
- "teh -> the doesn't work": user data from <= 1.0.1 counted every commit
  (typos typed many times in tests) and now crossed the promotion threshold;
  also the 1.1.0 checklist itself asked to undo the correction (an explicit
  keep). One-time migration (learning schema 2): legacy counts capped below
  the threshold, curated typos dropped.
- "word registration does not work": the flow passes in both immediate and
  composition mode tests; known words (e.g. "клево") intentionally get no
  "+ Add" chip. Asked the user for the word.
- "first letters uppercase / second letter replaces the first" in Firefox:
  not reproducible from code alone; two candidate mechanisms around the empty
  composer re-render. Requested a trace (no fix without evidence).
- "•••" in the strip is the existing toolbar toggle, not a bug.
RED->GREEN: suggestionsFollowTheCaseOfTheTypedWord, legacyLearningDoesNotPromoteOldTypos,
topRowKeyPreviewIsDrawnAboveTheToolbar; tappingSuggestionReplacesTheTypedWord
updated to the case-following strip ("Hello").
