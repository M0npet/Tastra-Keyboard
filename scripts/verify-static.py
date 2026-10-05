#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

def require(path, token, description=None):
    text = (ROOT / path).read_text(encoding='utf-8')
    if token not in text:
        errors.append(f"{path}: missing {description or token!r}")

def forbid(path, token, description=None):
    text = (ROOT / path).read_text(encoding='utf-8')
    if token in text:
        errors.append(f"{path}: forbidden {description or token!r}")

# Architecture / model checks.
require('src/core/toolbarregistry.h', 'language.visible = false;', 'hidden toolbar language action')
for action in ('clipboard', 'emoji', 'textEditing', 'settings'):
    require('src/core/toolbarregistry.h', f'{action}.visible = true;', f'visible {action} action')

require('src/core/keyboardmodel.cpp', 'QString KeyboardModel::alternateForKey', 'long-press alternate model')
require('src/core/keyboardmodel.cpp', 'QStringLiteral("ß")')
require('src/core/keyboardmodel.cpp', 'QStringLiteral("ґ")')
require('src/core/keyboardmodel.cpp', 'QStringLiteral("ё")')

for method in ('deleteForward', 'moveLeft', 'moveRight', 'moveHome', 'moveEnd'):
    require('src/core/inputmethodbackend.h', f'virtual void {method}() = 0;', f'backend {method}')
    require('src/core/keyboardcontroller.cpp', f'void KeyboardController::{method}()', f'controller {method}')
    require('src/platform/kwin/kwininputmethodv1backend.cpp', f'void KWinInputMethodV1Backend::{method}()', f'KWin {method}')

# Bridge/persistence checks.
for token in (
    'Q_PROPERTY(bool amoled',
    'Q_PROPERTY(double keyScale',
    'Q_PROPERTY(bool keyBorders',
    'Q_PROPERTY(bool keyPopups',
    'Q_PROPERTY(QString clipboardText',
    'Q_INVOKABLE void pasteClipboard()',
    'Q_INVOKABLE void clearClipboard()',
    'Q_INVOKABLE void tapAlternate',
):
    require('src/app/keyboarduibridge.h', token)

require('src/app/keyboarduibridge.cpp', 'QSettings settings;', 'settings persistence')
require('src/app/keyboarduibridge.cpp', 'QGuiApplication::clipboard()', 'system clipboard access')
require('src/app/main.cpp', 'bridge.resetInputContext();', 'typing/panel reset on context close')
require('CMakeLists.txt', 'Qt6::Gui', 'Qt GUI dependency for clipboard')


# Smart typing / local privacy-first engine checks.
for path, token in (
    ('src/core/locallexicon.cpp', 'LocalLexicon::bestCorrection'),
    ('src/core/locallexicon.cpp', '/usr/share/hunspell'),
    ('src/core/typingengine.cpp', 'TypingEngine::chooseSuggestion'),
    ('src/core/typingengine.cpp', 'TypingEngine::endGlide'),
    ('src/core/clipboardhistory.cpp', 'ClipboardHistory::capture'),
    ('src/core/emojicatalog.cpp', 'emoji-test.txt'),
):
    require(path, token)

for token in (
    'Q_PROPERTY(QStringList suggestions',
    'Q_PROPERTY(bool autocorrectEnabled',
    'Q_PROPERTY(bool learningEnabled',
    'Q_PROPERTY(bool glideEnabled',
    'Q_PROPERTY(QStringList clipboardHistory',
    'Q_INVOKABLE void selectSuggestion',
    'Q_INVOKABLE QString endGlide',
):
    require('src/app/keyboarduibridge.h', token)

