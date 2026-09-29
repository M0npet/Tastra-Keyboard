// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/clipboardhistory.h"
#include "core/emojicatalog.h"
#include "core/keyboardmodel.h"
#include "core/panelmanager.h"
#include "core/toolbarregistry.h"
#include "core/toolbarmodel.h"
#include "core/typingengine.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace V3Keyboard
{

class KeyboardController;

class KeyboardUiBridge final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool uppercase READ uppercase NOTIFY keyboardStateChanged)
    Q_PROPERTY(bool capsLock READ capsLock NOTIFY keyboardStateChanged)
    Q_PROPERTY(bool symbolsActive READ symbolsActive NOTIFY keyboardStateChanged)
    Q_PROPERTY(QString languageCode READ languageCode NOTIFY keyboardStateChanged)
    Q_PROPERTY(QString languageLabel READ languageLabel NOTIFY keyboardStateChanged)
    Q_PROPERTY(QStringList row1 READ row1 NOTIFY keyboardStateChanged)
    Q_PROPERTY(QStringList row2 READ row2 NOTIFY keyboardStateChanged)
    Q_PROPERTY(QStringList row3 READ row3 NOTIFY keyboardStateChanged)
    Q_PROPERTY(QString activePanel READ activePanel NOTIFY toolbarStateChanged)
    Q_PROPERTY(QStringList toolbarActionIds READ toolbarActionIds NOTIFY toolbarStateChanged)
    Q_PROPERTY(QVariantList toolbarActions READ toolbarActions NOTIFY toolbarStateChanged)
    Q_PROPERTY(QStringList languageCodes READ languageCodes NOTIFY keyboardStateChanged)
    Q_PROPERTY(QStringList languageLabels READ languageLabels NOTIFY keyboardStateChanged)
    Q_PROPERTY(QString currentWord READ currentWord NOTIFY suggestionsChanged)
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY suggestionsChanged)
    Q_PROPERTY(bool secureInput READ secureInput NOTIFY inputContextChanged)

    Q_PROPERTY(bool amoled READ amoled WRITE setAmoled NOTIFY uiPreferencesChanged)
    Q_PROPERTY(QString layoutMode READ layoutMode WRITE setLayoutMode NOTIFY uiPreferencesChanged)
    Q_PROPERTY(bool voiceBuilt READ voiceBuilt NOTIFY voiceChanged)
    Q_PROPERTY(QString voiceState READ voiceState NOTIFY voiceChanged)
    Q_PROPERTY(QString voiceMessage READ voiceMessage NOTIFY voiceChanged)
    Q_PROPERTY(double keyScale READ keyScale WRITE setKeyScale NOTIFY uiPreferencesChanged)
    Q_PROPERTY(bool keyBorders READ keyBorders WRITE setKeyBorders NOTIFY uiPreferencesChanged)
    Q_PROPERTY(bool keyPopups READ keyPopups WRITE setKeyPopups NOTIFY uiPreferencesChanged)
    Q_PROPERTY(bool suggestionsEnabled READ suggestionsEnabled WRITE setSuggestionsEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool autocorrectEnabled READ autocorrectEnabled WRITE setAutocorrectEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool learningEnabled READ learningEnabled WRITE setLearningEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool autoCapitalizationEnabled READ autoCapitalizationEnabled WRITE setAutoCapitalizationEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool doubleSpacePeriodEnabled READ doubleSpacePeriodEnabled WRITE setDoubleSpacePeriodEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool glideEnabled READ glideEnabled WRITE setGlideEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool compositionEnabled READ compositionEnabled WRITE setCompositionEnabled NOTIFY typingPreferencesChanged)
    Q_PROPERTY(bool clipboardHistoryEnabled READ clipboardHistoryEnabled WRITE setClipboardHistoryEnabled NOTIFY clipboardChanged)

    Q_PROPERTY(QString clipboardText READ clipboardText NOTIFY clipboardChanged)
    Q_PROPERTY(QStringList clipboardHistory READ clipboardHistory NOTIFY clipboardChanged)
    Q_PROPERTY(QStringList emojiItems READ emojiItems CONSTANT)
    Q_PROPERTY(QStringList emojiCategories READ emojiCategories CONSTANT)

