// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyboarduibridge.h"

#include <QGuiApplication>
#include <QStyleHints>
#include <QStandardPaths>
#include <QFile>
#include "voicecontroller.h"
#include "keyboardlayoutmetrics.h"

#include "core/keyboardcontroller.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QtGlobal>

namespace V3Keyboard
{
namespace
{

QClipboard *systemClipboard()
{
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) return nullptr;
    return QGuiApplication::clipboard();
}

}

KeyboardUiBridge::KeyboardUiBridge(
    KeyboardController &controller,
    KeyboardModel &model,
    QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_model(model)
    , m_typingEngine(controller)
    , m_toolbarRegistry(ToolbarRegistry::createDefault())
    , m_toolbarModel(m_toolbarRegistry)
{
    QSettings settings;
    m_model.setLanguage(settings.value(QStringLiteral("language"), QStringLiteral("en")).toString());
    m_typingEngine.setLanguage(m_model.languageCode());
    m_emojiCatalog.setKeywordLanguage(m_model.languageCode());
    m_typingEngine.setKeyboardRows({m_model.row1().join(QString()), m_model.row2().join(QString()), m_model.row3().join(QString())});
    if (m_voice) m_voice->setLanguage(m_model.languageCode());
    m_amoled = settings.value(QStringLiteral("amoled"), false).toBool();
    {
        const QString stored = settings.value(QStringLiteral("theme")).toString();
        if (stored == QStringLiteral("system") || stored == QStringLiteral("light")
            || stored == QStringLiteral("dark") || stored == QStringLiteral("amoled")) {
            m_theme = stored;
        } else if (m_amoled) {
            m_theme = QStringLiteral("amoled");        // migrate the pre-1.0 toggle
        }
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, &KeyboardUiBridge::uiPreferencesChanged);
    }
#endif
    loadShortcuts();
    m_emojiSuggestions = settings.value(QStringLiteral("emojiSuggestions"), true).toBool();
    m_autoSpaceAfterPunctuation = settings.value(QStringLiteral("autoSpaceAfterPunctuation"), false).toBool();
    m_typingEngine.setAutoSpaceAfterPunctuation(m_autoSpaceAfterPunctuation);
    m_emojiRow = settings.value(QStringLiteral("emojiRow"), false).toBool();
    m_symbolHints = settings.value(QStringLiteral("symbolHints"), true).toBool();
    m_numberRow = settings.value(QStringLiteral("numberRow"), false).toBool();
    m_blockOffensive = settings.value(QStringLiteral("blockOffensive"), true).toBool();
    m_typingEngine.setBlockOffensive(m_blockOffensive);
    {
        const QString mode = settings.value(QStringLiteral("layoutMode"), QStringLiteral("full")).toString();
        if (mode == QStringLiteral("left") || mode == QStringLiteral("right")) m_layoutMode = mode;
    }
    m_keyScale = qBound(0.85, settings.value(QStringLiteral("keyScale"), 1.0).toDouble(), 1.20);
    m_keyBorders = settings.value(QStringLiteral("keyBorders"), true).toBool();
    m_keyPopups = settings.value(QStringLiteral("keyPopups"), true).toBool();
    m_glideEnabled = settings.value(QStringLiteral("glideEnabled"), true).toBool();

    m_typingEngine.setSuggestionsEnabled(settings.value(QStringLiteral("suggestionsEnabled"), true).toBool());
    m_typingEngine.setAutocorrectEnabled(settings.value(QStringLiteral("autocorrectEnabled"), true).toBool());
    m_typingEngine.setLearningEnabled(settings.value(QStringLiteral("learningEnabled"), true).toBool());
    m_typingEngine.setAutoCapitalizationEnabled(settings.value(QStringLiteral("autoCapitalizationEnabled"), true).toBool());
    m_typingEngine.setDoubleSpacePeriodEnabled(settings.value(QStringLiteral("doubleSpacePeriodEnabled"), true).toBool());
    m_typingEngine.setCompositionEnabled(settings.value(QStringLiteral("compositionEnabled"), true).toBool());

    if (auto *clipboard = systemClipboard()) {
        captureClipboard();
        connect(clipboard, &QClipboard::dataChanged, this, [this]() {
            captureClipboard();
            Q_EMIT clipboardChanged();
        });
    }
}

