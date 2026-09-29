// SPDX-License-Identifier: GPL-3.0-or-later

#include "locallexicon.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QVector>

#include <hunspell/hunspell.hxx>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>

Q_LOGGING_CATEGORY(lcLexicon, "v3keyboard.lexicon", QtWarningMsg)

namespace V3Keyboard
{

// Immutable once built; created on a worker thread, then only used on the
// thread that owns the LocalLexicon.
struct DictionaryData
{
    QString language;
    std::unique_ptr<Hunspell> speller;
    QByteArray encoding;
    // Compact completion index: all lowercase words concatenated, plus one
    // sorted offset per word. Bit 31 marks words the dictionary capitalizes.
    QString blob;
    std::vector<quint32> entries;

    // Word frequency (sorted by word for prefix search); rank 1 = most common.
    struct FreqEntry { QString word; quint32 rank; bool capital; };
    std::vector<FreqEntry> freq;

    std::pair<size_t, size_t> freqRange(QStringView prefix) const
    {
        auto less = [](const FreqEntry &e, QStringView p) { return QStringView(e.word).compare(p) < 0; };
        const auto lo = std::lower_bound(freq.begin(), freq.end(), prefix, less);
        const QString upper = prefix.toString() + QChar(0xFFFF);
        const auto hi = std::lower_bound(lo, freq.end(), QStringView(upper), less);
        return {size_t(lo - freq.begin()), size_t(hi - freq.begin())};
    }

    const FreqEntry *freqFind(QStringView word) const
    {
        const auto [lo, hi] = freqRange(word);
        return lo < hi && freq[lo].word == word ? &freq[lo] : nullptr;
    }

    static constexpr quint32 CapitalFlag = 0x80000000u;

    QStringView wordAt(size_t i) const
    {
        const quint32 start = entries[i] & ~CapitalFlag;
        qsizetype end = blob.indexOf(QLatin1Char('\n'), start);
        return QStringView(blob).mid(start, end - start);
    }
    bool capitalizedAt(size_t i) const { return entries[i] & CapitalFlag; }

    std::pair<size_t, size_t> prefixRange(QStringView prefix) const
    {
        auto less = [this](quint32 entry, QStringView p) {
            const quint32 start = entry & ~CapitalFlag;
            const qsizetype end = blob.indexOf(QLatin1Char('\n'), start);
            return QStringView(blob).mid(start, end - start).compare(p) < 0;
        };
        const auto lo = std::lower_bound(entries.begin(), entries.end(), prefix, less);
        const QString upper = prefix.toString() + QChar(0xFFFF);
        const auto hi = std::lower_bound(lo, entries.end(), QStringView(upper), less);
        return {size_t(lo - entries.begin()), size_t(hi - entries.begin())};
    }

    bool contains(QStringView word) const
    {
        const auto [lo, hi] = prefixRange(word);
        return lo < hi && wordAt(lo) == word;
    }

    QByteArray encode(const QString &word) const
    {
        if (encoding == "UTF-8") return word.toUtf8();
        if (encoding == "ISO8859-1" || encoding == "ISO-8859-1") return word.toLatin1();
        QStringEncoder encoder(encoding.constData());
        if (!encoder.isValid()) return {};
        return encoder.encode(word);
    }