require('tests/tst_smarttyping.cpp', 'autocorrectReplacesCommittedWordBeforeSpace')
require('tests/tst_smarttyping.cpp', 'shortTokensAreNotAggressivelyAutocorrected')
require('src/core/locallexicon.cpp', 'typed.size() < 3', 'conservative short-token autocorrect guard')
require('tests/tst_smarttyping.cpp', 'glideDecoderCanResolveSimpleTrace')
require('tests/tst_clipboardemoji.cpp', 'clipboardHistoryDeduplicatesAndKeepsNewestFirst')

# UI checks.
qml_path = ROOT / 'src/ui/Main.qml'
qml = qml_path.read_text(encoding='utf-8')
for token in (
    'id: topToolbar',
    'model: keyboardBridge.toolbarActions',
    'id: languageChooserPanel',
    'onLongPressed: keyboardBridge.openLanguagePanel()',
    'keyboardBridge.alternateForKey(modelData)',
    'keyboardBridge.tapAlternateText(alternates[0])',
    'keyboardBridge.clipboardText',
    'keyboardBridge.emojiSearch',
    'keyboardBridge.suggestions',
    'keyboardBridge.selectSuggestion',
    'keyboardBridge.clipboardHistory',
    'keyboardBridge.backspaceRepeated',
    'keyboardBridge.moveCursor',
    'keyboardBridge.moveLeft()',
    'keyboardBridge.deleteForward()',
    'keyboardBridge.cycleTheme()',
    'keyboardBridge.setKeyScale',
    'keyboardBridge.setKeyBorders',
    'keyboardBridge.setKeyPopups',
):
    if token not in qml:
        errors.append(f'src/ui/Main.qml: missing {token!r}')

if 'Language' in re.sub(r'Keyboard language', '', qml) and 'languageChooserPanel' not in qml:
    errors.append('src/ui/Main.qml: language UI unexpectedly missing')

# Cheap syntax sanity: braces outside strings/comments must balance.
def balance_braces(text: str):
    depth = 0
    i = 0
    in_str = None
    line_comment = False
    block_comment = False
    while i < len(text):
        c = text[i]
        n = text[i+1] if i + 1 < len(text) else ''
        if line_comment:
            if c == '\n':
                line_comment = False
            i += 1
            continue
        if block_comment:
            if c == '*' and n == '/':
                block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_str:
            if c == '\\':
                i += 2
                continue
            if c == in_str:
                in_str = None
            i += 1
            continue
        if c == '/' and n == '/':
            line_comment = True
            i += 2
            continue
        if c == '/' and n == '*':
            block_comment = True
            i += 2
            continue
        if c in ('"', "'"):
            in_str = c
        elif c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth < 0:
                return False, depth
        i += 1
    return depth == 0 and not in_str and not block_comment, depth

ok, depth = balance_braces(qml)
if not ok:
    errors.append(f'src/ui/Main.qml: brace/string sanity failed (depth={depth})')

# Test intent checks.
require('tests/tst_keyboardmodel.cpp', 'exposesLanguageSpecificLongPressAlternates')
require('tests/tst_keyboardmodel.cpp', 'QStringLiteral("clipboard")')
require('tests/tst_specialkeys.cpp', 'controllerForwardsEditingKeys')
require('tests/tst_keyboarduibridge.cpp', 'editingCommandsReachController')

# Secure/input-context integration checks.
for _file, _marker in [
    ("src/app/keyboarduibridge.h", "Q_PROPERTY(bool secureInput"),
    ("src/app/keyboarduibridge.cpp", "setSurroundingText"),
    ("src/app/keyboarduibridge.cpp", "setContentType"),
    ("src/platform/kwin/kwininputmethodv1connection.h", "surroundingTextChanged"),
    ("src/platform/kwin/kwininputmethodv1connection.h", "contentTypeChanged"),
    ("src/app/main.cpp", "preferredLanguageChanged"),
]:
    if _marker not in (ROOT / _file).read_text(encoding="utf-8"):
        errors.append(f"secure/input-context marker missing: {_file}: {_marker}")


