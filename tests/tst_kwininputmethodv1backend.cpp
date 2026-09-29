// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "platform/kwin/kwininputmethodv1backend.h"

class FakeInputMethodV1Context final : public V3Keyboard::KWin::InputMethodV1Context
{
public:
    void commitString(quint32 serial, const QString &text) override
    {
        ++commitCount;
        lastSerial = serial;
        lastText = text;
    }

    void keySym(
        quint32 serial,
        quint32 time,
        quint32 sym,
        quint32 state,
        quint32 modifiers) override
    {
        Q_UNUSED(time)
        Q_UNUSED(modifiers)

        keySerials.append(serial);
        keySyms.append(sym);
        keyStates.append(state);
    }

    int commitCount = 0;
    quint32 lastSerial = 0;
    QString lastText;
    QList<quint32> keySerials;
    QList<quint32> keySyms;
    QList<quint32> keyStates;
};

class KWinInputMethodV1BackendTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void commitUsesLatestSerial()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(42);
        backend.commitText(QStringLiteral("a"));

        QCOMPARE(context.commitCount, 1);
        QCOMPARE(context.lastSerial, quint32(42));
        QCOMPARE(context.lastText, QStringLiteral("a"));
    }

    void commitWithoutContextIsIgnored()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;

        backend.setLatestSerial(99);
        backend.commitText(QStringLiteral("a"));

        QVERIFY(true);
    }

    void deactivationStopsCommits()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(7);
        backend.setContext(nullptr);
        backend.commitText(QStringLiteral("x"));

        QCOMPARE(context.commitCount, 0);
    }

    void backspaceGeneratesBackspaceKeysym()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(123);
        backend.backspace();

        QCOMPARE(context.keySerials, QList<quint32>({123, 123}));
        QCOMPARE(context.keyStates, QList<quint32>({1, 0}));
        QCOMPARE(context.keySyms, QList<quint32>({0xff08, 0xff08}));
    }

    void enterGeneratesReturnKeysym()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(456);
        backend.enter();

        QCOMPARE(context.keySerials, QList<quint32>({456, 456}));
        QCOMPARE(context.keyStates, QList<quint32>({1, 0}));
        QCOMPARE(context.keySyms, QList<quint32>({0xff0d, 0xff0d}));
    }
};

QTEST_APPLESS_MAIN(KWinInputMethodV1BackendTest)

#include "tst_kwininputmethodv1backend.moc"