bool KeyboardUiBridge::uppercase() const { return m_model.uppercase() || m_typingEngine.wantsAutoUppercase(); }
bool KeyboardUiBridge::capsLock() const { return m_model.capsLock(); }
bool KeyboardUiBridge::symbolsActive() const { return m_model.symbolsActive(); }
QString KeyboardUiBridge::languageCode() const { return m_model.languageCode(); }
QString KeyboardUiBridge::languageLabel() const { return m_model.languageLabel(); }
QStringList KeyboardUiBridge::row1() const { return m_model.row1(); }
QStringList KeyboardUiBridge::row2() const { return m_model.row2(); }
QStringList KeyboardUiBridge::row3() const { return m_model.row3(); }
QString KeyboardUiBridge::activePanel() const { return panelIdName(m_panelManager.activePanel()); }
QStringList KeyboardUiBridge::toolbarActionIds() const { return m_toolbarModel.visibleActionIds(); }
QStringList KeyboardUiBridge::languageCodes() const { return m_model.languageCodes(); }
QStringList KeyboardUiBridge::languageLabels() const { return m_model.languageLabels(); }
QString KeyboardUiBridge::currentWord() const { return m_typingEngine.currentWord(); }
QString KeyboardUiBridge::autocorrectSuggestion() const { return m_typingEngine.autocorrectTarget(); }
QString KeyboardUiBridge::inputPurpose() const { return m_inputPurpose; }

void KeyboardUiBridge::insertEmoji(const QString &glyph)
{
    tapText(glyph);
    m_emojiCatalog.noteUsed(glyph);
    Q_EMIT emojiChanged();
}

// Gboard "personal dictionary" shortcuts: one "shortcut<TAB>expansion" or
// "shortcut = expansion" per line in ~/.config/v3-keyboard/shortcuts.txt.
void KeyboardUiBridge::loadShortcuts()
{
    m_shortcuts.clear();
    QString path = qEnvironmentVariable("V3KBD_SHORTCUTS_FILE");
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
            + QStringLiteral("/v3-keyboard/shortcuts.txt");
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const QString text = QString::fromUtf8(file.readAll());
    for (QString line : text.split(QLatin1Char('\n'))) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
        qsizetype sep = line.indexOf(QLatin1Char('\t'));
        if (sep < 0) sep = line.indexOf(QLatin1Char('='));
        if (sep <= 0) continue;
        const QString key = line.left(sep).trimmed().toLower();
        const QString value = line.mid(sep + 1).trimmed();
        if (!key.isEmpty() && !value.isEmpty()) m_shortcuts.insert(key, value);
    }
}

QStringList KeyboardUiBridge::suggestions() const
{
    QStringList list = m_typingEngine.suggestions();
    const QString expansion = m_shortcuts.value(m_typingEngine.currentWord().toLower());
    if (!expansion.isEmpty()) {
        list.removeAll(expansion);
        list.prepend(expansion);
        while (list.size() > 3) list.removeLast();
        return list;
    }
    const QString target = m_typingEngine.autocorrectTarget();
    if (!target.isEmpty()) {
        // Gboard order: what was typed (kept on tap), then the correction in
        // the middle, then the best remaining alternative.
        const QString typed = m_typingEngine.currentWord();
        QStringList ordered = {typed, target};
        for (const QString &item : list) {
            if (ordered.size() >= 3) break;
            if (!ordered.contains(item)) ordered.append(item);
        }
        list = ordered;
    }
    if (m_emojiSuggestions && !m_typingEngine.currentWord().isEmpty()) {
        const QString emoji = m_emojiCatalog.emojiForWord(m_typingEngine.currentWord());
        if (!emoji.isEmpty()) {
            if (list.size() >= 3) list.removeLast();
            list.append(emoji);
        }
    }
    return list;
}