# Beta 0.2.2 live-input repair checks.
require('src/core/typingengine.h', 'bool syncSurroundingText', 'state-aware surrounding-text sync')
require('src/core/typingengine.cpp', 'm_history', 'history-based stale echo reconciliation (0.2.4 replaces 0.2.3 predicted states)')
require('src/core/typingengine.cpp', 'void TypingEngine::backspaceRepeated', 'batched repeated backspace')
require('src/core/locallexicon.cpp', 'learnWordWithContext', 'single-persist contextual learning')
require('tests/tst_smarttyping.cpp', 'staleSurroundingEchoDoesNotClobberLocalWord')
require('tests/tst_smarttyping.cpp', 'terminalPunctuationImmediatelyArmsCapitalization')
require('tests/tst_smarttyping.cpp', 'repeatedBackspaceUpdatesCompositionInOneOperation')
require('tests/tst_keyboarduibridge.cpp', 'ordinaryTypingDoesNotInvalidateWholeKeyboard')

# Beta 0.2.3 root-cause fixes.
for path, token, why in (
    ('src/core/inputmethodbackend.h', 'virtual bool deleteBeforeCursor', 'same-channel deletion seam'),
    ('src/platform/kwin/kwininputmethodv1backend.cpp', 'deleteSurroundingText(-static_cast<qint32>(bytes), bytes)', 'delete_surrounding_text in UTF-8 bytes'),
    ('src/core/typingengine.cpp', 'bool TypingEngine::replaceBeforeCursor', 'replacement via text channel'),
    ('src/core/locallexicon.cpp', '#include <hunspell/hunspell.hxx>', 'affix-aware validity via libhunspell'),
    ('src/core/locallexicon.cpp', 'std::thread(', 'background dictionary loading'),
    ('src/core/locallexicon.cpp', 'if (!hasSystemDictionary()) return {};', 'no autocorrect without a validity oracle'),
    ('src/app/keyboarduibridge.cpp', 'Date = 9', 'text-input-v1 purpose enum (9 is date, not PIN)'),
    ('src/ui/Main.qml', 'root.toolbarExpanded = false\n                                keyboardBridge.selectSuggestion(modelData)', 'no access after delegate self-destruction'),
    ('tests/tst_smarttyping.cpp', 'externalCursorMoveAdoptsClientState', 'external edit regression'),
    ('tests/tst_smarttyping.cpp', 'autocorrectUsesTextChannelOnceClientConfirmedWord', 'text-channel replacement'),
    ('tests/tst_lexicon.cpp', 'inflectedFormsAreValidAndNeverRewritten', 'no rewriting of valid inflections'),
    ('tests/tst_qmlkeyboard.cpp', 'tappingSuggestionReplacesTheTypedWord', 'UI-level suggestion tap'),
    ('tests/tst_keyboarduibridge.cpp', 'dateFieldIsNotTreatedAsSecret', 'purpose mapping'),
    ('scripts/v3kbd-dictionaries.sh', 'sha256sum', 'pinned dictionary provisioning'),
):
    require(path, token, why)
forbid('src/app/keyboarduibridge.cpp', 'pinPurpose = 9', 'wrong PIN purpose value')