    bool spell(const QString &word) const
    {
        if (!speller || word.isEmpty()) return false;
        const QByteArray bytes = encode(word);
        return !bytes.isEmpty() && speller->spell(std::string(bytes.constData(), bytes.size()));
    }
};

namespace
{

QStringList coreWords(const QString &code)
{
    if (code == QStringLiteral("de")) {
        return {
            QStringLiteral("aber"), QStringLiteral("alle"), QStringLiteral("auch"), QStringLiteral("auf"),
            QStringLiteral("aus"), QStringLiteral("bei"), QStringLiteral("bin"), QStringLiteral("bitte"),
            QStringLiteral("danke"), QStringLiteral("das"), QStringLiteral("dein"), QStringLiteral("der"),
            QStringLiteral("die"), QStringLiteral("du"), QStringLiteral("ein"), QStringLiteral("eine"),
            QStringLiteral("für"), QStringLiteral("gut"), QStringLiteral("hallo"), QStringLiteral("haben"),
            QStringLiteral("heute"), QStringLiteral("hier"), QStringLiteral("ich"), QStringLiteral("ist"),
            QStringLiteral("ja"), QStringLiteral("kann"), QStringLiteral("machen"), QStringLiteral("mein"),
            QStringLiteral("mit"), QStringLiteral("morgen"), QStringLiteral("nicht"), QStringLiteral("noch"),
            QStringLiteral("oder"), QStringLiteral("schon"), QStringLiteral("sehr"), QStringLiteral("sein"),
            QStringLiteral("sie"), QStringLiteral("so"), QStringLiteral("und"), QStringLiteral("uns"),
            QStringLiteral("was"), QStringLiteral("wenn"), QStringLiteral("wie"), QStringLiteral("wir"),
            QStringLiteral("wo"), QStringLiteral("wollen"), QStringLiteral("zeit"), QStringLiteral("zu")
        };
    }
    if (code == QStringLiteral("uk")) {
        return {
            QStringLiteral("але"), QStringLiteral("без"), QStringLiteral("буде"), QStringLiteral("бути"),
            QStringLiteral("вам"), QStringLiteral("вас"), QStringLiteral("вже"), QStringLiteral("він"),
            QStringLiteral("вона"), QStringLiteral("вони"), QStringLiteral("все"), QStringLiteral("гарно"),
            QStringLiteral("де"), QStringLiteral("для"), QStringLiteral("добре"), QStringLiteral("дякую"),
            QStringLiteral("є"), QStringLiteral("зараз"), QStringLiteral("і"), QStringLiteral("його"),
            QStringLiteral("коли"), QStringLiteral("може"), QStringLiteral("можна"), QStringLiteral("ми"),
            QStringLiteral("мене"), QStringLiteral("на"), QStringLiteral("нам"), QStringLiteral("не"),
            QStringLiteral("ні"), QStringLiteral("потрібно"), QStringLiteral("привіт"), QStringLiteral("про"),
            QStringLiteral("так"), QStringLiteral("тебе"), QStringLiteral("тепер"), QStringLiteral("ти"),
            QStringLiteral("тут"), QStringLiteral("у"), QStringLiteral("хочу"), QStringLiteral("це"),
            QStringLiteral("цей"), QStringLiteral("час"), QStringLiteral("що"), QStringLiteral("як"),
            QStringLiteral("я"), QStringLiteral("який"), QStringLiteral("якщо")
        };
    }
    if (code == QStringLiteral("ru")) {
        return {
            QStringLiteral("а"), QStringLiteral("без"), QStringLiteral("будет"), QStringLiteral("быть"),
            QStringLiteral("вам"), QStringLiteral("вас"), QStringLiteral("всё"), QStringLiteral("вы"),
            QStringLiteral("где"), QStringLiteral("да"), QStringLiteral("для"), QStringLiteral("добро"),
            QStringLiteral("есть"), QStringLiteral("ещё"), QStringLiteral("здесь"), QStringLiteral("и"),
            QStringLiteral("как"), QStringLiteral("когда"), QStringLiteral("может"), QStringLiteral("можно"),
            QStringLiteral("мы"), QStringLiteral("мне"), QStringLiteral("на"), QStringLiteral("нам"),
            QStringLiteral("не"), QStringLiteral("нет"), QStringLiteral("но"), QStringLiteral("он"),
            QStringLiteral("она"), QStringLiteral("они"), QStringLiteral("очень"), QStringLiteral("по"),
            QStringLiteral("пока"), QStringLiteral("потом"), QStringLiteral("привет"), QStringLiteral("про"),
            QStringLiteral("сейчас"), QStringLiteral("спасибо"), QStringLiteral("так"), QStringLiteral("тебя"),
            QStringLiteral("ты"), QStringLiteral("у"), QStringLiteral("хочу"), QStringLiteral("что"),
            QStringLiteral("это"), QStringLiteral("я"), QStringLiteral("если")
        };
    }

    return {
        QStringLiteral("a"), QStringLiteral("about"), QStringLiteral("after"), QStringLiteral("all"),
        QStringLiteral("also"), QStringLiteral("and"), QStringLiteral("are"), QStringLiteral("as"),
        QStringLiteral("at"), QStringLiteral("be"), QStringLiteral("because"), QStringLiteral("but"),
        QStringLiteral("can"), QStringLiteral("do"), QStringLiteral("for"), QStringLiteral("from"),
        QStringLiteral("good"), QStringLiteral("have"), QStringLiteral("hello"), QStringLiteral("here"),
        QStringLiteral("how"), QStringLiteral("i"), QStringLiteral("if"), QStringLiteral("in"),
        QStringLiteral("is"), QStringLiteral("it"), QStringLiteral("just"), QStringLiteral("know"),
        QStringLiteral("like"), QStringLiteral("me"), QStringLiteral("more"), QStringLiteral("my"),
        QStringLiteral("need"), QStringLiteral("no"), QStringLiteral("not"), QStringLiteral("now"),
        QStringLiteral("of"), QStringLiteral("on"), QStringLiteral("one"), QStringLiteral("or"),
        QStringLiteral("please"), QStringLiteral("so"), QStringLiteral("thank"), QStringLiteral("that"),
        QStringLiteral("the"), QStringLiteral("there"), QStringLiteral("this"), QStringLiteral("time"),
        QStringLiteral("to"), QStringLiteral("today"), QStringLiteral("want"), QStringLiteral("was"),
        QStringLiteral("we"), QStringLiteral("what"), QStringLiteral("when"), QStringLiteral("where"),
        QStringLiteral("will"), QStringLiteral("with"), QStringLiteral("yes"), QStringLiteral("you"),
        QStringLiteral("your")
    };
}

// Only strings that are never valid words go here: without a dictionary they
// are the sole autocorrections, so they must be unambiguous.
QHash<QString, QString> curatedTypos(const QString &code)
{
    QHash<QString, QString> map;
    auto add = [&map](const char *typo, const char *word) {
        map.insert(QString::fromUtf8(typo), QString::fromUtf8(word));
    };
    if (code == QStringLiteral("de")) {
        add("dsa", "das"); add("udn", "und"); add("nciht", "nicht"); add("ncith", "nicht");
        add("hlalo", "hallo"); add("danek", "danke"); add("iwe", "wie");
    } else if (code == QStringLiteral("uk")) {
        add("првиіт", "привіт"); add("дяукю", "дякую");
        add("щоо", "що"); add("якщко", "якщо");
    } else if (code == QStringLiteral("ru")) {
        add("првиет", "привет"); add("пирвет", "привет"); add("спаисбо", "спасибо");
        add("сапсибо", "спасибо"); add("сейчса", "сейчас"); add("очнеь", "очень");
    } else {
        add("teh", "the"); add("hte", "the"); add("adn", "and"); add("taht", "that");
        add("waht", "what"); add("thier", "their"); add("recieve", "receive");
        add("becuase", "because"); add("wiht", "with"); add("thsi", "this");
    }
    return map;
}

QStringList dictionaryNames(const QString &code)
{
    if (code == QStringLiteral("de")) {
        return {QStringLiteral("de_DE"), QStringLiteral("de_DE_frami"), QStringLiteral("de_AT"), QStringLiteral("de_CH")};
    }
    if (code == QStringLiteral("uk")) return {QStringLiteral("uk_UA")};
    if (code == QStringLiteral("ru")) return {QStringLiteral("ru_RU")};
    return {QStringLiteral("en_US"), QStringLiteral("en_GB")};
}

QString capitalizeFirst(const QString &word)
{
    if (word.isEmpty()) return word;
    QString result = word;
    result[0] = result.at(0).toUpper();
    return result;
}

QStringList &searchPathOverride()
{
    static QStringList paths;
    return paths;
}

QStringList &frequencyPathOverride()
{
    static QStringList paths;
    return paths;
}

void loadFrequency(DictionaryData &data, const QString &language, const QStringList &paths)
{
    QFile file;
    for (const QString &dir : paths) {
        file.setFileName(dir + QLatin1Char('/') + language + QStringLiteral(".txt"));
        if (file.exists()) break;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const QString text = QString::fromUtf8(file.readAll());
    quint32 rank = 0;
    for (QStringView line : QStringView(text).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QString word = line.trimmed().toString();
        if (word.isEmpty()) continue;
        ++rank;
        bool capital = false;
        if (data.speller) {
            // Keep only real words; German nouns and names are valid only
            // capitalized, so remember that for display.
            if (!data.spell(word)) {
                if (!data.spell(capitalizeFirst(word))) continue;
                capital = true;
            }
        }
        data.freq.push_back({word, rank, capital});
    }
    std::sort(data.freq.begin(), data.freq.end(), [](const auto &a, const auto &b) { return a.word < b.word; });
    data.freq.erase(std::unique(data.freq.begin(), data.freq.end(), [](const auto &a, const auto &b) { return a.word == b.word; }), data.freq.end());
    data.freq.shrink_to_fit();
}

bool isWordText(QStringView word)
{
    if (word.isEmpty() || word.size() > 48) return false;
    for (const QChar ch : word) {
        if (!ch.isLetter() && ch != QLatin1Char('\'') && ch != QLatin1Char('-')) return false;
    }
    return true;
}

QString decodeText(const QByteArray &raw, const QByteArray &encoding)
{
    if (encoding.isEmpty() || encoding == "UTF-8") return QString::fromUtf8(raw);
    if (encoding == "ISO8859-1" || encoding == "ISO-8859-1") return QString::fromLatin1(raw);
    QStringDecoder decoder(encoding.constData());
    if (!decoder.isValid()) return {};
    return decoder.decode(raw);
}

std::shared_ptr<DictionaryData> loadDictionary(const QString &language, const QStringList &searchPaths,
                                               const QStringList &frequencyPaths)
{
    QElapsedTimer timer;
    timer.start();
    auto data = std::make_shared<DictionaryData>();
    data->language = language;

    QString base;
    for (const QString &name : dictionaryNames(language)) {
        for (const QString &dir : searchPaths) {
            const QString candidate = dir + QLatin1Char('/') + name;
            if (QFileInfo::exists(candidate + QStringLiteral(".dic")) && QFileInfo::exists(candidate + QStringLiteral(".aff"))) {
                base = candidate;
                break;
            }
        }
        if (!base.isEmpty()) break;
    }
    if (base.isEmpty()) {
        qCInfo(lcLexicon) << "no dictionary for" << language;
        loadFrequency(*data, language, frequencyPaths);
        return data;
    }

    const QByteArray aff = QFile::encodeName(base + QStringLiteral(".aff"));
    const QByteArray dic = QFile::encodeName(base + QStringLiteral(".dic"));
    data->speller = std::make_unique<Hunspell>(aff.constData(), dic.constData());
    data->encoding = QByteArray(data->speller->get_dict_encoding().c_str()).toUpper();

    QFile file(QString::fromLocal8Bit(dic));
    if (!file.open(QIODevice::ReadOnly)) return data;
    const QString text = decodeText(file.readAll(), data->encoding);

    struct Item { QString word; bool capital; };
    std::vector<Item> items;
    items.reserve(text.count(QLatin1Char('\n')));
    bool first = true;
    for (QStringView line : QStringView(text).split(QLatin1Char('\n'))) {
        if (first) { first = false; continue; }          // entry count
        if (line.isEmpty() || line.front().isSpace() || line.front() == QLatin1Char('#')) continue;
        qsizetype cut = line.size();
        for (qsizetype i = 0; i < line.size(); ++i) {
            const QChar ch = line.at(i);
            if ((ch == QLatin1Char('/') && (i == 0 || line.at(i - 1) != QLatin1Char('\\'))) || ch.isSpace()) { cut = i; break; }
        }
        const QStringView word = line.left(cut);
        if (!isWordText(word)) continue;
        const QString lower = word.toString().toLower();
        if (word.size() > 1 && word.toString().toUpper() == word) continue;   // acronyms
        items.push_back({lower, word.front().isUpper()});
    }
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) {
        const int c = a.word.compare(b.word);
        return c < 0 || (c == 0 && !a.capital && b.capital);
    });

