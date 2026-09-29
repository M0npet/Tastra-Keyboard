// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

#include "app/tracelog.h"
#include "core/locallexicon.h"

#include <QFile>
#include <QLoggingCategory>
#include <QTemporaryDir>

Q_LOGGING_CATEGORY(lcTraceProbe, "v3keyboard.test", QtWarningMsg)
Q_LOGGING_CATEGORY(lcOtherProbe, "other.test", QtWarningMsg)

#include "app/keyboarduibridge.h"
#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"

class FakeBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override
    {
        preedit.clear();
        commits.append(text);
        events.append(QStringLiteral("commit:") + text);
    }

    void backspace() override
    {
        ++backspaceCount;
    }

    void deleteForward() override
    {
        ++deleteForwardCount;
    }

    void moveLeft() override
    {
        ++moveLeftCount;
        events.append(QStringLiteral("left"));
    }

    bool setPreedit(const QString &text) override
    {
        preedit = text;
        events.append(QStringLiteral("pre:") + text);
        return true;
    }

    QString preedit;
    QStringList events;

    void moveRight() override
    {
        ++moveRightCount;
    }

    void moveHome() override
    {
        ++moveHomeCount;
    }

    void moveEnd() override
    {
        ++moveEndCount;
    }

    void enter() override
    {
        ++enterCount;
    }

    QStringList commits;
    int backspaceCount = 0;
    int deleteForwardCount = 0;
    int moveLeftCount = 0;
    int moveRightCount = 0;
    int moveHomeCount = 0;
    int moveEndCount = 0;
    int enterCount = 0;
};

class KeyboardUiBridgeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("V3KeyboardTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_keyboarduibridge"));
        V3Keyboard::LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
    }

    void init() { QSettings().clear(); }

    // KWin sends text-input-v1 enums and maps PIN to password (8); 9 is date.
    void dateFieldIsNotTreatedAsSecret()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        bridge.setContentType(0, 9);
        QVERIFY(!bridge.secureInput());
        bridge.setContentType(0, 8);
        QVERIFY(bridge.secureInput());
        bridge.setContentType(0x40, 0);
        QVERIFY(bridge.secureInput());
    }

    void urlAndEmailFieldsDisableSmartTypingWithoutBeingSecret()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        for (quint32 purpose : {5u, 6u}) {
            bridge.setContentType(0, purpose);
            QVERIFY(!bridge.secureInput());
            QVERIFY(!bridge.uppercase());             // no auto-capitalization
            bridge.tapLetter(QStringLiteral("h"));
            bridge.tapLetter(QStringLiteral("e"));
            QVERIFY(bridge.suggestions().isEmpty());  // no suggestions / learning
            bridge.resetInputContext();
        }
        QCOMPARE(backend.commits.first(), QStringLiteral("h"));
    }

    void lowercaseHintDisablesAutoCapitalization()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        QVERIFY(bridge.uppercase());                   // sentence start by default
        bridge.setContentType(0x8, 0);
        QVERIFY(!bridge.uppercase());
        bridge.resetInputContext();
        QVERIFY(bridge.uppercase());
    }

    void cursorMovesAndLanguageSwitchCommitTheComposedWordFirst()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QString(), 0, 0);        // text-input client

        bridge.tapLetter(QStringLiteral("o"));
        bridge.tapLetter(QStringLiteral("k"));
        QCOMPARE(backend.preedit, QStringLiteral("Ok"));
        bridge.moveLeft();
        QCOMPARE(backend.events.mid(backend.events.size() - 2),
                 QStringList({QStringLiteral("commit:Ok"), QStringLiteral("left")}));

        bridge.setSurroundingText(QStringLiteral("x"), 1, 1);
        bridge.tapLetter(QStringLiteral("n"));
        bridge.nextLanguage();
        QCOMPARE(backend.commits.last(), QStringLiteral("n"));
    }

    void compositionCanBeSwitchedOff()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setSurroundingText(QString(), 0, 0);

        QVERIFY(bridge.compositionEnabled());
        bridge.setCompositionEnabled(false);
        bridge.tapLetter(QStringLiteral("a"));
        QCOMPARE(backend.commits, QStringList({QStringLiteral("A")}));
        QVERIFY(backend.preedit.isEmpty());
    }

    void traceIsOptInAndOnlyCapturesKeyboardCategories()
    {
        QTemporaryDir dir;
        QVERIFY(!V3Keyboard::enableTraceIfRequested(dir.path()));   // no flag file
        QFile flag(dir.path() + QStringLiteral("/trace.enable"));
        QVERIFY(flag.open(QIODevice::WriteOnly));
        flag.close();
        QVERIFY(V3Keyboard::enableTraceIfRequested(dir.path()));

        qCDebug(lcTraceProbe) << "probe bytes" << 3;
        qCWarning(lcOtherProbe) << "not ours";
        QFile log(dir.path() + QStringLiteral("/trace.log"));
        QVERIFY(log.open(QIODevice::ReadOnly));
        const QByteArray content = log.readAll();
        QVERIFY(content.contains("v3keyboard.test: probe bytes 3"));
        QVERIFY(!content.contains("not ours"));
    }

    void tapTextReachesController()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        bridge.tapText(QStringLiteral("a"));

        QCOMPARE(backend.commits, QStringList{QStringLiteral("a")});
    }

    void editingCommandsReachController()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        bridge.moveLeft();
        bridge.moveRight();
        bridge.moveHome();
        bridge.moveEnd();
        bridge.deleteForward();

        QCOMPARE(backend.moveLeftCount, 1);
        QCOMPARE(backend.moveRightCount, 1);
        QCOMPARE(backend.moveHomeCount, 1);
        QCOMPARE(backend.moveEndCount, 1);
        QCOMPARE(backend.deleteForwardCount, 1);
    }


    void ordinaryTypingDoesNotInvalidateWholeKeyboard()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        // Consume the initial sentence-start capitalization with the first key.
        bridge.tapLetter(QStringLiteral("h"));

        QSignalSpy keyboardSpy(&bridge, &V3Keyboard::KeyboardUiBridge::keyboardStateChanged);
        QSignalSpy suggestionSpy(&bridge, &V3Keyboard::KeyboardUiBridge::suggestionsChanged);

        bridge.tapLetter(QStringLiteral("e"));

        QCOMPARE(keyboardSpy.count(), 0);
        QCOMPARE(suggestionSpy.count(), 1);
    }
};

QTEST_APPLESS_MAIN(KeyboardUiBridgeTest)

#include "tst_keyboarduibridge.moc"