# Beta 0.2.4: composition (preedit) + history-based echo reconciliation.
for path, token, why in (
    ('src/core/inputmethodbackend.h', 'virtual bool setPreedit', 'preedit seam'),
    ('src/platform/kwin/kwininputmethodv1backend.cpp', 'm_context->preeditString(m_latestSerial, text, text)', 'preedit with commit fallback for KWin commitPendingText'),
    ('src/core/typingengine.cpp', 'm_pendingSpace', 'double-space without deletion'),
    ('src/core/typingengine.cpp', 'm_history', 'stale echoes of older states are never adopted'),
    ('src/core/typingengine.cpp', '!m_sensitiveContext && !m_preeditRejected', 'no preedit in secret fields'),
    ('src/app/tracelog.cpp', 'trace.enable', 'opt-in file trace'),
    ('tests/tst_smarttyping.cpp', 'oneStepStaleEchoesDoNotFlipCaseOrWord', 'Firefox stale-echo regression'),
    ('tests/tst_smarttyping.cpp', 'compositionCommitsCorrectedWordWithoutDeletions', 'composition autocorrect'),
    ('tests/tst_qmlkeyboard.cpp', 'is not declared', 'no implicit handler parameters'),
    ('src/core/typingengine.cpp', 'm_spaceFromSuggestion', 'Space after a suggestion is not a double-space (0.2.5)'),
    ('tests/tst_smarttyping.cpp', 'spaceAfterSuggestionIsNotADoubleSpace', '0.2.5 live regression'),
    ('src/core/typingengine.cpp', 'ignored while composing', 'echoes never abandon a live preedit (0.2.6)'),
    ('src/core/typingengine.cpp', 'constexpr qint64 SettleMs = 150;', 'settle window sized from device trace'),
    ('tests/tst_smarttyping.cpp', 'placeholderEchoDuringCompositionKeepsPendingSpace', '0.2.6 trace regression'),
    ('src/ui/Main.qml', 'maximumTouchPoints: 1', 'per-key touch handling for two-thumb typing (0.2.7)'),
    ('src/ui/Main.qml', 'if (item !== glideStartItem) return', 'glide bound to the finger that started it'),
    ('tests/tst_qmlkeyboard.cpp', 'overlappingTwoThumbTapsAreBothCommitted', 'two-thumb regression'),
    ('src/core/typingengine.cpp', 'm_pendingOriginal', 'revertible autocorrection'),
    ('tests/tst_smarttyping.cpp', 'backspaceRightAfterAutocorrectRevertsAndRemembers', 'undo autocorrect'),
    ('src/platform/kwin/kwininputmethodv1connection.cpp', '~ProtocolInputMethodV1Context() override', 'contexts destroyed after deactivate'),
    ('data/frequency/ATTRIBUTION.md', 'CC BY-SA 4.0', 'frequency data attribution (0.3.0)'),
    ('data/emoji/ATTRIBUTION.md', 'Unicode License V3', 'CLDR attribution'),
    ('src/core/locallexicon.cpp', 'std::partial_sort', 'rank-first completion selection'),
    ('src/core/emojicatalog.cpp', 'void EmojiCatalog::setKeywordLanguage', 'localized emoji search'),
    ('src/app/keyboarduibridge.cpp', 'void KeyboardUiBridge::setLayoutMode', 'compact layout mode'),
    ('tests/tst_lexicon.cpp', 'germanNounsFromFrequencyKeepTheirCapital', 'frequency + speller capitals'),
    ('tests/tst_qmlkeyboard.cpp', 'compactModeDocksTheKeysLeftOrRight', 'compact layout'),
    ('src/app/voicecontroller.cpp', 'MinimumSamples', 'voice controller (0.4.0)'),
    ('src/voice/whisperrecognizer.cpp', 'm_idleTimer.setInterval(60000)', 'model released when idle'),
    ('src/voice/qtaudiorecorder.cpp', 'MaxSeconds = 30', 'bounded in-memory recording'),
    ('scripts/v3kbd-voice-setup.sh', '422f1ae452ade6f30a004d7e5c6a43195e4433bc370bf23fac9cc591f01a8898', 'pinned model checksum'),
    ('src/app/keyboarduibridge.cpp', 'if (m_voice) m_voice->cancel();', 'dictation never lands in a new field'),
    ('tests/tst_voice.cpp', 'emptyResultsAndCancelledRecognitionInsertNothing', 'voice state machine'),
    ('data/blocklist/ATTRIBUTION.md', 'CC BY 4.0', 'offensive-word list attribution (0.5.0)'),
    ('src/core/locallexicon.cpp', 'bool LocalLexicon::suggestible', 'offensive/forgotten words never suggested'),
    ('src/core/keyboardmodel.cpp', 'QStringList KeyboardModel::alternatesForKey', 'Gboard long-press alternates'),
    ('src/ui/Main.qml', 'id: choicePicker', 'slide-to-choose picker'),
    ('src/ui/Main.qml', 'onPressAndHold: {', 'long-press suggestion to remove'),
    ('src/core/emojicatalog.cpp', 'QString EmojiCatalog::emojiForWord', 'emoji suggestions'),
    ('tests/tst_qmlkeyboard.cpp', 'gboardLongPressPickerNumberRowHintsAndForget', 'Gboard behaviours UI test'),
    ('src/core/locallexicon.cpp', 'bool LocalLexicon::neighbours', 'neighbour-key corrections (0.6.0)'),
    ('src/core/typingengine.cpp', 'QString TypingEngine::autocorrectTarget', 'strip shows what Space inserts'),
    ('src/app/keyboarduibridge.cpp', 'void KeyboardUiBridge::loadShortcuts', 'text shortcuts'),
    ('src/core/emojicatalog.cpp', 'void EmojiCatalog::noteUsed', 'recent emojis'),
    ('src/ui/Main.qml', 'numpadShown', 'number pad in number/phone fields'),
    ('src/ui/Main.qml', 'id: glideTrail', 'gesture trail'),
    ('tests/tst_qmlkeyboard.cpp', 'numberFieldsShowANumpadEmailFieldsAnAt', 'field-type layouts'),
    ('src/core/typingengine.cpp', 'm_autoSpacePending', 'auto-space after punctuation (0.7.0)'),
    ('src/app/keyboarduibridge.cpp', 'text == QStringLiteral("\'") && m_model.symbolsActive()', 'apostrophe returns to letters'),
    ('src/ui/Main.qml', 'function keyAt(x, y)', 'slide gestures hit-test'),
    ('src/ui/Main.qml', 'objectName: "emojiRowKey_" + modelData', 'emoji fast-access row'),
    ('tests/tst_qmlkeyboard.cpp', 'gboardSlideGesturesAndEmojiRow', 'slide gestures UI test'),
    ('COPYING', 'GNU GENERAL PUBLIC LICENSE', 'license text shipped (1.0)'),
    ('src/app/keyboarduibridge.cpp', 'QString KeyboardUiBridge::effectiveTheme', 'system/light/dark/AMOLED themes'),
    ('src/ui/Main.qml', 'objectName: "aboutText"', 'about: version + attributions'),
    ('tests/tst_qmlkeyboard.cpp', 'lightDarkAndAmoledPalettes', 'theme palettes UI test'),
    ('tests/tst_typingstress.cpp', 'typedTextSurvivesRandomSequences', 'randomised no-lost-letters stress test (1.0.1)'),
    ('src/core/typingengine.cpp', 'm_pendingSpaceAfterWord', 'LatinIME space/punctuation swap'),
    ('src/core/locallexicon.cpp', 'QString LocalLexicon::correctionPreview', 'cheap per-keystroke correction preview'),
    ('src/core/locallexicon.cpp', 'int hunspellBudget = 250;', 'bounded Hunspell work at Space'),
    ('src/core/locallexicon.cpp', 'bool LocalLexicon::isUserWord', 'personal dictionary + promotion threshold (1.1.0)'),
    ('src/core/typingengine.cpp', 'm_lexicon.promoteWord(original);', 'undoing autocorrect promotes the word'),
    ('src/ui/Main.qml', 'objectName: "saveWordChip"', 'touch-again-to-save chip'),
    ('tests/tst_lexicon.cpp', 'oneAccidentalCommitDoesNotLegitimiseATypo', 'typos are not learned from one commit'),
    ('tests/tst_qmlkeyboard.cpp', 'savingAndRemovingPersonalWordsThroughTheUi', 'personal dictionary UI test'),
    ('src/ui/Main.qml', '        z: 130', 'key rows above the toolbar (top-row previews visible, 1.1.1)'),
    ('src/core/locallexicon.cpp', 'learning/%1/schema', 'legacy learning migration'),
    ('src/core/typingengine.cpp', 'suggestions follow the case of what was typed', 'case-following suggestions'),
    ('tests/tst_lexicon.cpp', 'legacyLearningDoesNotPromoteOldTypos', 'migration regression'),
    ('src/ui/Main.qml', 'id: backspaceRepeat', 'Backspace auto-repeat (1.1.2)'),
    ('src/core/typingengine.cpp', 'void TypingEngine::rearmSentenceStartFromText', 'capitalisation after clearing the field'),
    ('tests/tst_qmlkeyboard.cpp', 'holdingBackspaceKeepsDeletingUntilRelease', 'Backspace hold UI test'),
    ('src/core/typingengine.cpp', 'bool TypingEngine::doubleSpaceAllowedAfter', 'LatinIME double-space rules (1.1.3)'),
    ('tests/tst_smarttyping.cpp', 'doubleSpacePeriodFollowsLatinIME', 'LatinIME double-space test'),
    ('src/ui/Main.qml', 'objectName: "keyPreview"', 'key preview clamped inside the panel'),
    ('tests/tst_qmlkeyboard.cpp', 'topRowKeyPreviewStaysInsideThePanel', 'geometric preview test'),
    ('src/ui/Main.qml', 'objectName: "settingsScrollIndicator"', 'settings scroll indicator'),
    ('src/core/locallexicon.cpp', 'int LocalLexicon::neighbourBonus', 'touch-aware neighbour correction (1.2.0)'),
    ('src/ui/Main.qml', 'keyboardBridge.tapLetterAt(modelData', 'touch point passed from the keys'),
    ('tests/tst_qmlkeyboard.cpp', 'touchPointInsideTheKeyReachesCorrection', 'touch point end-to-end test'),
    ('src/core/typingengine.cpp', 'bool TypingEngine::atEmptyParagraph', 'empty-paragraph first letter (1.2.1)'),
    ('tests/tst_smarttyping.cpp', 'emptyParagraphCommitsTheFirstLetterBeforeComposing', 'empty-paragraph test'),
    ('src/app/panelvisibility.cpp', 'PanelVisibility::setActive', 'panel hide grace time'),
    ('tests/tst_panelvisibility.cpp', 'showsAtOnceAndRidesOutAShortDeactivation', 'panel grace-time test'),
    ('src/core/capsmode.cpp', 'bool sentenceCaps(', 'LatinIME capitalisation port (1.3.0)'),
    ('tests/tst_capsmode.cpp', 'englishFollowsLatinIME', 'LatinIME caps test vectors'),
    ('src/app/keyboardhider.cpp', 'org.kde.kwin.VirtualKeyboard', 'hide keyboard via KWin D-Bus'),
    ('src/ui/Main.qml', 'objectName: "hideKeyboardButton"', 'Gboard hide button'),
):
    require(path, token, why)
forbid('src/ui/Main.qml', 'onPressStarted: root.beginGlideCandidate', 'implicit signal parameters (deprecated in Qt 6.11)')

if errors:
    print('STATIC VERIFY: FAILED')
    for error in errors:
        print(f' - {error}')
    sys.exit(1)

print('STATIC VERIFY: OK')
print('v1.3.0 markers present: LatinIME caps, hide button, empty paragraph, panel grace, touch-aware correction, LatinIME double space, preview clamp, backspace repeat, live-test fixes, personal dictionary, stress-tested, latency-bounded, themes, about, license, slide gestures, auto-space, emoji row, proximity, Gboard strip/fields/shortcuts/trail, Gboard behaviours, offline voice, composition, echo handling, two-thumb input, undoable autocorrect, frequency ranking, localized emoji, compact layout; QML braces balanced.')