bool KeyboardUiBridge::emojiSuggestionsEnabled() const { return m_emojiSuggestions; }
bool KeyboardUiBridge::autoSpaceAfterPunctuation() const { return m_autoSpaceAfterPunctuation; }
bool KeyboardUiBridge::emojiRow() const { return m_emojiRow; }
QStringList KeyboardUiBridge::recentEmojis() const { return m_emojiCatalog.glyphs(QStringLiteral("Recent")).mid(0, 10); }

void KeyboardUiBridge::setAutoSpaceAfterPunctuation(bool enabled)
{
    if (m_autoSpaceAfterPunctuation == enabled) return;
    m_autoSpaceAfterPunctuation = enabled;
    m_typingEngine.setAutoSpaceAfterPunctuation(enabled);
    persistPreference(QStringLiteral("autoSpaceAfterPunctuation"), enabled);
    Q_EMIT typingPreferencesChanged();
}

void KeyboardUiBridge::setEmojiRow(bool enabled)
{
    if (m_emojiRow == enabled) return;
    m_emojiRow = enabled;
    persistPreference(QStringLiteral("emojiRow"), enabled);
    Q_EMIT uiPreferencesChanged();
}
bool KeyboardUiBridge::symbolHints() const { return m_symbolHints; }
bool KeyboardUiBridge::numberRow() const { return m_numberRow; }

void KeyboardUiBridge::setSymbolHints(bool enabled)
{
    if (m_symbolHints == enabled) return;
    m_symbolHints = enabled;
    persistPreference(QStringLiteral("symbolHints"), enabled);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::setNumberRow(bool enabled)
{
    if (m_numberRow == enabled) return;
    m_numberRow = enabled;
    persistPreference(QStringLiteral("numberRow"), enabled);
    Q_EMIT uiPreferencesChanged();
}

QStringList KeyboardUiBridge::alternatesForKey(const QString &key) const
{
    QStringList list = m_model.alternatesForKey(key);
    if (key == QStringLiteral(".") && (m_inputPurpose == QStringLiteral("url") || m_inputPurpose == QStringLiteral("email"))) {
        // Gboard: long-press the period in address fields for domains.
        const QStringList domains = {QStringLiteral(".com"), QStringLiteral(".de"), QStringLiteral(".ru"),
                                     QStringLiteral(".ua"), QStringLiteral(".org"), QStringLiteral(".net")};
        list = domains + list;
    }
    return list;
}

QString KeyboardUiBridge::symbolHintForKey(const QString &key) const
{
    if (!m_symbolHints) return {};
    for (const QString &choice : m_model.alternatesForKey(key)) {
        if (!choice.isEmpty() && !choice.front().isLetter()) return choice;
    }
    return {};
}

void KeyboardUiBridge::tapAlternateText(const QString &text)
{
    if (text.isEmpty()) return;
    const bool uppercaseBefore = uppercase();
    if (text.front().isLetter()) m_typingEngine.typeLetter(text);
    else m_typingEngine.typeText(text);
    if (text.front().isLetter()) m_model.consumeShiftAfterLetter();
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}
bool KeyboardUiBridge::blockOffensive() const { return m_blockOffensive; }

void KeyboardUiBridge::setEmojiSuggestionsEnabled(bool enabled)
{
    if (m_emojiSuggestions == enabled) return;
    m_emojiSuggestions = enabled;
    persistPreference(QStringLiteral("emojiSuggestions"), enabled);
    Q_EMIT typingPreferencesChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::setBlockOffensive(bool enabled)
{
    if (m_blockOffensive == enabled) return;
    m_blockOffensive = enabled;
    m_typingEngine.setBlockOffensive(enabled);
    persistPreference(QStringLiteral("blockOffensive"), enabled);
    Q_EMIT typingPreferencesChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::forgetSuggestion(const QString &word)
{
    m_typingEngine.forgetWord(word);
    Q_EMIT suggestionsChanged();
}

QVariantList KeyboardUiBridge::toolbarActions() const
{
    QVariantList result;
    for (const auto &action : m_toolbarModel.visibleActions()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), action.id);
        item.insert(QStringLiteral("label"), action.label);
        item.insert(QStringLiteral("iconName"), action.iconName);
        item.insert(QStringLiteral("enabled"), action.enabled);
        result.append(item);
    }
    return result;
}

bool KeyboardUiBridge::amoled() const { return effectiveTheme() == QStringLiteral("amoled"); }
QString KeyboardUiBridge::theme() const { return m_theme; }
QString KeyboardUiBridge::version() const { return QStringLiteral(V3KBD_VERSION); }

QString KeyboardUiBridge::effectiveTheme() const
{
    if (m_theme != QStringLiteral("system")) return m_theme;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())
        && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light) {
        return QStringLiteral("light");
    }
