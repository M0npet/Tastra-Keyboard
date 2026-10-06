// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "core/keyboardmodel.h"
#include "../src/core/panelmanager.h"
#include "../src/core/toolbarregistry.h"
#include "../src/core/toolbarmodel.h"

class KeyboardModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void longPressAlternatesFollowGboardPositions()
    {
        V3Keyboard::KeyboardModel model;                       // EN QWERTY
        QCOMPARE(model.alternatesForKey(QStringLiteral("q")), QStringList({QStringLiteral("1")}));
        QCOMPARE(model.alternatesForKey(QStringLiteral("p")), QStringList({QStringLiteral("0")}));
        QCOMPARE(model.alternatesForKey(QStringLiteral("a")).value(0), QStringLiteral("@"));
        QVERIFY(model.alternatesForKey(QStringLiteral("a")).contains(QStringLiteral("á")));
        QCOMPARE(model.alternatesForKey(QStringLiteral("e")).mid(0, 2), QStringList({QStringLiteral("3"), QStringLiteral("è")}));
        QCOMPARE(model.alternatesForKey(QStringLiteral("m")), QStringList({QStringLiteral("?")}));
        QCOMPARE(model.alternatesForKey(QStringLiteral(".")).value(0), QStringLiteral(","));
        QVERIFY(model.alternatesForKey(QStringLiteral(".")).contains(QStringLiteral("!")));

        model.setLanguage(QStringLiteral("de"));               // language letter first
        QCOMPARE(model.alternatesForKey(QStringLiteral("s")), QStringList({QStringLiteral("ß"), QStringLiteral("#")}));
        QCOMPARE(model.alternateForKey(QStringLiteral("s")), QStringLiteral("ß"));
        model.setLanguage(QStringLiteral("ru"));
        QCOMPARE(model.alternatesForKey(QStringLiteral("й")), QStringList({QStringLiteral("1")}));
        QCOMPARE(model.alternatesForKey(QStringLiteral("е")), QStringList({QStringLiteral("ё"), QStringLiteral("5")}));
        model.setLanguage(QStringLiteral("uk"));
        QCOMPARE(model.alternatesForKey(QStringLiteral("г")).value(0), QStringLiteral("ґ"));

        model.toggleSymbols();
        // Symbols layer: Gboard's extras (fractions, superscripts) on digits.
        QVERIFY(model.alternatesForKey(QStringLiteral("1")).contains(QStringLiteral("½")));
    }

    void symbolPagesLikeGboard()
    {
        V3Keyboard::KeyboardModel model;
        model.toggleSymbols();
        QCOMPARE(model.symbolPage(), 0);
        // Page 1 (?123): the currency of the language.
        QVERIFY(model.symbolRows().at(1).contains(QStringLiteral("$")));
        model.setLanguage(QStringLiteral("de"));
        QVERIFY(model.symbolRows().at(1).contains(QStringLiteral("€")));
        model.setLanguage(QStringLiteral("ru"));
        QVERIFY(model.symbolRows().at(1).contains(QStringLiteral("₽")));
        model.setLanguage(QStringLiteral("uk"));
        QVERIFY(model.symbolRows().at(1).contains(QStringLiteral("₴")));
        QVERIFY(model.symbolRows().at(1).contains(QStringLiteral("_")));
        // Page 2 (=\<): programming and typographic symbols.
        model.toggleSymbolPage();
        QCOMPARE(model.symbolPage(), 1);
        QString page2;
        for (const QStringList &row : model.symbolRows()) page2 += row.join(QString());
        for (const QString &sym : {QStringLiteral("<"), QStringLiteral(">"), QStringLiteral("["), QStringLiteral("]"),
                                   QStringLiteral("{"), QStringLiteral("}"), QStringLiteral("\\"), QStringLiteral("|"),
                                   QStringLiteral("~"), QStringLiteral("^"), QStringLiteral("="), QStringLiteral("%")}) {
            QVERIFY2(page2.contains(sym), qPrintable(sym));
        }
        for (const QStringList &row : model.symbolRows()) QVERIFY(row.size() >= 8 && row.size() <= 10);
        // Long-press extras on symbol keys.
        QVERIFY(model.alternatesForKey(QStringLiteral("\"")).contains(QStringLiteral("«")));
        QVERIFY(model.alternatesForKey(QStringLiteral("-")).contains(QStringLiteral("—")));
        QVERIFY(model.alternatesForKey(QStringLiteral("₴")).contains(QStringLiteral("€")));
        // Leaving the symbols layer resets to page 1.
        model.toggleSymbols();
        model.toggleSymbols();
        QCOMPARE(model.symbolPage(), 0);
    }

    void startsLowercase()
    {
        V3Keyboard::KeyboardModel model;

        QVERIFY(!model.uppercase());
        QVERIFY(!model.capsLock());
        QCOMPARE(model.textForLetter(QStringLiteral("a")), QStringLiteral("a"));
    }

    void oneShiftUppercasesOneLetter()
    {
        V3Keyboard::KeyboardModel model;

        model.pressShift();

        QVERIFY(model.uppercase());
        QVERIFY(!model.capsLock());
        QCOMPARE(model.textForLetter(QStringLiteral("q")), QStringLiteral("Q"));

        model.consumeShiftAfterLetter();

        QVERIFY(!model.uppercase());
    }

    void secondShiftEnablesCapsLock()
    {
        V3Keyboard::KeyboardModel model;

        model.pressShift();
        model.pressShift();

        QVERIFY(model.uppercase());
        QVERIFY(model.capsLock());

        model.consumeShiftAfterLetter();

        QVERIFY(model.uppercase());
        QVERIFY(model.capsLock());
        QCOMPARE(model.textForLetter(QStringLiteral("z")), QStringLiteral("Z"));
    }

    void shiftCyclesBackToLowercaseFromCapsLock()
    {
        V3Keyboard::KeyboardModel model;

        model.pressShift();
        model.pressShift();
        model.pressShift();

        QVERIFY(!model.uppercase());
        QVERIFY(!model.capsLock());
    }

    void symbolLayerTogglesAndClearsShift()
    {
        V3Keyboard::KeyboardModel model;

        model.pressShift();
        QVERIFY(model.uppercase());

        model.toggleSymbols();

        QVERIFY(model.symbolsActive());
        QVERIFY(!model.uppercase());
        QVERIFY(!model.capsLock());

        model.toggleSymbols();

        QVERIFY(!model.symbolsActive());
        QVERIFY(!model.uppercase());
    }

    void cyclesSupportedLanguages()
    {
        V3Keyboard::KeyboardModel model;

        QCOMPARE(model.languageCode(), QStringLiteral("en"));
        QCOMPARE(model.languageLabel(), QStringLiteral("English"));

        model.nextLanguage();
        QCOMPARE(model.languageCode(), QStringLiteral("de"));
        QCOMPARE(model.languageLabel(), QStringLiteral("Deutsch"));

        model.nextLanguage();
        QCOMPARE(model.languageCode(), QStringLiteral("uk"));
        QCOMPARE(model.languageLabel(), QStringLiteral("Українська"));

        model.nextLanguage();
        QCOMPARE(model.languageCode(), QStringLiteral("ru"));
        QCOMPARE(model.languageLabel(), QStringLiteral("Русский"));

        model.nextLanguage();
        QCOMPARE(model.languageCode(), QStringLiteral("en"));
    }

    void exposesExpectedRowsForEachLanguage()
    {
        V3Keyboard::KeyboardModel model;

        QCOMPARE(model.row1().join(QString()), QStringLiteral("qwertyuiop"));
        QCOMPARE(model.row2().join(QString()), QStringLiteral("asdfghjkl"));
        QCOMPARE(model.row3().join(QString()), QStringLiteral("zxcvbnm"));

        model.setLanguage(QStringLiteral("de"));
        QCOMPARE(model.row1().join(QString()), QStringLiteral("qwertzuiopü"));
        QCOMPARE(model.row2().join(QString()), QStringLiteral("asdfghjklöä"));
        QCOMPARE(model.row3().join(QString()), QStringLiteral("yxcvbnm"));

        model.setLanguage(QStringLiteral("uk"));
        QCOMPARE(model.row1().join(QString()), QStringLiteral("йцукенгшщзхї"));
        QCOMPARE(model.row2().join(QString()), QStringLiteral("фівапролджє"));
        QCOMPARE(model.row3().join(QString()), QStringLiteral("ячсмитьбю"));

        model.setLanguage(QStringLiteral("ru"));
        QCOMPARE(model.row1().join(QString()), QStringLiteral("йцукенгшщзхъ"));
        QCOMPARE(model.row2().join(QString()), QStringLiteral("фывапролджэ"));
        QCOMPARE(model.row3().join(QString()), QStringLiteral("ячсмитьбю"));
    }

    void changingLanguageClearsShiftState()
    {
        V3Keyboard::KeyboardModel model;

        model.pressShift();
        QVERIFY(model.uppercase());

        model.setLanguage(QStringLiteral("uk"));

        QVERIFY(!model.uppercase());
        QVERIFY(!model.capsLock());
    }

    void exposesLanguageSpecificLongPressAlternates()
    {
        V3Keyboard::KeyboardModel model;

        QCOMPARE(model.alternateForKey(QStringLiteral("s")), QString());

        model.setLanguage(QStringLiteral("de"));
        QCOMPARE(model.alternateForKey(QStringLiteral("s")), QStringLiteral("ß"));
        model.pressShift();
        QCOMPARE(model.alternateForKey(QStringLiteral("s")), QStringLiteral("ẞ"));

        model.setLanguage(QStringLiteral("uk"));
        QCOMPARE(model.alternateForKey(QStringLiteral("г")), QStringLiteral("ґ"));

        model.setLanguage(QStringLiteral("ru"));
        QCOMPARE(model.alternateForKey(QStringLiteral("е")), QStringLiteral("ё"));

        model.toggleSymbols();
        QCOMPARE(model.alternateForKey(QStringLiteral("е")), QString());
    }

    void toolbarRegistryRejectsDuplicateIds()
    {
        V3Keyboard::ToolbarRegistry registry;

        V3Keyboard::ToolbarAction first;
        first.id = QStringLiteral("language");
        first.label = QStringLiteral("Language");
        first.kind = V3Keyboard::ToolbarActionKind::OpenPanel;
        first.panel = V3Keyboard::PanelId::Language;

        QVERIFY(registry.registerAction(first));
        QVERIFY(!registry.registerAction(first));
        QCOMPARE(registry.actions().size(), 1);
    }

    void defaultToolbarOrderIsStableAndHiddenActionsAreFiltered()
    {
        const auto registry = V3Keyboard::ToolbarRegistry::createDefault();
        const V3Keyboard::ToolbarModel model(registry);

        QCOMPARE(
            model.visibleActionIds(),
            QStringList({
                QStringLiteral("clipboard"),
                QStringLiteral("emoji"),
                QStringLiteral("text-editing"),
                QStringLiteral("settings"),
            })
        );

        const auto all = registry.actions();
        QCOMPARE(all.size(), 5);
        QCOMPARE(all.at(0).id, QStringLiteral("language"));
        QCOMPARE(all.at(1).id, QStringLiteral("clipboard"));
        QCOMPARE(all.at(2).id, QStringLiteral("emoji"));
        QCOMPARE(all.at(3).id, QStringLiteral("text-editing"));
        QCOMPARE(all.at(4).id, QStringLiteral("settings"));
    }

    void disabledToolbarActionCannotActivate()
    {
        V3Keyboard::ToolbarRegistry registry;

        V3Keyboard::ToolbarAction action;
        action.id = QStringLiteral("disabled");
        action.label = QStringLiteral("Disabled");
        action.kind = V3Keyboard::ToolbarActionKind::OpenPanel;
        action.panel = V3Keyboard::PanelId::Settings;
        action.enabled = false;

        QVERIFY(registry.registerAction(action));

        const V3Keyboard::ToolbarModel model(registry);
        V3Keyboard::PanelManager panels;

        QVERIFY(!model.activate(QStringLiteral("disabled"), panels));
        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Typing);
    }

    void hiddenLanguageToolbarActionDoesNotActivate()
    {
        const auto registry = V3Keyboard::ToolbarRegistry::createDefault();
        const V3Keyboard::ToolbarModel model(registry);
        V3Keyboard::PanelManager panels;

        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Typing);
        QVERIFY(!model.activate(QStringLiteral("language"), panels));
        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Typing);
    }

    void panelManagerReturnsToTyping()
    {
        V3Keyboard::PanelManager panels;

        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Typing);
        QVERIFY(panels.openPanel(V3Keyboard::PanelId::Emoji));
        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Emoji);

        QVERIFY(panels.returnToTyping());
        QCOMPARE(panels.activePanel(), V3Keyboard::PanelId::Typing);
        QVERIFY(!panels.returnToTyping());
    }

    void exposesToolbarDescriptorsForUi()
    {
        const auto registry = V3Keyboard::ToolbarRegistry::createDefault();
        const V3Keyboard::ToolbarModel model(registry);

        const auto actions = model.visibleActions();
        QCOMPARE(actions.size(), 4);
        QCOMPARE(actions.at(0).id, QStringLiteral("clipboard"));
        QCOMPARE(actions.at(1).id, QStringLiteral("emoji"));
        QCOMPARE(actions.at(2).id, QStringLiteral("text-editing"));
        QCOMPARE(actions.at(3).id, QStringLiteral("settings"));
    }

    void exposesSupportedLanguageList()
    {
        V3Keyboard::KeyboardModel model;

        QCOMPARE(
            model.languageCodes(),
            QStringList({
                QStringLiteral("en"),
                QStringLiteral("de"),
                QStringLiteral("uk"),
                QStringLiteral("ru"),
            })
        );

        QCOMPARE(
            model.languageLabels(),
            QStringList({
                QStringLiteral("English"),
                QStringLiteral("Deutsch"),
                QStringLiteral("Українська"),
                QStringLiteral("Русский"),
            })
        );
    }

    void languageActionStaysOutOfVisibleToolbar()
    {
        const auto registry = V3Keyboard::ToolbarRegistry::createDefault();
        const auto *language = registry.actionById(QStringLiteral("language"));

        QVERIFY(language != nullptr);
        QVERIFY(!language->visible);
        QCOMPARE(language->panel, V3Keyboard::PanelId::Language);
    }
};

QTEST_APPLESS_MAIN(KeyboardModelTest)

#include "tst_keyboardmodel.moc"