    qsizetype total = 0;
    for (const Item &item : items) total += item.word.size() + 1;
    data->blob.reserve(total);
    data->entries.reserve(items.size());
    const QString *previous = nullptr;
    for (const Item &item : items) {
        if (previous && *previous == item.word) continue;   // lowercase variant sorted first wins
        data->entries.push_back(quint32(data->blob.size()) | (item.capital ? DictionaryData::CapitalFlag : 0));
        data->blob += item.word;
        data->blob += QLatin1Char('\n');
        previous = &item.word;
    }
    data->blob.squeeze();
    loadFrequency(*data, language, frequencyPaths);
    qCInfo(lcLexicon) << "loaded" << language << data->entries.size() << "entries," << data->freq.size()
                      << "frequency words in" << timer.elapsed() << "ms";
    return data;
}


bool isAdjacentTransposition(const QString &a, const QString &b)
{
    if (a.size() != b.size() || a.size() < 2) return false;
    int first = -1;
    for (int i = 0; i < a.size(); ++i) {
        if (a.at(i) != b.at(i)) { first = i; break; }
    }
    if (first < 0 || first + 1 >= a.size()) return false;
    return a.at(first) == b.at(first + 1) && a.at(first + 1) == b.at(first)
        && QStringView(a).mid(first + 2) == QStringView(b).mid(first + 2);
}

bool differsByRepeatedLetter(const QString &shorter, const QString &longer)
{
    if (longer.size() != shorter.size() + 1) return false;
    for (int i = 0; i < longer.size(); ++i) {
        QString removed = longer;
        removed.remove(i, 1);
        if (removed == shorter) {
            return (i > 0 && longer.at(i - 1) == longer.at(i))
                || (i + 1 < longer.size() && longer.at(i + 1) == longer.at(i));
        }
    }
    return false;
}

int levenshtein(const QString &a, const QString &b)
{
    QVector<int> prev(b.size() + 1);
    QVector<int> cur(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j) prev[j] = j;
    for (int i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (int j = 1; j <= b.size(); ++j) {
            const int cost = a.at(i - 1) == b.at(j - 1) ? 0 : 1;
            cur[j] = qMin(qMin(cur[j - 1] + 1, prev[j] + 1), prev[j - 1] + cost);
        }
        prev.swap(cur);
    }
    return prev[b.size()];
}

QString collapsed(const QString &word)
{
    QString result;
    for (const QChar ch : word) {
        if (result.isEmpty() || result.back() != ch) result.append(ch);
    }
    return result;
}

}

