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