#endif
    return QStringLiteral("dark");
}

void KeyboardUiBridge::setTheme(const QString &theme)
{
    if (theme != QStringLiteral("system") && theme != QStringLiteral("light")
        && theme != QStringLiteral("dark") && theme != QStringLiteral("amoled")) return;
    if (m_theme == theme) return;
    m_theme = theme;
    persistPreference(QStringLiteral("theme"), theme);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::cycleTheme()
{
    static const QStringList order = {QStringLiteral("system"), QStringLiteral("light"),
                                      QStringLiteral("dark"), QStringLiteral("amoled")};
    setTheme(order.at((order.indexOf(m_theme) + 1) % order.size()));
}
QString KeyboardUiBridge::layoutMode() const { return m_layoutMode; }
bool KeyboardUiBridge::voiceBuilt() const { return m_voice != nullptr; }
QString KeyboardUiBridge::voiceState() const { return m_voice ? m_voice->state() : QStringLiteral("unavailable"); }
QString KeyboardUiBridge::voiceMessage() const { return m_voice ? m_voice->message() : QString(); }

void KeyboardUiBridge::setVoiceController(VoiceController *voice)
{
    m_voice = voice;
    if (!m_voice) return;
    m_voice->setLanguage(m_model.languageCode());
    connect(m_voice, &VoiceController::stateChanged, this, &KeyboardUiBridge::voiceChanged);
    connect(m_voice, &VoiceController::textRecognized, this, [this](const QString &text) {
        const bool uppercaseBefore = uppercase();
        m_typingEngine.insertDictation(text);
        if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
        Q_EMIT suggestionsChanged();
    });
    Q_EMIT voiceChanged();
}

void KeyboardUiBridge::toggleVoice()
{
    if (m_voice) m_voice->toggle();
}
double KeyboardUiBridge::keyScale() const { return m_keyScale; }
bool KeyboardUiBridge::keyBorders() const { return m_keyBorders; }
bool KeyboardUiBridge::keyPopups() const { return m_keyPopups; }
bool KeyboardUiBridge::suggestionsEnabled() const { return m_typingEngine.suggestionsEnabled(); }
bool KeyboardUiBridge::autocorrectEnabled() const { return m_typingEngine.autocorrectEnabled(); }
bool KeyboardUiBridge::learningEnabled() const { return m_typingEngine.learningEnabled(); }
bool KeyboardUiBridge::autoCapitalizationEnabled() const { return m_typingEngine.autoCapitalizationEnabled(); }
bool KeyboardUiBridge::doubleSpacePeriodEnabled() const { return m_typingEngine.doubleSpacePeriodEnabled(); }
bool KeyboardUiBridge::glideEnabled() const { return m_glideEnabled; }
bool KeyboardUiBridge::compositionEnabled() const { return m_typingEngine.compositionEnabled(); }
bool KeyboardUiBridge::secureInput() const { return m_secureInput; }
bool KeyboardUiBridge::clipboardHistoryEnabled() const { return m_clipboardHistory.enabled(); }

QString KeyboardUiBridge::clipboardText() const
{
    if (auto *clipboard = systemClipboard()) return clipboard->text();
    return {};
}

QStringList KeyboardUiBridge::clipboardHistory() const { return m_clipboardHistory.items(); }
QStringList KeyboardUiBridge::emojiItems() const { return m_emojiCatalog.glyphs(QStringLiteral("All"), {}, 240); }
QStringList KeyboardUiBridge::emojiCategories() const { return m_emojiCatalog.categories(); }
QStringList KeyboardUiBridge::emojiSearch(const QString &query, const QString &category) const
{
    return m_emojiCatalog.glyphs(category, query, 240);
}

QVariantMap KeyboardUiBridge::layoutMetrics(bool portrait) const
{
    auto metrics = portrait ? KeyboardLayoutMetrics::portrait() : KeyboardLayoutMetrics::landscape();
    metrics.keyHeight *= m_keyScale;
    metrics.fontSize *= qBound(0.92, m_keyScale, 1.12);
    metrics.popupHeight *= m_keyScale;
    metrics.keyRadius *= qBound(0.95, m_keyScale, 1.08);
    return {
        {QStringLiteral("contentWidthRatio"), metrics.contentWidthRatio},
        {QStringLiteral("maxContentWidth"), metrics.maxContentWidth},
        {QStringLiteral("keyHeight"), metrics.keyHeight},
        {QStringLiteral("keyGap"), metrics.keyGap},
        {QStringLiteral("outerMargin"), metrics.outerMargin},
        {QStringLiteral("topPadding"), metrics.topPadding},
        {QStringLiteral("bottomPadding"), metrics.bottomPadding},
        {QStringLiteral("keyRadius"), metrics.keyRadius},
        {QStringLiteral("fontSize"), metrics.fontSize},
        {QStringLiteral("popupHeight"), metrics.popupHeight},
        {QStringLiteral("panelHeight"), metrics.panelHeight()},
    };
}

QString KeyboardUiBridge::alternateForKey(const QString &text) const
{
    QString alternate = m_model.alternateForKey(text);
    if (!alternate.isEmpty() && uppercase()) alternate = alternate.toUpper();
    return alternate;
}

void KeyboardUiBridge::typingStateDidChange()
{
    Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::tapLetter(const QString &letter)
{
    const bool oneShotWasActive = m_model.uppercase() && !m_model.capsLock();
    const bool autoUpperWasActive = m_typingEngine.wantsAutoUppercase();
    const QString output = uppercase() ? letter.toUpper() : letter.toLower();
    m_typingEngine.typeLetter(output);
    m_model.consumeShiftAfterLetter();

    // Normal typing changes suggestions on every tap, but it does not need to
    // invalidate every row/key binding. Keeping keyboardStateChanged off the
    // hot path removes a large amount of QML work in Chromium text fields.
    if (oneShotWasActive || autoUpperWasActive) Q_EMIT keyboardStateChanged();
    if (m_typingEngine.suggestionsEnabled()) Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::tapAlternate(const QString &base)
{
    QString alternate = m_model.alternateForKey(base);
    if (alternate.isEmpty()) return;
    const bool autoUpperWasActive = m_typingEngine.wantsAutoUppercase();
    if (uppercase()) alternate = alternate.toUpper();
    const bool oneShotWasActive = m_model.uppercase() && !m_model.capsLock();
    m_typingEngine.typeLetter(alternate);
    m_model.consumeShiftAfterLetter();
    if (oneShotWasActive || autoUpperWasActive) Q_EMIT keyboardStateChanged();
    if (m_typingEngine.suggestionsEnabled()) Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::tapText(const QString &text)
{
    const bool uppercaseBefore = uppercase();
    m_typingEngine.typeText(text);
    // Gboard (16.7+, on by default): an apostrophe on the symbols layer
    // switches straight back to letters to finish "don't" / "it's".
    if (text == QStringLiteral("'") && m_model.symbolsActive()) {
        toggleSymbols();
        return;
    }
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::selectSuggestion(const QString &word)
{
    const bool uppercaseBefore = uppercase();
    if (!word.isEmpty() && !word.front().isLetter()) {
        // Emoji suggestion: keep the typed word, add the emoji after it.
        m_typingEngine.commitComposition();
        m_typingEngine.typeText(QStringLiteral(" ") + word);
    } else {
        m_typingEngine.chooseSuggestion(word);
    }
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::shift()
{
    m_model.pressShift();
    Q_EMIT keyboardStateChanged();
}

void KeyboardUiBridge::toggleSymbols()
{
    m_model.toggleSymbols();
    Q_EMIT keyboardStateChanged();
}

void KeyboardUiBridge::nextLanguage()
{
    m_typingEngine.commitComposition();
    m_model.nextLanguage();
    m_typingEngine.setLanguage(m_model.languageCode());
    m_emojiCatalog.setKeywordLanguage(m_model.languageCode());
    m_typingEngine.setKeyboardRows({m_model.row1().join(QString()), m_model.row2().join(QString()), m_model.row3().join(QString())});
    if (m_voice) m_voice->setLanguage(m_model.languageCode());
    persistLanguage();
    typingStateDidChange();
}

void KeyboardUiBridge::setLanguage(const QString &code)
{
    m_typingEngine.commitComposition();
    m_model.setLanguage(code);
    m_typingEngine.setLanguage(m_model.languageCode());
    m_emojiCatalog.setKeywordLanguage(m_model.languageCode());
    m_typingEngine.setKeyboardRows({m_model.row1().join(QString()), m_model.row2().join(QString()), m_model.row3().join(QString())});
    if (m_voice) m_voice->setLanguage(m_model.languageCode());
    persistLanguage();
    typingStateDidChange();
}

void KeyboardUiBridge::activateToolbarAction(const QString &id)
{
    if (m_toolbarModel.activate(id, m_panelManager)) {
        if (id == QStringLiteral("clipboard")) Q_EMIT clipboardChanged();
        Q_EMIT toolbarStateChanged();
    }
}

void KeyboardUiBridge::openLanguagePanel()
{
    if (m_panelManager.openPanel(PanelId::Language)) Q_EMIT toolbarStateChanged();
}

void KeyboardUiBridge::closePanel()
{
    if (m_panelManager.closePanel()) Q_EMIT toolbarStateChanged();
}

void KeyboardUiBridge::space()
{
    const bool uppercaseBefore = uppercase();
    m_typingEngine.space();
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::backspace()
{
    const bool uppercaseBefore = uppercase();
    m_typingEngine.backspace();
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::backspaceRepeated(int count)
{
    const bool uppercaseBefore = uppercase();
    m_typingEngine.backspaceRepeated(count);
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::deleteForward()
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.deleteForward();
    typingStateDidChange();
}

void KeyboardUiBridge::moveLeft()
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.moveLeft();
    typingStateDidChange();
}

void KeyboardUiBridge::moveRight()
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.moveRight();
    typingStateDidChange();
}

void KeyboardUiBridge::moveCursor(int delta)
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    const int bounded = qBound(-48, delta, 48);
    if (bounded < 0) for (int i = 0; i < -bounded; ++i) m_controller.moveLeft();
    if (bounded > 0) for (int i = 0; i < bounded; ++i) m_controller.moveRight();
    typingStateDidChange();
}

void KeyboardUiBridge::moveHome()
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.moveHome();
    typingStateDidChange();
}

void KeyboardUiBridge::moveEnd()
{
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.moveEnd();
    typingStateDidChange();
}

void KeyboardUiBridge::enter()
{
    const bool uppercaseBefore = uppercase();
    m_typingEngine.enter();
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::beginGlide(const QString &key)
{
    if (m_glideEnabled && !m_model.symbolsActive()) m_typingEngine.beginGlide(key);
}

void KeyboardUiBridge::glideThrough(const QString &key)
{
    if (m_glideEnabled && !m_model.symbolsActive()) m_typingEngine.glideThrough(key);
}

QString KeyboardUiBridge::endGlide()
{
    if (!m_glideEnabled || m_model.symbolsActive()) return {};
    const bool uppercaseBefore = uppercase();
    const QString word = m_typingEngine.endGlide();
    if (!word.isEmpty()) {
        if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
        Q_EMIT suggestionsChanged();
    }
    return word;
}

void KeyboardUiBridge::captureClipboard()
{
    m_clipboardHistory.capture(clipboardText());
}

void KeyboardUiBridge::pasteClipboard()
{
    const QString text = clipboardText();
    if (!text.isEmpty()) {
        m_typingEngine.commitComposition();
        m_typingEngine.resetComposition();
        m_controller.tapText(text);
        Q_EMIT suggestionsChanged();
    }
}

void KeyboardUiBridge::pasteClipboardHistory(int index)
{
    const auto items = m_clipboardHistory.items();
    if (index < 0 || index >= items.size()) return;
    m_typingEngine.commitComposition();
    m_typingEngine.resetComposition();
    m_controller.tapText(items.at(index));
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::removeClipboardHistory(int index)
{
    m_clipboardHistory.removeAt(index);
    Q_EMIT clipboardChanged();
}

void KeyboardUiBridge::clearClipboard()
{
    if (auto *clipboard = systemClipboard()) clipboard->clear();
    Q_EMIT clipboardChanged();
}

void KeyboardUiBridge::clearClipboardHistory()
{
    m_clipboardHistory.clear();
    Q_EMIT clipboardChanged();
}

void KeyboardUiBridge::setAmoled(bool enabled)
{
    // Kept for compatibility; themes are chosen with setTheme().
    setTheme(enabled ? QStringLiteral("amoled") : QStringLiteral("dark"));
}

void KeyboardUiBridge::setLayoutMode(const QString &mode)
{
    if (mode != QStringLiteral("full") && mode != QStringLiteral("left") && mode != QStringLiteral("right")) return;
    if (m_layoutMode == mode) return;
    m_layoutMode = mode;
    persistPreference(QStringLiteral("layoutMode"), mode);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::cycleLayoutMode()
{
    setLayoutMode(m_layoutMode == QStringLiteral("full") ? QStringLiteral("left")
                  : m_layoutMode == QStringLiteral("left") ? QStringLiteral("right")
                                                            : QStringLiteral("full"));
}

void KeyboardUiBridge::setKeyScale(double scale)
{
    const double bounded = qBound(0.85, scale, 1.20);
    if (qAbs(m_keyScale - bounded) < 0.001) return;
    m_keyScale = bounded;
    persistPreference(QStringLiteral("keyScale"), bounded);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::setKeyBorders(bool enabled)
{
    if (m_keyBorders == enabled) return;
    m_keyBorders = enabled;
    persistPreference(QStringLiteral("keyBorders"), enabled);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::setKeyPopups(bool enabled)
{
    if (m_keyPopups == enabled) return;
    m_keyPopups = enabled;
    persistPreference(QStringLiteral("keyPopups"), enabled);
    Q_EMIT uiPreferencesChanged();
}

void KeyboardUiBridge::setSuggestionsEnabled(bool enabled)
{
    if (m_typingEngine.suggestionsEnabled() == enabled) return;
    m_typingEngine.setSuggestionsEnabled(enabled);
    persistPreference(QStringLiteral("suggestionsEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::setAutocorrectEnabled(bool enabled)
{
    if (m_typingEngine.autocorrectEnabled() == enabled) return;
    m_typingEngine.setAutocorrectEnabled(enabled);
    persistPreference(QStringLiteral("autocorrectEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
}

void KeyboardUiBridge::setLearningEnabled(bool enabled)
{
    if (m_typingEngine.learningEnabled() == enabled) return;
    m_typingEngine.setLearningEnabled(enabled);
    persistPreference(QStringLiteral("learningEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
}

void KeyboardUiBridge::setAutoCapitalizationEnabled(bool enabled)
{
    if (m_typingEngine.autoCapitalizationEnabled() == enabled) return;
    m_typingEngine.setAutoCapitalizationEnabled(enabled);
    persistPreference(QStringLiteral("autoCapitalizationEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
    Q_EMIT keyboardStateChanged();
}

void KeyboardUiBridge::setDoubleSpacePeriodEnabled(bool enabled)
{
    if (m_typingEngine.doubleSpacePeriodEnabled() == enabled) return;
    m_typingEngine.setDoubleSpacePeriodEnabled(enabled);
    persistPreference(QStringLiteral("doubleSpacePeriodEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
}

void KeyboardUiBridge::setGlideEnabled(bool enabled)
{
    if (m_glideEnabled == enabled) return;
    m_glideEnabled = enabled;
    persistPreference(QStringLiteral("glideEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
}

void KeyboardUiBridge::setCompositionEnabled(bool enabled)
{
    if (m_typingEngine.compositionEnabled() == enabled) return;
    m_typingEngine.setCompositionEnabled(enabled);
    persistPreference(QStringLiteral("compositionEnabled"), enabled);
    Q_EMIT typingPreferencesChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::setClipboardHistoryEnabled(bool enabled)
{
    if (m_clipboardHistory.enabled() == enabled) return;
    m_clipboardHistory.setEnabled(enabled);
    Q_EMIT clipboardChanged();
}

void KeyboardUiBridge::clearLearnedWords()
{
    m_typingEngine.clearLearning();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::resetInputContext()
{
    if (m_voice) m_voice->cancel();                 // never type into a new field
    loadShortcuts();                                 // pick up edits without a restart
    if (m_inputPurpose != QStringLiteral("text")) {
        m_inputPurpose = QStringLiteral("text");
        Q_EMIT inputContextChanged();
    }
    m_typingEngine.setSensitiveContext(false);
    m_typingEngine.resetInputContext();
    if (m_secureInput) {
        m_secureInput = false;
        Q_EMIT inputContextChanged();
    }
    m_panelManager.returnToTyping();
    Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
    Q_EMIT toolbarStateChanged();
}

void KeyboardUiBridge::setSurroundingText(const QString &text, int cursorByte, int anchorByte)
{
    const bool uppercaseBefore = uppercase();
    if (!m_typingEngine.syncSurroundingText(text, cursorByte, anchorByte)) return;
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::setContentType(quint32 hint, quint32 purpose)
{
    // KWin forwards text-input-v1 enums here and maps PIN to password (8).
    constexpr quint32 lowercaseHint = 0x8;
    constexpr quint32 hiddenTextHint = 0x40;
    constexpr quint32 sensitiveDataHint = 0x80;
    enum : quint32 { Digits = 2, Number = 3, Phone = 4, Url = 5, Email = 6, Password = 8,
                     Date = 9, Time = 10, DateTime = 11, Terminal = 12 };

    const bool secure = (hint & (hiddenTextHint | sensitiveDataHint)) != 0 || purpose == Password;
    // Addresses, numbers and terminals must not be autocorrected, capitalized
    // or learned from, but they are not secrets.
    const bool restricted = purpose == Digits || purpose == Number || purpose == Phone
        || purpose == Url || purpose == Email || purpose == Date || purpose == Time
        || purpose == DateTime || purpose == Terminal;

    const QString purposeName = (purpose == Digits || purpose == Number) ? QStringLiteral("number")
        : purpose == Phone ? QStringLiteral("phone")
        : purpose == Url ? QStringLiteral("url")
        : purpose == Email ? QStringLiteral("email")
        : QStringLiteral("text");
    const bool purposeChanged = purposeName != m_inputPurpose;
    m_inputPurpose = purposeName;

    const bool uppercaseBefore = uppercase();
    m_typingEngine.setSensitiveContext(secure || restricted);
    m_typingEngine.setAutoCapitalizationAllowed((hint & lowercaseHint) == 0 && !restricted);
    if (m_secureInput != secure || purposeChanged) {
        m_secureInput = secure;
        Q_EMIT inputContextChanged();
    }
    if (uppercaseBefore != uppercase()) Q_EMIT keyboardStateChanged();
    Q_EMIT suggestionsChanged();
}

void KeyboardUiBridge::resetCompositionFromClient()
{
    m_typingEngine.resetComposition();
    typingStateDidChange();
}

void KeyboardUiBridge::setPreferredLanguage(const QString &language)
{
    const QString lowered = language.toLower();
    QString code;
    if (lowered.startsWith(QStringLiteral("de"))) code = QStringLiteral("de");
    else if (lowered.startsWith(QStringLiteral("uk"))) code = QStringLiteral("uk");
    else if (lowered.startsWith(QStringLiteral("ru"))) code = QStringLiteral("ru");
    else if (lowered.startsWith(QStringLiteral("en"))) code = QStringLiteral("en");
    if (!code.isEmpty() && code != m_model.languageCode()) setLanguage(code);
}

void KeyboardUiBridge::persistLanguage() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("language"), m_model.languageCode());
}

void KeyboardUiBridge::persistPreference(const QString &key, const QVariant &value) const
{
    QSettings settings;
    settings.setValue(key, value);
}

}