LocalLexicon::LocalLexicon()
{
    m_coreWords = coreWords(m_language);
    m_typoMap = curatedTypos(m_language);
}

LocalLexicon::~LocalLexicon()
{
    flushLearning();
    if (m_pending.valid()) m_pending.wait();
}

void LocalLexicon::setDictionarySearchPaths(const QStringList &paths)
{
    searchPathOverride() = paths;
}

void LocalLexicon::setFrequencySearchPaths(const QStringList &paths)
{
    frequencyPathOverride() = paths;
}

QStringList LocalLexicon::frequencySearchPaths()
{
    if (!frequencyPathOverride().isEmpty()) return frequencyPathOverride();
    return {QStringLiteral(":/v3keyboard/frequency")};
}

int LocalLexicon::frequencyRank(const QString &word) const
{
    if (!m_data) return 0;
    const auto *entry = m_data->freqFind(word);
    return entry ? int(entry->rank) : 0;
}

QStringList LocalLexicon::dictionarySearchPaths()
{
    if (!searchPathOverride().isEmpty()) return searchPathOverride();
    const QString env = qEnvironmentVariable("V3KBD_DICTIONARY_DIRS");
    if (!env.isEmpty()) return env.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    return {
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/v3-keyboard/dictionaries"),
        QStringLiteral("/usr/share/hunspell"),
        QStringLiteral("/usr/share/myspell/dicts"),
    };
}