public:
    explicit KeyboardUiBridge(
        KeyboardController &controller,
        KeyboardModel &model,
        QObject *parent = nullptr);

    bool uppercase() const;
    bool capsLock() const;
    bool symbolsActive() const;

    QString languageCode() const;
    QString languageLabel() const;
    QStringList row1() const;
    QStringList row2() const;
    QStringList row3() const;

    QString activePanel() const;
    QStringList toolbarActionIds() const;
    QVariantList toolbarActions() const;
    QStringList languageCodes() const;
    QStringList languageLabels() const;
    QString currentWord() const;
    QStringList suggestions() const;

    bool amoled() const;
    QString layoutMode() const;
    bool voiceBuilt() const;
    QString voiceState() const;
    QString voiceMessage() const;
    // Optional: only set when the binary was built with offline voice input.
    void setVoiceController(class VoiceController *voice);
    double keyScale() const;
    bool keyBorders() const;
    bool keyPopups() const;
    bool suggestionsEnabled() const;
    bool autocorrectEnabled() const;
    bool learningEnabled() const;
    bool autoCapitalizationEnabled() const;
    bool doubleSpacePeriodEnabled() const;
    bool glideEnabled() const;
    bool compositionEnabled() const;
    bool secureInput() const;
    bool clipboardHistoryEnabled() const;

    QString clipboardText() const;
    QStringList clipboardHistory() const;
    QStringList emojiItems() const;
    QStringList emojiCategories() const;

    Q_INVOKABLE QVariantMap layoutMetrics(bool portrait) const;
    Q_INVOKABLE QString alternateForKey(const QString &text) const;
    Q_INVOKABLE QStringList emojiSearch(const QString &query, const QString &category = QStringLiteral("All")) const;

    Q_INVOKABLE void tapLetter(const QString &letter);
    Q_INVOKABLE void tapAlternate(const QString &base);
    Q_INVOKABLE void tapText(const QString &text);
    Q_INVOKABLE void selectSuggestion(const QString &word);
    Q_INVOKABLE void shift();
    Q_INVOKABLE void toggleSymbols();
    Q_INVOKABLE void nextLanguage();
    Q_INVOKABLE void setLanguage(const QString &code);
    Q_INVOKABLE void activateToolbarAction(const QString &id);
    Q_INVOKABLE void openLanguagePanel();
    Q_INVOKABLE void closePanel();
    Q_INVOKABLE void space();
    Q_INVOKABLE void backspace();
    Q_INVOKABLE void backspaceRepeated(int count);
    Q_INVOKABLE void deleteForward();
    Q_INVOKABLE void moveLeft();
    Q_INVOKABLE void moveRight();
    Q_INVOKABLE void moveCursor(int delta);
    Q_INVOKABLE void moveHome();
    Q_INVOKABLE void moveEnd();
    Q_INVOKABLE void enter();

    Q_INVOKABLE void beginGlide(const QString &key);
    Q_INVOKABLE void glideThrough(const QString &key);
    Q_INVOKABLE QString endGlide();

    Q_INVOKABLE void pasteClipboard();
    Q_INVOKABLE void pasteClipboardHistory(int index);
    Q_INVOKABLE void removeClipboardHistory(int index);
    Q_INVOKABLE void clearClipboard();
    Q_INVOKABLE void clearClipboardHistory();

    Q_INVOKABLE void setAmoled(bool enabled);
    // "full", "left" or "right" (compact keyboard docked to one side).
    Q_INVOKABLE void setLayoutMode(const QString &mode);
    Q_INVOKABLE void cycleLayoutMode();
    Q_INVOKABLE void toggleVoice();
    Q_INVOKABLE void setKeyScale(double scale);
    Q_INVOKABLE void setKeyBorders(bool enabled);
    Q_INVOKABLE void setKeyPopups(bool enabled);
    Q_INVOKABLE void setSuggestionsEnabled(bool enabled);
    Q_INVOKABLE void setAutocorrectEnabled(bool enabled);
    Q_INVOKABLE void setLearningEnabled(bool enabled);
    Q_INVOKABLE void setAutoCapitalizationEnabled(bool enabled);
    Q_INVOKABLE void setDoubleSpacePeriodEnabled(bool enabled);
    Q_INVOKABLE void setGlideEnabled(bool enabled);
    Q_INVOKABLE void setCompositionEnabled(bool enabled);
    Q_INVOKABLE void setClipboardHistoryEnabled(bool enabled);
    Q_INVOKABLE void clearLearnedWords();

    void resetInputContext();
    void setSurroundingText(const QString &text, int cursorByte, int anchorByte);
    void setContentType(quint32 hint, quint32 purpose);
    void resetCompositionFromClient();
    void setPreferredLanguage(const QString &language);

Q_SIGNALS:
    void keyboardStateChanged();
    void toolbarStateChanged();
    void uiPreferencesChanged();
    void voiceChanged();
    void typingPreferencesChanged();
    void suggestionsChanged();
    void clipboardChanged();
    void inputContextChanged();

private:
    void persistLanguage() const;
    void persistPreference(const QString &key, const QVariant &value) const;
    void typingStateDidChange();
    void captureClipboard();

    KeyboardController &m_controller;
    KeyboardModel &m_model;
    TypingEngine m_typingEngine;
    ClipboardHistory m_clipboardHistory;
    EmojiCatalog m_emojiCatalog;
    ToolbarRegistry m_toolbarRegistry;
    ToolbarModel m_toolbarModel;
    PanelManager m_panelManager;

    bool m_amoled = false;
    QString m_layoutMode = QStringLiteral("full");
    class VoiceController *m_voice = nullptr;
    double m_keyScale = 1.0;
    bool m_keyBorders = true;
    bool m_keyPopups = true;
    bool m_glideEnabled = true;
    bool m_secureInput = false;
};

}
