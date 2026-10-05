// SPDX-License-Identifier: GPL-3.0-or-later

#include "typingengine.h"
#include "keyboardcontroller.h"

#include <QElapsedTimer>

#include <limits>
#include <QLoggingCategory>

// Decisions and sizes only; typed text never reaches the log.
Q_LOGGING_CATEGORY(lcEngine, "v3keyboard.engine", QtWarningMsg)

namespace V3Keyboard
{
namespace
{

constexpr int ModelTail = 256;
constexpr int CompareTail = 64;
constexpr int MaxHistory = 64;
// Echoes of our own edits arrive 2-6 ms later (p99, device trace 0.2.5; max
// 36 ms). Unknown states inside this window are self-caused, not user edits.
constexpr qint64 SettleMs = 150;

qint64 monotonicMs()
{
    static QElapsedTimer timer = [] { QElapsedTimer t; t.start(); return t; }();
    return timer.elapsed();
}

QString tail(const QString &text)
{
    return text.size() > ModelTail ? text.right(ModelTail) : text;
}

bool sameTextState(const QString &a, const QString &b)
{
    if (a.size() >= CompareTail && b.size() >= CompareTail) return a.right(CompareTail) == b.right(CompareTail);
    return a == b;
}

}

TypingEngine::TypingEngine(KeyboardController &controller)
    : m_controller(controller)
{
    refreshSuggestions();
}

void TypingEngine::setLanguage(const QString &code)
{
    m_lexicon.setLanguage(code);
    resetComposition();
}

QString TypingEngine::language() const { return m_lexicon.language(); }

void TypingEngine::setSuggestionsEnabled(bool enabled)
{
    m_suggestionsEnabled = enabled;
    refreshSuggestions();
}
void TypingEngine::setAutocorrectEnabled(bool enabled) { m_autocorrectEnabled = enabled; }
void TypingEngine::setLearningEnabled(bool enabled) { m_learningEnabled = enabled; }
void TypingEngine::setAutoCapitalizationEnabled(bool enabled) { m_autoCapitalizationEnabled = enabled; }
void TypingEngine::setDoubleSpacePeriodEnabled(bool enabled) { m_doubleSpacePeriodEnabled = enabled; }
void TypingEngine::setAutoCapitalizationAllowed(bool allowed) { m_autoCapitalizationAllowed = allowed; }
bool TypingEngine::surroundingTextSupported() const { return m_surroundingSupported; }
void TypingEngine::setClockForTesting(std::function<qint64()> clock) { m_clock = std::move(clock); }
void TypingEngine::setBlockOffensive(bool enabled) { m_lexicon.setBlockOffensive(enabled); refreshSuggestions(); }
void TypingEngine::setKeyboardRows(const QStringList &rows) { m_lexicon.setKeyboardRows(rows); }
void TypingEngine::setAutoSpaceAfterPunctuation(bool enabled) { m_autoSpaceAfterPunctuation = enabled; m_autoSpacePending = false; }
void TypingEngine::forgetWord(const QString &word) { m_lexicon.forgetWord(word); refreshSuggestions(); }
bool TypingEngine::addUserWord(const QString &word) { const bool ok = m_lexicon.addUserWord(word); refreshSuggestions(); return ok; }
void TypingEngine::removeUserWord(const QString &word) { m_lexicon.removeUserWord(word); refreshSuggestions(); }
QStringList TypingEngine::userWords() const { return m_lexicon.userWords(); }
bool TypingEngine::isKnownWord(const QString &word) const { return m_lexicon.hasWord(word); }
void TypingEngine::reloadUserDictionaryFile() { m_lexicon.reloadUserDictionaryFile(); refreshSuggestions(); }

void TypingEngine::setCompositionEnabled(bool enabled) { if (!enabled) commitComposition(); m_compositionEnabled = enabled; }
bool TypingEngine::compositionEnabled() const { return m_compositionEnabled; }
bool TypingEngine::composing() const { return m_composing || m_pendingSpace; }

bool TypingEngine::compositionAvailable() const
{
    // Preedit needs a text-input client (KWin drops it for the fake-key path)
    // and must never show secrets in clear text.
    return m_compositionEnabled && m_surroundingSupported && !m_sensitiveContext && !m_preeditRejected;
}

bool TypingEngine::setPreeditLocal(const QString &text)
{
    m_lastLocalOpMs = m_clock ? m_clock() : monotonicMs();
    if (!m_controller.setPreedit(text)) {
        qCDebug(lcEngine) << "preedit rejected -> immediate commits for this context";
        m_preeditRejected = true;
        return false;
    }
    // Clients such as Firefox include the preedit in their surrounding text.
    if (!text.isEmpty()) recordState(tail(m_model + text));
    return true;
}

QString TypingEngine::corrected(const QString &word) const
{
    if (!m_autocorrectEnabled || word.isEmpty()) return word;
    if (!m_noCorrectionFor.isEmpty() && word.compare(m_noCorrectionFor, Qt::CaseInsensitive) == 0) return word;
    const QString correction = m_lexicon.bestCorrection(word, m_previousWord);
    if (correction.isEmpty()) return word;
    QString formatted = correction;
    if (word.front().isUpper()) formatted[0] = formatted.at(0).toUpper();
    return formatted;
}

// Returns what is held in the preedit after a word (corrected word, if any,
// plus the space) and clears that state. The caller commits it.
QString TypingEngine::takePendingText()
{
    QString text = m_pendingWord;
    if (!m_pendingWord.isEmpty()) {
        m_currentWord = m_pendingWord;
        finalizeCurrentWord();
    }
    if (m_pendingPeriod) text += QLatin1Char('.');
    m_pendingWord.clear();
    m_pendingOriginal.clear();
    m_pendingSpace = false;
    m_pendingPeriod = false;
    return text;
}

void TypingEngine::insertDictation(const QString &text)
{
    QString phrase = text.simplified();
    if (phrase.isEmpty()) return;
    const bool hadWord = (m_composing && !m_currentWord.isEmpty()) || !m_currentWord.isEmpty();
    commitComposition();
    if (!m_sensitiveContext && m_sentenceStart && m_autoCapitalizationEnabled && m_autoCapitalizationAllowed) {
        phrase[0] = phrase.at(0).toUpper();
    }
    const QString separator = hadWord ? QStringLiteral(" ") : QString();
    commitLocal(separator + phrase + QLatin1Char(' '));
    const QChar last = phrase.back();
    m_sentenceStart = last == QLatin1Char('.') || last == QLatin1Char('!') || last == QLatin1Char('?');
    m_sentencePunctuationPending = false;
    const QStringList words = phrase.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    m_previousWord = words.isEmpty() ? QString() : words.last().toLower();
    m_currentWord.clear();
    m_lastActionWasSpace = false;
    m_spaceFromSuggestion = false;
    refreshSuggestions();
}

void TypingEngine::commitComposition()
{
    if (m_composing && !m_currentWord.isEmpty()) {
        const QString word = m_currentWord;
        finalizeCurrentWord();
        commitLocal(word);
    } else if (m_pendingSpace) {
        commitLocal(takePendingText() + QLatin1Char(' '));
    }
    m_composing = false;
    m_pendingSpace = false;
}

bool TypingEngine::suggestionsEnabled() const { return m_suggestionsEnabled; }
bool TypingEngine::autocorrectEnabled() const { return m_autocorrectEnabled; }
bool TypingEngine::learningEnabled() const { return m_learningEnabled; }
bool TypingEngine::autoCapitalizationEnabled() const { return m_autoCapitalizationEnabled; }
bool TypingEngine::doubleSpacePeriodEnabled() const { return m_doubleSpacePeriodEnabled; }
bool TypingEngine::wantsAutoUppercase() const
{
    return !m_sensitiveContext && m_autoCapitalizationEnabled && m_autoCapitalizationAllowed
        && m_sentenceStart && m_currentWord.isEmpty();
}
bool TypingEngine::sensitiveContext() const { return m_sensitiveContext; }
QString TypingEngine::currentWord() const { return m_currentWord; }
QString TypingEngine::previousWord() const { return m_previousWord; }
QStringList TypingEngine::suggestions() const { return m_suggestions; }
QStringList TypingEngine::glideTrace() const { return m_glideTrace; }

QString TypingEngine::formatForSentence(const QString &word) const
{
    if (!m_autoCapitalizationEnabled || !m_autoCapitalizationAllowed || !m_sentenceStart || word.isEmpty()) return word;
    QString result = word;
    result[0] = result.at(0).toUpper();
    return result;
}

void TypingEngine::typeLetter(const QString &text, QPointF touch)
{
    const qsizetype wordBefore = m_currentWord.size();
    m_lastWasDoublePeriod = false;
    if (text.isEmpty()) return;
    m_spaceFromSuggestion = false;
    if (m_sensitiveContext) {
        commitLocal(text);
        m_currentWord.clear();
        m_suggestions.clear();
        m_sentenceStart = false;
        m_lastActionWasSpace = false;
        return;
    }
    if (m_pendingSpace) commitLocal(takePendingText() + QLatin1Char(' '));
    if (m_autoSpacePending) {
        m_autoSpacePending = false;
        commitLocal(QStringLiteral(" "));
    }
    // Composition starts only at a word boundary so a word is never split
    // between committed text and preedit.
    if (!m_composing && m_currentWord.isEmpty() && compositionAvailable()) m_composing = true;
    m_currentWord += text;
    if (m_composing && !setPreeditLocal(m_currentWord)) {
        m_composing = false;
        commitLocal(m_currentWord);
    } else if (!m_composing) {
        commitLocal(text);
    }
    m_sentenceStart = false;
    m_sentencePunctuationPending = false;
    m_lastActionWasSpace = false;
    if (m_currentWord.size() == wordBefore + 1) {
        m_touchOffsets.resize(wordBefore);
        m_touchOffsets.append(touch);
    }
    refreshSuggestions();
}

void TypingEngine::typeText(const QString &text)
{
    m_lastWasDoublePeriod = false;
    if (text.isEmpty()) return;
    m_spaceFromSuggestion = false;
    if (m_sensitiveContext) {
        commitLocal(text);
        m_currentWord.clear();
        m_suggestions.clear();
        m_lastActionWasSpace = false;
        return;
    }

    const bool punctuation = text.size() == 1 && QStringLiteral(".,!?;:").contains(text);
    m_autoSpacePending = false;
    // Only after words: "3.5" or "e.g" style tokens stay intact.
    const bool afterLetter = !m_currentWord.isEmpty() || (!m_model.isEmpty() && m_model.back().isLetter());
    const bool armAutoSpace = m_autoSpaceAfterPunctuation && punctuation && afterLetter && !m_sensitiveContext;
    if (m_composing) {
        // Word and punctuation leave in one commit; the word is corrected first.
        const QString word = punctuation ? corrected(m_currentWord) : m_currentWord;
        m_currentWord = word;
        finalizeCurrentWord();
        m_composing = false;
        commitLocal(word + text);
    } else if (m_pendingSpace) {
        const bool afterWord = m_pendingSpaceAfterWord;
        const QString held = takePendingText();
        if (punctuation && afterWord) {
            // LatinIME/Gboard: "word ," becomes "word, " — the space typed
            // after the word moves behind the punctuation (held again so a
            // following Space only confirms it).
            commitLocal(held + text);
            if (setPreeditLocal(QStringLiteral(" "))) {
                m_pendingSpace = true;
                m_spaceFromSuggestion = true;
                m_pendingSpaceAfterWord = false;
            } else {
                commitLocal(QStringLiteral(" "));
            }
            observePunctuation(text);
            m_lastActionWasSpace = false;
            refreshSuggestions();
            return;
        }
        commitLocal(held + QStringLiteral(" ") + text);
    } else {
        if (punctuation && !m_currentWord.isEmpty()) finalizeCurrentWord();
        commitLocal(text);
    }
    observePunctuation(text);
    m_lastActionWasSpace = false;
    m_autoSpacePending = armAutoSpace;
    refreshSuggestions();
}

void TypingEngine::observePunctuation(const QString &text)
{
    if (text.contains(QLatin1Char('.')) || text.contains(QLatin1Char('!')) || text.contains(QLatin1Char('?'))) {
        m_sentencePunctuationPending = true;
        // Arm capitalization immediately. This also covers clients that do not
        // send a fresh surrounding-text event between punctuation and the next tap.
        m_sentenceStart = true;
    }
}

void TypingEngine::commitLocal(const QString &text)
{
    m_lastLocalOpMs = m_clock ? m_clock() : monotonicMs();
    m_controller.tapText(text);
    m_model = tail(m_model + text);
    recordLocalState();
}

void TypingEngine::backspaceLocal()
{
    m_lastLocalOpMs = m_clock ? m_clock() : monotonicMs();
    m_controller.backspace();
    if (!m_model.isEmpty()) {
        const bool pair = m_model.size() >= 2 && m_model.back().isLowSurrogate();
        m_model.chop(pair ? 2 : 1);
    }
    recordLocalState();
}

bool TypingEngine::deleteLocal(const QString &text)
{
    m_lastLocalOpMs = m_clock ? m_clock() : monotonicMs();
    if (!m_controller.deleteBeforeCursor(text)) return false;
    m_model.chop(text.size());
    recordLocalState();
    return true;
}

void TypingEngine::recordLocalState()
{
    m_inSync = false;
    recordState(m_model);
}

void TypingEngine::recordState(const QString &state)
{
    if (!m_history.isEmpty() && m_history.back() == state) return;
    m_history.append(state);
    if (m_history.size() > MaxHistory) m_history.removeFirst();
}

void TypingEngine::forgetTextState()
{
    m_model.clear();
    m_history = {QString()};
    m_inSync = false;
    // The keyboard no longer models the text (cursor moved, paste, client
    // reset): the next echo is the truth and must not be treated as
    // self-caused by the settle window.
    m_lastLocalOpMs = std::numeric_limits<qint64>::min() / 2;
}

// AOSP LatinIME InputLogic.tryPerformDoubleSpacePeriod (Apache-2.0): the
// second space must come within 1100 ms, and the character before the first
// space must be a letter/digit, one of ' " ) ] } > + % or a symbol (emoji).
bool TypingEngine::doubleSpaceAllowedAfter(const QString &textBeforeSpace) const
{
    if (!m_doubleSpacePeriodEnabled || !m_lastActionWasSpace || m_previousSpaceMs < 0) return false;
    if (m_lastSpaceMs - m_previousSpaceMs >= 1100) return false;
    if (textBeforeSpace.isEmpty()) return false;
    const QChar last = textBeforeSpace.back();
    if (last.isLowSurrogate()) return true;                      // emoji
    return last.isLetterOrNumber() || QStringLiteral("'\")]}>+%").contains(last)
        || last.category() == QChar::Symbol_Other;
}

// After deleting, an empty field (or one ending in . ! ?) starts a sentence
// again (Gboard). Only for clients that report their text: otherwise the
// keyboard cannot know what precedes the cursor.
void TypingEngine::rearmSentenceStartFromText()
{
    if (!m_surroundingSupported || !m_currentWord.isEmpty() || m_composing) return;
    QString before = m_model;
    while (!before.isEmpty() && before.back().isSpace()) before.chop(1);
    m_sentenceStart = before.isEmpty() || before.endsWith(QLatin1Char('.'))
        || before.endsWith(QLatin1Char('!')) || before.endsWith(QLatin1Char('?'));
}

bool TypingEngine::inSyncWithClient() const
{
    return m_surroundingSupported && m_inSync;
}

// Replaces `existing` (the text right before the cursor) with `replacement`.
// On text-input clients the deletion uses delete_surrounding_text so it stays
// ordered with the commit; Backspace key events travel on a different channel
// that GTK/Firefox apply after already-received text-input commits.
bool TypingEngine::replaceBeforeCursor(const QString &existing, const QString &replacement, bool allowUnconfirmed)
{
    if (existing.isEmpty()) return false;
    if (m_surroundingSupported) {
        const bool confirmed = inSyncWithClient() && m_model.endsWith(existing);
        qCDebug(lcEngine) << "replace len" << existing.size() << "->" << replacement.size()
                          << (confirmed ? "text-channel" : (allowUnconfirmed ? "key-fallback" : "skipped"));
        if (confirmed && deleteLocal(existing)) {
            commitLocal(replacement);
            return true;
        }
        if (!confirmed && !allowUnconfirmed) return false;
    }
    for (int i = 0; i < existing.size(); ++i) backspaceLocal();
    commitLocal(replacement);
    return true;
}

void TypingEngine::finalizeCurrentWord(bool updatePrevious)
{
    if (m_currentWord.isEmpty()) return;
    const QString finalized = m_currentWord.toLower();
    if (m_learningEnabled) {
        // Persist one combined learning update instead of serializing the full
        // learned maps twice for every completed word.
        m_lexicon.learnWordWithContext(finalized, m_previousWord);
    }
    if (updatePrevious) m_previousWord = finalized;
    m_currentWord.clear();
}

void TypingEngine::space()
{
    m_lastWasDoublePeriod = false;
    // Every Space press restarts the LatinIME double-space countdown.
    m_previousSpaceMs = m_lastSpaceMs;
    m_lastSpaceMs = m_clock ? m_clock() : monotonicMs();
    m_autoSpacePending = false;
    if (m_spaceFromSuggestion) {
        // The suggestion already added a space. This Space confirms it and
        // counts as the user's first Space; only the next one makes ". ".
        m_spaceFromSuggestion = false;
        m_lastActionWasSpace = true;
        refreshSuggestions();
        return;
    }
    if (m_sensitiveContext) {
        commitLocal(QStringLiteral(" "));
        m_currentWord.clear();
        m_suggestions.clear();
        return;
    }
    if (m_composing) {
        const QString original = m_currentWord;
        const QString word = corrected(original);
        m_composing = false;
        m_noCorrectionFor.clear();
        if (word != original && setPreeditLocal(word + QLatin1Char(' '))) {
            // Keep the correction revertible: Backspace right now restores
            // what was typed; any other key commits it. Nothing is deleted.
            m_pendingWord = word;
            m_pendingOriginal = original;
            m_pendingSpace = true;
            m_currentWord.clear();
        } else {
            m_currentWord = word;
            finalizeCurrentWord();
            commitLocal(word);
            // Hold the space in the preedit: a second Space can then become
            // ". " with a plain commit, no deletion needed.
            if (setPreeditLocal(QStringLiteral(" "))) m_pendingSpace = true;
            else commitLocal(QStringLiteral(" "));
        }
        m_pendingSpaceAfterWord = m_pendingSpace;
        m_lastActionWasSpace = true;
        if (m_sentencePunctuationPending) {
            m_sentenceStart = true;
            m_sentencePunctuationPending = false;
        }
        refreshSuggestions();
        return;
    }
    if (m_pendingSpace) {
        const QString before = m_pendingWord.isEmpty() ? m_model : m_pendingWord;
        const bool doubleSpace = !m_pendingPeriod && doubleSpaceAllowedAfter(before);
        const QString held = takePendingText();
        if (doubleSpace) {
            // Held as ". " in the preedit so Backspace can revert it to " "
            // without deleting committed text (LatinIME revertDoubleSpacePeriod).
            if (!held.isEmpty()) commitLocal(held);
            if (setPreeditLocal(QStringLiteral(". "))) {
                m_pendingSpace = true;
                m_pendingPeriod = true;
            } else {
                commitLocal(QStringLiteral(". "));
            }
            m_lastSpaceMs = -1;               // no chained double space
            m_pendingSpaceAfterWord = false;
            m_sentenceStart = true;
            m_sentencePunctuationPending = false;
            m_lastActionWasSpace = false;
            refreshSuggestions();
            return;
        }
        commitLocal(held + QLatin1Char(' '));
        if (setPreeditLocal(QStringLiteral(" "))) m_pendingSpace = true;
        else commitLocal(QStringLiteral(" "));
        m_pendingSpaceAfterWord = false;            // consecutive spaces
        m_lastActionWasSpace = true;
        refreshSuggestions();
        return;
    }
    if (m_currentWord.isEmpty()) {
        const QString beforeSpace = m_model.endsWith(QLatin1Char(' ')) ? m_model.chopped(1) : QString();
        if (doubleSpaceAllowedAfter(beforeSpace)
            && replaceBeforeCursor(QStringLiteral(" "), QStringLiteral(". "), false)) {
            m_lastWasDoublePeriod = true;
            m_lastSpaceMs = -1;               // no chained double space
            m_sentenceStart = true;
            m_sentencePunctuationPending = false;
            m_lastActionWasSpace = false;
            refreshSuggestions();
            return;
        }
        commitLocal(QStringLiteral(" "));
        if (m_sentencePunctuationPending) {
            m_sentenceStart = true;
            m_sentencePunctuationPending = false;
        }
        m_lastActionWasSpace = true;
        refreshSuggestions();
        return;
    }

    if (m_autocorrectEnabled) {
        const QString correction = m_lexicon.bestCorrection(m_currentWord, m_previousWord);
        if (!correction.isEmpty()) {
            QString formatted = correction;
            if (!m_currentWord.isEmpty() && m_currentWord.front().isUpper()) {
                formatted[0] = formatted.at(0).toUpper();
            }
            if (replaceBeforeCursor(m_currentWord, formatted, false)) m_currentWord = formatted;
        }
    }

    finalizeCurrentWord();
    commitLocal(QStringLiteral(" "));
    m_lastActionWasSpace = true;
    if (m_sentencePunctuationPending) {
        m_sentenceStart = true;
        m_sentencePunctuationPending = false;
    }
    refreshSuggestions();
}

void TypingEngine::backspace()
{
    m_autoSpacePending = false;
    m_spaceFromSuggestion = false;
    if (m_pendingSpace && m_pendingPeriod) {
        // LatinIME: Backspace right after a double-space period gives a single
        // space, and the next word is not capitalised.
        m_pendingPeriod = false;
        setPreeditLocal(QStringLiteral(" "));
        m_pendingSpaceAfterWord = false;
        m_sentenceStart = false;
        m_lastActionWasSpace = false;
        refreshSuggestions();
        return;
    }
    if (m_lastWasDoublePeriod) {
        m_lastWasDoublePeriod = false;
        if (replaceBeforeCursor(QStringLiteral(". "), QStringLiteral(" "), false)) {
            m_sentenceStart = false;
            m_lastActionWasSpace = false;
            refreshSuggestions();
            return;
        }
    }
    if (m_pendingSpace && !m_pendingWord.isEmpty()) {
        // Undo the autocorrection: back to exactly what was typed, still
        // composing, and do not correct that word again.
        const QString original = m_pendingOriginal;
        m_pendingWord.clear();
        m_pendingOriginal.clear();
        m_pendingSpace = false;
        m_noCorrectionFor = original;
        // Undoing a correction is an explicit "this is my word" (Gboard).
        m_lexicon.promoteWord(original);
        m_currentWord = original;
        m_composing = true;
        setPreeditLocal(original);
        m_lastActionWasSpace = false;
        rearmSentenceStartFromText();
        refreshSuggestions();
        return;
    }
    if (m_pendingSpace) {
        m_pendingSpace = false;
        setPreeditLocal(QString());
        m_lastActionWasSpace = false;
        rearmSentenceStartFromText();
        refreshSuggestions();
        return;
    }
    if (m_composing && !m_currentWord.isEmpty()) {
        m_currentWord.chop(1);
        setPreeditLocal(m_currentWord);
        if (m_currentWord.isEmpty()) m_composing = false;
        rearmSentenceStartFromText();
        refreshSuggestions();
        return;
    }
    backspaceLocal();
    if (m_sensitiveContext) {
        m_currentWord.clear();
        m_suggestions.clear();
        return;
    }
    if (!m_currentWord.isEmpty()) {
        m_currentWord.chop(1);
    } else {
        m_previousWord.clear();
        m_sentencePunctuationPending = false;
    }
    m_lastActionWasSpace = false;
    rearmSentenceStartFromText();
        refreshSuggestions();
}

void TypingEngine::backspaceRepeated(int count)
{
    m_spaceFromSuggestion = false;
    int bounded = qBound(1, count, 32);
    if (m_pendingSpace && bounded > 0) {
        if (!m_pendingWord.isEmpty()) {
            m_currentWord = m_pendingOriginal;   // revert, then keep deleting
            m_noCorrectionFor = m_pendingOriginal;
            m_lexicon.promoteWord(m_pendingOriginal);
            m_pendingWord.clear();
            m_pendingOriginal.clear();
            m_composing = true;
        }
        m_pendingSpace = false;
        setPreeditLocal(m_composing ? m_currentWord : QString());
        --bounded;
    }
    if (m_composing && bounded > 0) {
        const int fromPreedit = qMin(bounded, int(m_currentWord.size()));
        m_currentWord.chop(fromPreedit);
        setPreeditLocal(m_currentWord);
        bounded -= fromPreedit;
        if (m_currentWord.isEmpty()) m_composing = false;
        if (bounded == 0) {
            m_lastActionWasSpace = false;
            rearmSentenceStartFromText();
        refreshSuggestions();
            return;
        }
    }
    for (int i = 0; i < bounded; ++i) backspaceLocal();

    if (m_sensitiveContext) {
        m_currentWord.clear();
        m_suggestions.clear();
        return;
    }

    if (!m_currentWord.isEmpty()) {
        const int remove = qMin(bounded, static_cast<int>(m_currentWord.size()));
        m_currentWord.chop(remove);
        if (remove < bounded) m_previousWord.clear();
    } else {
        m_previousWord.clear();
        m_sentencePunctuationPending = false;
    }

    m_lastActionWasSpace = false;
    rearmSentenceStartFromText();
        refreshSuggestions();
}

void TypingEngine::enter()
{
    m_lastWasDoublePeriod = false;
    m_autoSpacePending = false;
    m_spaceFromSuggestion = false;
    if (m_pendingSpace) {
        const QString held = takePendingText();
        if (!held.isEmpty()) commitLocal(held);      // corrected word, no trailing space
        else setPreeditLocal(QString());
    }
    // Commit before the Return key: GTK applies text-input commits before
    // queued key events, so this order can never be inverted.
    if (m_composing) commitComposition();
    if (!m_currentWord.isEmpty()) finalizeCurrentWord();
    m_lastLocalOpMs = m_clock ? m_clock() : monotonicMs();
    m_controller.enter();
    m_model = tail(m_model + QLatin1Char('\n'));
    recordLocalState();
    m_sentenceStart = true;
    m_sentencePunctuationPending = false;
    m_lastActionWasSpace = false;
    refreshSuggestions();
}

void TypingEngine::chooseSuggestion(const QString &word)
{
    if (word.isEmpty() || m_sensitiveContext) return;
    QString selected = word;
    if (!m_currentWord.isEmpty() && m_currentWord.front().isUpper()) {
        selected[0] = selected.at(0).toUpper();
    } else if (m_sentenceStart) {
        selected = formatForSentence(selected);
    }

    const bool composed = m_composing;
    if (m_pendingSpace) commitLocal(takePendingText() + QLatin1Char(' '));
    if (m_composing) {
        m_composing = false;
        commitLocal(selected);               // replaces the preedit atomically
    } else if (!m_currentWord.isEmpty()) {
        // A deliberate tap: fall back to key events if the client has not
        // confirmed the word yet rather than ignoring the user's choice.
        replaceBeforeCursor(m_currentWord, selected, true);
    } else {
        commitLocal(selected);
    }
    m_currentWord = selected;
    finalizeCurrentWord();
    if (!(composed || compositionAvailable()) || !setPreeditLocal(QStringLiteral(" "))) {
        commitLocal(QStringLiteral(" "));
    } else {
        m_pendingSpace = true;
    }
    m_pendingSpaceAfterWord = m_pendingSpace;
    m_spaceFromSuggestion = true;
    m_lastActionWasSpace = false;
    m_sentenceStart = false;
    refreshSuggestions();
}

QString TypingEngine::autocorrectTarget() const { return m_autocorrectTarget; }

void TypingEngine::refreshSuggestions()
{
    // Touch offsets follow the word: shorter after Backspace, empty for a new one.
    if (m_touchOffsets.size() > m_currentWord.size()) m_touchOffsets.resize(m_currentWord.size());
    m_lexicon.setTouchOffsets(m_touchOffsets);
    // What Space would insert instead of the typed word (shown highlighted
    // in the middle of the strip, as Gboard does).
    m_autocorrectTarget.clear();
    if (!m_sensitiveContext && m_autocorrectEnabled && !m_currentWord.isEmpty()
        && (m_noCorrectionFor.isEmpty() || m_currentWord.compare(m_noCorrectionFor, Qt::CaseInsensitive) != 0)) {
        // Cheap preview per keystroke; Space runs the full Hunspell check.
        QString target = m_lexicon.correctionPreview(m_currentWord, m_previousWord);
        if (!target.isEmpty() && m_currentWord.front().isUpper()) target[0] = target.at(0).toUpper();
        if (!target.isEmpty() && target != m_currentWord) m_autocorrectTarget = target;
    }
    if (m_sensitiveContext || !m_suggestionsEnabled) {
        m_suggestions.clear();
        return;
    }
    if (!m_currentWord.isEmpty()) {
        m_suggestions = m_lexicon.suggestions(m_currentWord, m_previousWord, 3);
        // Gboard: suggestions follow the case of what was typed ("Hel" ->
        // "Hello", "HEL" -> "HELLO"); words with their own casing ("iPhone")
        // are left alone.
        const bool allCaps = m_currentWord.size() >= 2 && m_currentWord.toUpper() == m_currentWord
            && m_currentWord.toLower() != m_currentWord;
        const bool initialCap = !m_currentWord.isEmpty() && m_currentWord.front().isUpper();
        for (QString &item : m_suggestions) {
            if (item.isEmpty() || item.toLower() != item) continue;
            if (allCaps) item = item.toUpper();
            else if (initialCap || m_sentenceStart) item[0] = item.at(0).toUpper();
        }
        return;
    }
    m_suggestions = m_lexicon.nextWords(m_previousWord, 3);
}

void TypingEngine::resetComposition()
{
    m_spaceFromSuggestion = false;
    m_pendingWord.clear();
    m_pendingOriginal.clear();
    m_composing = false;
    m_pendingSpace = false;
    m_currentWord.clear();
    m_previousWord.clear();
    m_suggestions.clear();
    m_glideTrace.clear();
    m_sentenceStart = true;
    m_lastActionWasSpace = false;
    m_sentencePunctuationPending = false;
    forgetTextState();
    m_lexicon.flushLearning();
    refreshSuggestions();
}

void TypingEngine::resetInputContext()
{
    m_surroundingSupported = false;
    m_preeditRejected = false;
    m_autoCapitalizationAllowed = true;
    resetComposition();
}

void TypingEngine::setSensitiveContext(bool sensitive)
{
    if (m_sensitiveContext == sensitive) return;
    m_sensitiveContext = sensitive;
    resetComposition();
}

bool TypingEngine::syncSurroundingText(const QString &text, int cursorByte, int anchorByte)
{
    if (cursorByte < 0 || anchorByte < 0) return false;
    m_surroundingSupported = true;
    if (m_sensitiveContext) return false;

    const QByteArray utf8 = text.toUtf8();
    const int bounded = qBound(0, cursorByte, static_cast<int>(utf8.size()));
    const QString before = tail(QString::fromUtf8(utf8.left(bounded)));

    if (composing()) {
        // The keyboard owns the composition. Echo contents are unreliable
        // (lagging caches, placeholder characters in empty editors), and
        // abandoning a live preedit loses text; external changes end a
        // composition through reset() (observed on device when the user
        // clicks elsewhere or switches windows).
        qCDebug(lcEngine) << "echo bytes" << bounded << "ignored while composing";
        return false;
    }

    if (cursorByte == anchorByte) {
        // Any state this keyboard produced recently (including states with
        // its preedit) is an echo. Clients such as Firefox answer from a
        // lagging cache, so stale echoes of *older* states are normal and
        // must never be mistaken for user edits.
        for (int i = m_history.size() - 1; i >= 0; --i) {
            if (sameTextState(m_history.at(i), before)) {
                m_inSync = !composing() && i == m_history.size() - 1 && sameTextState(m_model, before);
                qCDebug(lcEngine) << "echo bytes" << bounded << "known state" << i + 1 << "of" << m_history.size()
                                  << (m_inSync ? "in-sync" : "stale/composing");
                return false;
            }
        }
    }

    const qint64 now = m_clock ? (m_clock ? m_clock() : monotonicMs()) : monotonicMs();
    if (now - m_lastLocalOpMs < SettleMs) {
        qCDebug(lcEngine) << "echo bytes" << bounded << "unknown but self-caused (settle window) -> ignored";
        m_inSync = false;
        return false;
    }

    // A state the keyboard never produced, at human speed: cursor moved,
    // selection, or the application changed the text. The client is the
    // source of truth.
    qCDebug(lcEngine) << "echo bytes" << bounded << "unknown state -> adopt";
    m_composing = false;
    m_pendingSpace = false;
    m_spaceFromSuggestion = false;
    m_pendingWord.clear();
    m_pendingOriginal.clear();
    m_history = {before};
    m_model = before;
    m_inSync = true;

    QString trailing;
    QString incomingPrevious;
    bool incomingSentenceStart = false;
    if (cursorByte == anchorByte) {
        int start = before.size();
        while (start > 0 && before.at(start - 1).isLetter()) --start;
        trailing = before.mid(start);
        int prevEnd = start;
        while (prevEnd > 0 && !before.at(prevEnd - 1).isLetter()) --prevEnd;
        int prevStart = prevEnd;
        while (prevStart > 0 && before.at(prevStart - 1).isLetter()) --prevStart;
        incomingPrevious = before.mid(prevStart, prevEnd - prevStart).toLower();
        incomingSentenceStart = before.trimmed().isEmpty();
        if (!incomingSentenceStart && trailing.isEmpty()) {
            const QChar last = before.trimmed().back();
            incomingSentenceStart = last == QLatin1Char('.') || last == QLatin1Char('!') || last == QLatin1Char('?');
        }
    }

    if (m_currentWord == trailing && m_previousWord == incomingPrevious && m_sentenceStart == incomingSentenceStart) {
        return false;
    }
    m_currentWord = trailing;
    m_previousWord = incomingPrevious;
    m_sentenceStart = incomingSentenceStart;
    m_lastActionWasSpace = false;
    refreshSuggestions();
    return true;
}

void TypingEngine::clearLearning()
{
    m_lexicon.clearLearning();
    refreshSuggestions();
}

void TypingEngine::beginGlide(const QString &key)
{
    if (m_sensitiveContext) return;
    commitComposition();
    m_glideTrace.clear();
    glideThrough(key);
}

void TypingEngine::glideThrough(const QString &key)
{
    if (key.isEmpty() || m_sensitiveContext) return;
    const QString normalized = key.left(1).toLower();
    if (m_glideTrace.isEmpty() || m_glideTrace.back() != normalized) m_glideTrace.append(normalized);
}

QString TypingEngine::endGlide()
{
    if (m_sensitiveContext) { m_glideTrace.clear(); return {}; }
    const QString decoded = m_lexicon.decodeGlide(m_glideTrace, m_previousWord);
    m_glideTrace.clear();
    if (decoded.isEmpty()) return {};

    const QString formatted = formatForSentence(decoded);
    // Same flow as tapping a suggestion: commit the word, hold the automatic
    // space so a following Space confirms it instead of making ". ".
    chooseSuggestion(formatted);
    return formatted;
}

}