void LocalLexicon::setLanguage(const QString &code)
{
    if (code == m_language && m_loadStarted) return;
    if (code != m_language) flushLearning();
    m_language = code;
    m_coreWords = coreWords(code);
    m_typoMap = curatedTypos(code);
    loadLearning();
    startLoading();
}

QString LocalLexicon::language() const { return m_language; }

void LocalLexicon::startLoading()
{
    m_loadStarted = true;
    // Release the previous language right away: one dictionary in memory.
    m_data.reset();
    std::promise<std::shared_ptr<DictionaryData>> promise;
    m_pending = promise.get_future();
    std::thread([promise = std::move(promise), language = m_language, paths = dictionarySearchPaths(),
                 freqPaths = frequencySearchPaths()]() mutable {
        promise.set_value(loadDictionary(language, paths, freqPaths));
    }).detach();
}

void LocalLexicon::adoptLoadedData() const
{
    if (!m_pending.valid()) return;
    if (m_pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    auto data = m_pending.get();
    if (data && data->language == m_language) m_data = std::move(data);
}

bool LocalLexicon::waitForDictionary(int timeoutMs)
{
    if (!m_loadStarted) startLoading();
    if (m_pending.valid()) {
        if (timeoutMs < 0) m_pending.wait();
        else if (m_pending.wait_for(std::chrono::milliseconds(timeoutMs)) != std::future_status::ready) return false;
    }
    adoptLoadedData();
    return true;
}

bool LocalLexicon::hasSystemDictionary() const
{
    adoptLoadedData();
    return m_data && m_data->speller;
}

QString LocalLexicon::normalize(const QString &word) const
{
    QString w = word.trimmed().toLower();
    while (!w.isEmpty() && !w.front().isLetter()) w.remove(0, 1);
    while (!w.isEmpty() && !w.back().isLetter()) w.chop(1);
    return w;
}

QString LocalLexicon::alphabet() const
{
    if (m_language == QStringLiteral("de")) return QStringLiteral("abcdefghijklmnopqrstuvwxyzäöüß");
    if (m_language == QStringLiteral("uk")) return QStringLiteral("абвгґдеєжзиіїйклмнопрстуфхцчшщьюя'");
    if (m_language == QStringLiteral("ru")) return QStringLiteral("абвгдеёжзийклмнопрстуфхцчшщъыьэюя");
    return QStringLiteral("abcdefghijklmnopqrstuvwxyz");
}

bool LocalLexicon::isCore(const QString &word) const { return m_coreWords.contains(word); }

bool LocalLexicon::isValidWord(const QString &word) const
{
    if (word.isEmpty()) return false;
    if (m_personalFrequency.value(word) > 0 || isCore(word)) return true;
    adoptLoadedData();
    if (!m_data) return false;
    if (m_data->speller) {
        // Hunspell is case-sensitive: German nouns and names are stored
        // capitalized, so a lowercase "haus" is still a real word.
        return m_data->spell(word) || m_data->spell(capitalizeFirst(word));
    }
    return m_data->contains(word) || m_data->freqFind(word);
}

QString LocalLexicon::bigramKey(const QString &previousWord, const QString &word) const
{
    return previousWord + QChar(0x001f) + word;
}

int LocalLexicon::priorScore(const QString &candidate, const QString &previousWord) const
{
    int score = 0;
    // The small built-in core list is only a prior when no frequency data exists.
    if (isCore(candidate) && (!m_data || m_data->freq.empty())) score += 150;
    if (const int rank = frequencyRank(candidate); rank > 0) {
        // 360 for the most common word, falling logarithmically to 0 at 50k.
        score += qMax(0, int(360.0 * (1.0 - std::log(double(rank)) / std::log(50001.0))));
    }
    const int personal = m_personalFrequency.value(candidate);
    if (personal > 0) {
        // Personal use outweighs the generic core list after a few repetitions.
        score += qMin(450, 100 + personal * 40);
        // Bigrams only exist for learned words; skip the key allocation otherwise.
        if (!previousWord.isEmpty()) score += qMin(600, m_bigramFrequency.value(bigramKey(previousWord, candidate)) * 60);
    }
    return score;
}

QList<LocalLexicon::Candidate> LocalLexicon::correctionCandidates(const QString &typed, const QString &previousWord) const
{
    QSet<QString> variants;
    const QString chars = alphabet();
    for (int i = 0; i < typed.size(); ++i) {
        QString v = typed; v.remove(i, 1); variants.insert(v);
    }
    for (int i = 0; i + 1 < typed.size(); ++i) {
        QString v = typed; std::swap(v[i], v[i + 1]); variants.insert(v);
    }
    for (int i = 0; i < typed.size(); ++i) {
        for (const QChar ch : chars) {
            if (typed.at(i) == ch) continue;
            QString v = typed; v[i] = ch; variants.insert(v);
        }
    }
    for (int i = 0; i <= typed.size(); ++i) {
        for (const QChar ch : chars) {
            QString v = typed; v.insert(i, ch); variants.insert(v);
        }
    }
    variants.remove(typed);

    QList<Candidate> result;
    for (const QString &v : variants) {
        if (v.isEmpty() || !isValidWord(v)) continue;
        Candidate c;
        c.word = v;
        if (isAdjacentTransposition(typed, v)) c.edit = Edit::Transposition;
        else if (differsByRepeatedLetter(typed, v) || differsByRepeatedLetter(v, typed)) c.edit = Edit::RepeatedLetter;
        c.score = 800 + priorScore(v, previousWord) - qAbs(v.size() - typed.size()) * 8;
        if (c.edit == Edit::Transposition) c.score += 150;
        if (c.edit == Edit::RepeatedLetter) c.score += 100;
        result.append(c);
    }
    std::sort(result.begin(), result.end(), [](const Candidate &a, const Candidate &b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.word.size() != b.word.size()) return a.word.size() < b.word.size();
        return a.word < b.word;
    });
    return result;
}

