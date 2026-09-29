// SPDX-License-Identifier: GPL-3.0-or-later

#include "typingengine.h"
#include "keyboardcontroller.h"

namespace V3Keyboard
{

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

bool TypingEngine::suggestionsEnabled() const { return m_suggestionsEnabled; }
bool TypingEngine::autocorrectEnabled() const { return m_autocorrectEnabled; }
bool TypingEngine::learningEnabled() const { return m_learningEnabled; }
bool TypingEngine::autoCapitalizationEnabled() const { return m_autoCapitalizationEnabled; }
bool TypingEngine::doubleSpacePeriodEnabled() const { return m_doubleSpacePeriodEnabled; }
bool TypingEngine::wantsAutoUppercase() const { return !m_sensitiveContext && m_autoCapitalizationEnabled && m_sentenceStart && m_currentWord.isEmpty(); }
bool TypingEngine::sensitiveContext() const { return m_sensitiveContext; }
QString TypingEngine::currentWord() const { return m_currentWord; }
QString TypingEngine::previousWord() const { return m_previousWord; }
QStringList TypingEngine::suggestions() const { return m_suggestions; }
QStringList TypingEngine::glideTrace() const { return m_glideTrace; }

QString TypingEngine::formatForSentence(const QString &word) const
{
    if (!m_autoCapitalizationEnabled || !m_sentenceStart || word.isEmpty()) return word;
    QString result = word;
    result[0] = result.at(0).toUpper();
    return result;
}

void TypingEngine::typeLetter(const QString &text)
{
    if (text.isEmpty()) return;
    markLocalEdit();
    m_controller.tapText(text);
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
    markLocalEdit();
    if (m_sensitiveContext) {
        m_controller.tapText(text);
        m_currentWord.clear();
        m_suggestions.clear();
        m_lastActionWasSpace = false;
        return;
    }

    const bool punctuation = text.size() == 1 && QStringLiteral(".,!?;:").contains(text);
    if (punctuation && !m_currentWord.isEmpty()) {
        finalizeCurrentWord();
    }

    m_controller.tapText(text);
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

void TypingEngine::markLocalEdit()
{
    m_localEditPending = true;
}

void TypingEngine::replaceCommittedCurrentWord(const QString &replacement)
{
    if (m_currentWord.isEmpty() || replacement.isEmpty()) return;
    for (int i = 0; i < m_currentWord.size(); ++i) m_controller.backspace();
    m_controller.tapText(replacement);
    m_currentWord = replacement;
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
    markLocalEdit();
    if (m_sensitiveContext) {
        m_controller.tapText(QStringLiteral(" "));
        m_currentWord.clear();
        m_suggestions.clear();
        return;
    }
    if (m_currentWord.isEmpty()) {
        if (m_doubleSpacePeriodEnabled && m_lastActionWasSpace && !m_previousWord.isEmpty()) {
            m_controller.backspace();
            m_controller.tapText(QStringLiteral(". "));
            m_sentenceStart = true;
            m_sentencePunctuationPending = false;
            m_lastActionWasSpace = false;
            refreshSuggestions();
            return;
        }
        m_controller.tapText(QStringLiteral(" "));
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
            replaceCommittedCurrentWord(formatted);
        }
    }

    finalizeCurrentWord();
    m_controller.tapText(QStringLiteral(" "));
    m_lastActionWasSpace = true;
    if (m_sentencePunctuationPending) {
        m_sentenceStart = true;
        m_sentencePunctuationPending = false;
    }
    refreshSuggestions();
}

void TypingEngine::backspace()
{
    markLocalEdit();
    m_controller.backspace();
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
    markLocalEdit();
    for (int i = 0; i < bounded; ++i) m_controller.backspace();

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
    markLocalEdit();
    if (!m_currentWord.isEmpty()) finalizeCurrentWord();
    m_controller.enter();
    m_sentenceStart = true;
    m_sentencePunctuationPending = false;
    m_lastActionWasSpace = false;
    refreshSuggestions();
}

void TypingEngine::chooseSuggestion(const QString &word)
{
    if (word.isEmpty() || m_sensitiveContext) return;
    markLocalEdit();
    QString selected = word;
    if (!m_currentWord.isEmpty() && m_currentWord.front().isUpper()) {
        selected[0] = selected.at(0).toUpper();
    } else if (m_sentenceStart) {
        selected = formatForSentence(selected);
    }

    if (!m_currentWord.isEmpty()) {
        replaceCommittedCurrentWord(selected);
    } else {
        m_controller.tapText(selected);
        m_currentWord = selected;
    }
    finalizeCurrentWord();
    m_controller.tapText(QStringLiteral(" "));
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
    m_localEditPending = false;
    refreshSuggestions();
}

void TypingEngine::setSensitiveContext(bool sensitive)
{
    if (m_sensitiveContext == sensitive) return;
    m_sensitiveContext = sensitive;
    resetComposition();
}

bool TypingEngine::syncSurroundingText(const QString &text, int cursorByte, int anchorByte)
{
    if (m_sensitiveContext || cursorByte < 0 || anchorByte < 0 || cursorByte != anchorByte) return false;

    const QByteArray utf8 = text.toUtf8();
    const int bounded = qBound(0, cursorByte, static_cast<int>(utf8.size()));
    const QString before = QString::fromUtf8(utf8.left(bounded));

    int end = before.size();
    int start = end;
    while (start > 0 && before.at(start - 1).isLetter()) --start;
    const QString trailing = before.mid(start);

    int prevEnd = start;
    while (prevEnd > 0 && !before.at(prevEnd - 1).isLetter()) --prevEnd;
    int prevStart = prevEnd;
    while (prevStart > 0 && before.at(prevStart - 1).isLetter()) --prevStart;

    const QString incomingPrevious = before.mid(prevStart, prevEnd - prevStart).toLower();
    bool incomingSentenceStart = before.trimmed().isEmpty();
    if (!incomingSentenceStart && trailing.isEmpty()) {
        const QString trimmed = before.trimmed();
        if (!trimmed.isEmpty()) {
            const QChar last = trimmed.back();
            incomingSentenceStart = last == QLatin1Char('.') || last == QLatin1Char('!') || last == QLatin1Char('?');
        }
    }

    // Chromium/Qt clients can echo surrounding-text snapshots from before our
    // most recent commit. Do not let a stale echo overwrite the authoritative
    // local composition, otherwise autocorrect/suggestion replacement randomly
    // loses the word currently being typed.
    if (m_localEditPending) {
        if (!m_currentWord.isEmpty()) {
            if (trailing != m_currentWord) {
                return false;
            }
            m_localEditPending = false;
        } else if (m_lastActionWasSpace) {
            if (!trailing.isEmpty()) {
                return false;
            }
            m_localEditPending = false;
        } else if (!trailing.isEmpty()) {
            return false;
        } else {
            m_localEditPending = false;
        }
    }

    if (m_currentWord == trailing
        && m_previousWord == incomingPrevious
        && m_sentenceStart == incomingSentenceStart) {
        return false;
    }

    m_currentWord = trailing;
    m_previousWord = incomingPrevious;
    m_sentenceStart = incomingSentenceStart;
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
    markLocalEdit();
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
    m_controller.tapText(formatted);
    m_currentWord = formatted;
    m_sentenceStart = false;
    finalizeCurrentWord();
    m_controller.tapText(QStringLiteral(" "));
    m_lastActionWasSpace = true;
    refreshSuggestions();
    return formatted;
}

}
