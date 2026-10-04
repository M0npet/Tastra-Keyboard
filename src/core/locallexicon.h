// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QStringList>

#include <future>
#include <memory>

namespace V3Keyboard
{

struct DictionaryData;

// Local, offline word knowledge for one language at a time.
//
// * Validity ("is this a real word?") comes from libhunspell, which understands
//   affix rules, so inflected forms (wants, делаешь, gehst) are valid.
// * Completions come from a compact sorted index of the dictionary stems plus
//   the built-in core vocabulary and the user's locally learned words.
// * Only the active language is kept in memory. Loading happens on a worker
//   thread; until it finishes, core vocabulary and learned words still work.
class LocalLexicon
{
public:
    LocalLexicon();
    ~LocalLexicon();
    LocalLexicon(const LocalLexicon &) = delete;
    LocalLexicon &operator=(const LocalLexicon &) = delete;

    // Directories searched for <lang>.aff/<lang>.dic. Defaults to the
    // V3KBD_DICTIONARY_DIRS environment variable (colon separated) or the
    // standard system locations. Tests point this at fixtures.
    static void setDictionarySearchPaths(const QStringList &paths);
    static QStringList dictionarySearchPaths();
    // Directories (or qrc prefixes) searched for <lang>.txt word-frequency
    // lists, most frequent word first. Default: the bundled lists.
    static void setFrequencySearchPaths(const QStringList &paths);
    static QStringList frequencySearchPaths();
    // Offensive-word lists (<lang>.txt), used only to filter suggestions.
    static void setBlocklistSearchPaths(const QStringList &paths);
    static QStringList blocklistSearchPaths();

    void setBlockOffensive(bool enabled);
    // Rows of the on-screen layout. Like LatinIME's ProximityInfo, a typo on a
    // neighbouring key is a cheaper (more likely) error than a distant one.
    void setKeyboardRows(const QStringList &rows);
    // Gboard-style "remove suggestion": unlearn and never suggest it again.
    void forgetWord(const QString &word);

    void setLanguage(const QString &code);
    QString language() const;
    bool waitForDictionary(int timeoutMs = -1);
    bool hasSystemDictionary() const;

    QStringList suggestions(const QString &word, const QString &previousWord, int limit = 3) const;
    QString bestCorrection(const QString &word, const QString &previousWord = {}) const;
    // Cheap per-keystroke version: candidates are checked against the
    // frequency list, stems, learned and core words only (no Hunspell), so a
    // long unknown word cannot stall typing. Space uses bestCorrection().
    QString correctionPreview(const QString &word, const QString &previousWord = {}) const;
    QStringList nextWords(const QString &previousWord, int limit = 3) const;
    QString decodeGlide(const QStringList &trace, const QString &previousWord = {}) const;

    void learnWordWithContext(const QString &word, const QString &previousWord);
    void flushLearning();
    void clearLearning();

    bool hasWord(const QString &word) const;
    int dictionarySize() const;

private:
    enum class Edit { Other, RepeatedLetter, Transposition, NeighbourKey };
    struct Candidate {
        QString word;
        int score = 0;
        Edit edit = Edit::Other;
    };

    void startLoading();
    void adoptLoadedData() const;
    void loadLearning();
    QString normalize(const QString &word) const;
    QString alphabet() const;
    bool isCore(const QString &word) const;
    bool isValidWord(const QString &word) const;
    QList<Candidate> correctionCandidates(const QString &typed, const QString &previousWord, bool full = true) const;
    QString pickCorrection(const QString &word, const QString &previousWord, bool full) const;
    bool knownCheaply(const QString &word) const;
    int priorScore(const QString &candidate, const QString &previousWord) const;
    int frequencyRank(const QString &word) const;
    bool suggestible(const QString &word) const;
    bool neighbours(QChar a, QChar b) const;
    void loadBlocklist();
    QString bigramKey(const QString &previousWord, const QString &word) const;

    QString m_language = QStringLiteral("en");
    bool m_loadStarted = false;
    mutable std::future<std::shared_ptr<DictionaryData>> m_pending;
    mutable std::shared_ptr<DictionaryData> m_data;
    QStringList m_coreWords;
    QHash<QString, QString> m_typoMap;
    QHash<QString, int> m_personalFrequency;
    QHash<QString, int> m_bigramFrequency;
    int m_unsavedLearning = 0;
    bool m_blockOffensive = true;
    QSet<QString> m_offensive;
    QSet<QString> m_forgotten;
    QHash<QChar, QPointF> m_keyCentres;
};

}