QStringList LocalLexicon::suggestions(const QString &word, const QString &previousWord, int limit) const
{
    const QString typed = normalize(word);
    if (typed.isEmpty() || limit <= 0) return {};
    const QString prev = normalize(previousWord);
    adoptLoadedData();

    QHash<QString, int> scored;
    QSet<QString> capitalized;
    auto consider = [&](const QString &candidate, bool capital) {
        if (scored.contains(candidate)) return;
        int score = 1200 + priorScore(candidate, prev) - qAbs(candidate.size() - typed.size()) * 8;
        // The typed fragment itself only leads when it is a real, common word
        // ("he"), not an abbreviation-like fragment ("h", "пр").
        if (candidate == typed) {
            const int rank = frequencyRank(candidate);
            if (typed.size() > 2 || (rank > 0 && rank <= 300) || m_personalFrequency.value(candidate) > 0) score += 500;
            else score -= 300;
        }
        if (capital) { score -= 60; capitalized.insert(candidate); }
        scored.insert(candidate, score);
    };

    for (auto it = m_personalFrequency.constBegin(); it != m_personalFrequency.constEnd(); ++it) {
        if (it.key().startsWith(typed)) consider(it.key(), false);
    }
    for (const QString &core : m_coreWords) {
        if (core.startsWith(typed)) consider(core, false);
    }
    if (m_data && !m_data->freq.empty()) {
        // Only the most frequent completions can win; pick them by rank with
        // a cheap integer pass before the full scoring.
        const auto [lo, hi] = m_data->freqRange(typed);
        std::vector<size_t> picks;
        picks.reserve(hi - lo);
        for (size_t i = lo; i < hi; ++i) picks.push_back(i);
        const size_t keep = qMin<size_t>(picks.size(), 12);
        std::partial_sort(picks.begin(), picks.begin() + keep, picks.end(),
                          [this](size_t a, size_t b) { return m_data->freq[a].rank < m_data->freq[b].rank; });
        for (size_t k = 0; k < keep; ++k) consider(m_data->freq[picks[k]].word, m_data->freq[picks[k]].capital);
    }
    if (m_data && typed.size() >= 2 && m_data->freq.empty()) {
        // Dictionary stems carry no frequency, so single-letter prefixes would
        // only produce alphabetical noise at a high scan cost.
        const auto [lo, hi] = m_data->prefixRange(typed);
        const size_t end = qMin(hi, lo + 400);
        for (size_t i = lo; i < end; ++i) consider(m_data->wordAt(i).toString(), m_data->capitalizedAt(i));
    }

    QList<std::pair<QString, int>> ordered(scored.constKeyValueBegin(), scored.constKeyValueEnd());
    std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second) return a.second > b.second;
        if (a.first.size() != b.first.size()) return a.first.size() < b.first.size();
        return a.first < b.first;
    });
    QStringList result;
    for (const auto &item : ordered) {
        if (result.size() >= limit) break;
        result.append(capitalized.contains(item.first) ? capitalizeFirst(item.first) : item.first);
    }
    if (result.size() < limit && typed.size() >= 3 && !isValidWord(typed)) {
        for (const Candidate &c : correctionCandidates(typed, prev)) {
            if (result.size() >= limit) break;
            if (!result.contains(c.word)) result.append(c.word);
        }
    }
    return result;
}

