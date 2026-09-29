// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

namespace V3Keyboard
{

class LocalLexicon
{
public:
    LocalLexicon();

    void setLanguage(const QString &code);
    QString language() const;

    QStringList suggestions(const QString &word, const QString &previousWord, int limit = 3) const;
    QString bestCorrection(const QString &word, const QString &previousWord = {}) const;
    QStringList nextWords(const QString &previousWord, int limit = 3) const;
    QString decodeGlide(const QStringList &trace, const QString &previousWord = {}) const;

    void learnWord(const QString &word);
    void learnBigram(const QString &previousWord, const QString &word);
    void learnWordWithContext(const QString &word, const QString &previousWord);
    void clearLearning();

    bool hasWord(const QString &word) const;
    int dictionarySize() const;

private:
    void reload();
    void loadSystemDictionary();
    void loadFallbackWords();
    void loadLearning();
    void persistLearning() const;
    void addWord(const QString &word);
    QString normalize(const QString &word) const;
    QString alphabet() const;
    QSet<QString> oneEditCandidates(const QString &word) const;
    int scoreCandidate(const QString &candidate, const QString &typed, const QString &previousWord, bool prefix) const;
    QStringList ranked(const QSet<QString> &candidates, const QString &typed, const QString &previousWord, bool prefix, int limit) const;
    QString bigramKey(const QString &previousWord, const QString &word) const;

    QString m_language = QStringLiteral("en");
    QSet<QString> m_words;
    QHash<QString, QStringList> m_prefixIndex;
    QHash<QString, int> m_personalFrequency;
    QHash<QString, int> m_bigramFrequency;
};

}
