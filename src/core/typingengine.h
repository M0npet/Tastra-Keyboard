// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "locallexicon.h"

#include <QString>
#include <QStringList>

namespace V3Keyboard
{

class KeyboardController;

class TypingEngine
{
public:
    explicit TypingEngine(KeyboardController &controller);

    void setLanguage(const QString &code);
    QString language() const;

    void setSuggestionsEnabled(bool enabled);
    void setAutocorrectEnabled(bool enabled);
    void setLearningEnabled(bool enabled);
    void setAutoCapitalizationEnabled(bool enabled);
    void setDoubleSpacePeriodEnabled(bool enabled);
    void setAutoCapitalizationAllowed(bool allowed);

    bool suggestionsEnabled() const;
    bool autocorrectEnabled() const;
    bool learningEnabled() const;
    bool autoCapitalizationEnabled() const;
    bool doubleSpacePeriodEnabled() const;

    bool wantsAutoUppercase() const;
    bool sensitiveContext() const;
    QString currentWord() const;
    QString previousWord() const;
    QStringList suggestions() const;

    void typeLetter(const QString &text);
    void typeText(const QString &text);
    void backspace();
    void backspaceRepeated(int count);
    void space();
    void enter();
    void chooseSuggestion(const QString &word);
    void resetComposition();
    void resetInputContext();
    bool surroundingTextSupported() const;
    void setSensitiveContext(bool sensitive);
    bool syncSurroundingText(const QString &text, int cursorByte, int anchorByte);
    void clearLearning();

    void beginGlide(const QString &key);
    void glideThrough(const QString &key);
    QString endGlide();
    QStringList glideTrace() const;

private:
    QString formatForSentence(const QString &word) const;
    void refreshSuggestions();
    bool replaceBeforeCursor(const QString &existing, const QString &replacement, bool allowUnconfirmed);
    void finalizeCurrentWord(bool updatePrevious = true);
    void observePunctuation(const QString &text);

    // Local model of the text before the cursor, used to tell stale client
    // echoes (states this keyboard produced earlier) from external edits.
    void commitLocal(const QString &text);
    void backspaceLocal();
    bool deleteLocal(const QString &text);
    void recordLocalState();
    void forgetTextState();
    bool inSyncWithClient() const;

    KeyboardController &m_controller;
    LocalLexicon m_lexicon;
    QString m_currentWord;
    QString m_previousWord;
    QStringList m_suggestions;
    QStringList m_glideTrace;
    bool m_suggestionsEnabled = true;
    bool m_autocorrectEnabled = true;
    bool m_learningEnabled = true;
    bool m_autoCapitalizationEnabled = true;
    bool m_doubleSpacePeriodEnabled = true;
    bool m_sentenceStart = true;
    bool m_lastActionWasSpace = false;
    bool m_sentencePunctuationPending = false;
    bool m_sensitiveContext = false;
    bool m_autoCapitalizationAllowed = true;
    bool m_surroundingSupported = false;
    QString m_model;
    QString m_confirmed;
    QStringList m_predicted;
};

}
