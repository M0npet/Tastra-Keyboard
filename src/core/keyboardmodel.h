// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace V3Keyboard
{

class KeyboardModel
{
public:
    enum class ShiftState {
        Lowercase,
        OneShot,
        CapsLock,
    };

    enum class Layer {
        Alphabet,
        Symbols,
    };

    void pressShift();
    void consumeShiftAfterLetter();
    void toggleSymbols();

    bool uppercase() const;
    bool capsLock() const;
    bool symbolsActive() const;

    QString textForLetter(const QString &text) const;
    QString alternateForKey(const QString &text) const;
    // Gboard-style long-press choices; the first one is preselected (a
    // language letter such as ß/ё/ґ, otherwise the key's symbol hint).
    QStringList alternatesForKey(const QString &text) const;
    // Gboard symbol pages: 0 = "?123" (with the language's currency),
    // 1 = "=\<" (programming and typographic symbols). Rows of 10/9/8 keys.
    int symbolPage() const;
    void toggleSymbolPage();
    QList<QStringList> symbolRows() const;
    // The three letter rows of a language's layout (for layout conversion).
    static QStringList rowsForLanguage(const QString &code);

    QString languageCode() const;
    QString languageLabel() const;
    QStringList languageCodes() const;
    QStringList languageLabels() const;
    QStringList row1() const;
    QStringList row2() const;
    QStringList row3() const;

    void setLanguage(const QString &code);
    void nextLanguage();

private:
    void resetShift();

    ShiftState m_shiftState = ShiftState::Lowercase;
    Layer m_layer = Layer::Alphabet;
    int m_languageIndex = 0;
    int m_symbolPage = 0;
};

}