QString LocalLexicon::bestCorrection(const QString &word, const QString &previousWord) const
{
    const QString typed = normalize(word);
    // Two-character tokens are too ambiguous for automatic replacement.
    if (typed.size() < 3) return {};
    // Words the user has kept (typed and committed, or restored by undoing a
    // correction) are never rewritten, not even from the curated typo list.
    if (m_personalFrequency.value(typed) > 0) return {};
    const auto typo = m_typoMap.constFind(typed);
    if (typo != m_typoMap.constEnd()) return typo.value();

    // Without a real dictionary there is no way to know that the typed word
    // is wrong; rewriting "knows" or "form" would corrupt valid text.
    if (!hasSystemDictionary()) return {};
    if (isValidWord(typed)) return {};

    const QList<Candidate> candidates = correctionCandidates(typed, normalize(previousWord));
    if (candidates.isEmpty()) return {};
    const Candidate &best = candidates.first();
    const int rank = frequencyRank(best.word);
    const int margin = candidates.size() > 1 ? best.score - candidates.at(1).score : 1000;
    const bool confident = best.edit == Edit::Transposition
        || isCore(best.word)
        || m_personalFrequency.value(best.word) >= 2
        || (rank > 0 && rank <= 3000 && margin >= 60);
    if (!confident) return {};
    if (margin == 0) return {};
    return best.word;
}

QStringList LocalLexicon::nextWords(const QString &previousWord, int limit) const
{
    const QString prev = normalize(previousWord);
    if (prev.isEmpty() || limit <= 0) return {};
    struct Scored { QString word; int score; };
    QVector<Scored> scored;
    const QString prefix = prev + QChar(0x001f);
    for (auto it = m_bigramFrequency.constBegin(); it != m_bigramFrequency.constEnd(); ++it) {
        if (!it.key().startsWith(prefix)) continue;
        scored.push_back({it.key().mid(prefix.size()), it.value()});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.word < b.word;
    });
    QStringList result;
    for (const auto &item : scored) {
        if (result.size() >= limit) break;
        result.append(item.word);
    }
    return result;
}

