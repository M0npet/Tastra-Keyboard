// SPDX-License-Identifier: GPL-3.0-or-later

#include "typingengine.h"
#include "keyboardcontroller.h"

namespace V3Keyboard
{
namespace
{

constexpr int ModelTail = 256;
constexpr int CompareTail = 64;
constexpr int MaxPredicted = 64;

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

void TypingEngine::typeLetter(const QString &text)
{
    if (text.isEmpty()) return;
    commitLocal(text);
    if (m_sensitiveContext) {
        m_currentWord.clear();
        m_suggestions.clear();
        m_sentenceStart = false;
        m_lastActionWasSpace = false;
        return;
    }
    m_currentWord += text;
    m_sentenceStart = false;
    m_sentencePunctuationPending = false;
    m_lastActionWasSpace = false;
    refreshSuggestions();
}

void TypingEngine::typeText(const QString &text)
{
    if (text.isEmpty()) return;
    if (m_sensitiveContext) {
        commitLocal(text);
        m_currentWord.clear();
        m_suggestions.clear();
        m_lastActionWasSpace = false;
        return;
    }

    const bool punctuation = text.size() == 1 && QStringLiteral(".,!?;:").contains(text);
    if (punctuation && !m_currentWord.isEmpty()) {
        finalizeCurrentWord();
    }

    commitLocal(text);
    observePunctuation(text);
    m_lastActionWasSpace = false;
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
    m_controller.tapText(text);
    m_model = tail(m_model + text);
    recordLocalState();
}

void TypingEngine::backspaceLocal()
{
    m_controller.backspace();
    if (!m_model.isEmpty()) {
        const bool pair = m_model.size() >= 2 && m_model.back().isLowSurrogate();
        m_model.chop(pair ? 2 : 1);
    }
    recordLocalState();
}

bool TypingEngine::deleteLocal(const QString &text)
{
    if (!m_controller.deleteBeforeCursor(text)) return false;
    m_model.chop(text.size());
    recordLocalState();
    return true;
}

void TypingEngine::recordLocalState()
{
    m_predicted.append(m_model);
    if (m_predicted.size() > MaxPredicted) m_predicted.removeFirst();
}

void TypingEngine::forgetTextState()
{
    m_model.clear();
    m_confirmed.clear();
    m_predicted.clear();
}

bool TypingEngine::inSyncWithClient() const
{
    return m_surroundingSupported && m_predicted.isEmpty() && sameTextState(m_model, m_confirmed);
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
    if (m_sensitiveContext) {
        commitLocal(QStringLiteral(" "));
        m_currentWord.clear();
        m_suggestions.clear();
        return;
    }
    if (m_currentWord.isEmpty()) {
        if (m_doubleSpacePeriodEnabled && m_lastActionWasSpace && !m_previousWord.isEmpty()
            && replaceBeforeCursor(QStringLiteral(" "), QStringLiteral(". "), false)) {
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
    refreshSuggestions();
}

void TypingEngine::backspaceRepeated(int count)
{
    const int bounded = qBound(1, count, 32);
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
    refreshSuggestions();
}

void TypingEngine::enter()
{
    if (!m_currentWord.isEmpty()) finalizeCurrentWord();
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

    if (!m_currentWord.isEmpty()) {
        // A deliberate tap: fall back to key events if the client has not
        // confirmed the word yet rather than ignoring the user's choice.
        replaceBeforeCursor(m_currentWord, selected, true);
    } else {
        commitLocal(selected);
    }
    m_currentWord = selected;
    finalizeCurrentWord();
    commitLocal(QStringLiteral(" "));
    m_lastActionWasSpace = true;
    m_sentenceStart = false;
    refreshSuggestions();
}

void TypingEngine::refreshSuggestions()
{
    if (m_sensitiveContext || !m_suggestionsEnabled) {
        m_suggestions.clear();
        return;
    }
    if (!m_currentWord.isEmpty()) {
        m_suggestions = m_lexicon.suggestions(m_currentWord, m_previousWord, 3);
        if (m_sentenceStart) {
            for (QString &item : m_suggestions) item = formatForSentence(item);
        }
        return;
    }
    m_suggestions = m_lexicon.nextWords(m_previousWord, 3);
}

void TypingEngine::resetComposition()
{
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

    if (cursorByte == anchorByte) {
        // Echo of a state this keyboard produced? Earlier states are stale,
        // the latest one means client and keyboard agree. Either way the local
        // composition stays authoritative.
        for (int i = m_predicted.size() - 1; i >= 0; --i) {
            if (sameTextState(m_predicted.at(i), before)) {
                m_predicted.erase(m_predicted.begin(), m_predicted.begin() + i + 1);
                m_confirmed = before;
                return false;
            }
        }
    }

    // A state the keyboard never produced: cursor moved, selection, or the
    // application changed the text. The client is the source of truth.
    m_predicted.clear();
    m_confirmed = before;
    m_model = before;

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
            const QString trimmed = before.trimmed();
            const QChar last = trimmed.back();
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
    commitLocal(formatted);
    m_currentWord = formatted;
    m_sentenceStart = false;
    finalizeCurrentWord();
    commitLocal(QStringLiteral(" "));
    m_lastActionWasSpace = true;
    refreshSuggestions();
    return formatted;
}

}