QString LocalLexicon::decodeGlide(const QStringList &trace, const QString &previousWord) const
{
    if (trace.isEmpty()) return {};
    QString sequence;
    for (const QString &part : trace) {
        const QString normalized = normalize(part);
        if (!normalized.isEmpty()) sequence += normalized.front();
    }
    sequence = collapsed(sequence);
    if (sequence.size() < 2) return {};
    adoptLoadedData();
    const QString prev = normalize(previousWord);

    QSet<QString> candidates;
    auto consider = [&](const QString &word) {
        if (word.isEmpty() || word.front() != sequence.front() || word.back() != sequence.back()) return;
        if (qAbs(word.size() - sequence.size()) > qMax(3, int(sequence.size() / 2))) return;
        candidates.insert(word);
    };
    for (auto it = m_personalFrequency.constBegin(); it != m_personalFrequency.constEnd(); ++it) consider(it.key());
    for (const QString &core : m_coreWords) consider(core);
    if (m_data) {
        const auto [lo, hi] = m_data->prefixRange(sequence.left(1));
        for (size_t i = lo; i < hi; ++i) {
            const QStringView w = m_data->wordAt(i);
            if (!w.isEmpty() && w.back() == sequence.back()) consider(w.toString());
        }
    }

    QString best;
    int bestScore = -100000;
    for (const QString &word : candidates) {
        const int distance = levenshtein(collapsed(word), sequence);
        int score = 1000 - distance * 120 - qAbs(word.size() - sequence.size()) * 12;
        score += qMin(240, m_personalFrequency.value(word) * 20);
        if (isCore(word)) score += 60;
        if (!prev.isEmpty()) score += qMin(480, m_bigramFrequency.value(bigramKey(prev, word)) * 60);
        if (score > bestScore || (score == bestScore && word < best)) {
            bestScore = score;
            best = word;
        }
    }
    return bestScore >= 520 ? best : QString();
}

void LocalLexicon::loadLearning()
{
    m_personalFrequency.clear();
    m_bigramFrequency.clear();
    m_unsavedLearning = 0;
    QSettings settings;
    const QVariantMap words = settings.value(QStringLiteral("learning/%1/words").arg(m_language)).toMap();
    for (auto it = words.constBegin(); it != words.constEnd(); ++it) {
        const QString word = normalize(it.key());
        const int count = it.value().toInt();
        if (!word.isEmpty() && count > 0) m_personalFrequency.insert(word, count);
    }
    const QVariantMap bigrams = settings.value(QStringLiteral("learning/%1/bigrams").arg(m_language)).toMap();
    for (auto it = bigrams.constBegin(); it != bigrams.constEnd(); ++it) {
        if (it.value().toInt() > 0) m_bigramFrequency.insert(it.key(), it.value().toInt());
    }
}

void LocalLexicon::learnWordWithContext(const QString &word, const QString &previousWord)
{
    const QString w = normalize(word);
    if (w.size() < 2 || w.size() > 48) return;
    m_personalFrequency[w] = qMin(1000, m_personalFrequency.value(w) + 1);
    const QString prev = normalize(previousWord);
    if (!prev.isEmpty()) {
        const QString key = bigramKey(prev, w);
        m_bigramFrequency[key] = qMin(1000, m_bigramFrequency.value(key) + 1);
    }
    // Batch disk writes: one settings flush per 16 words (and on context or
    // language change / shutdown) instead of one per word.
    if (++m_unsavedLearning >= 16) flushLearning();
}

void LocalLexicon::flushLearning()
{
    if (m_unsavedLearning == 0) return;
    QVariantMap words;
    for (auto it = m_personalFrequency.constBegin(); it != m_personalFrequency.constEnd(); ++it) words.insert(it.key(), it.value());
    QVariantMap bigrams;
    for (auto it = m_bigramFrequency.constBegin(); it != m_bigramFrequency.constEnd(); ++it) bigrams.insert(it.key(), it.value());
    QSettings settings;
    settings.setValue(QStringLiteral("learning/%1/words").arg(m_language), words);
    settings.setValue(QStringLiteral("learning/%1/bigrams").arg(m_language), bigrams);
    m_unsavedLearning = 0;
}

void LocalLexicon::clearLearning()
{
    m_personalFrequency.clear();
    m_bigramFrequency.clear();
    m_unsavedLearning = 0;
    QSettings settings;
    settings.remove(QStringLiteral("learning/%1").arg(m_language));
}

bool LocalLexicon::hasWord(const QString &word) const { return isValidWord(normalize(word)); }

int LocalLexicon::dictionarySize() const
{
    adoptLoadedData();
    return int(m_coreWords.size()) + (m_data ? int(m_data->entries.size()) : 0);
}

}
