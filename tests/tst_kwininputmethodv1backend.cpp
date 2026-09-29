// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include <QPair>

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

    void deleteSurroundingText(qint32 index, quint32 length) override
    {
        deletes.append({index, length});
    }

    QList<QPair<qint32, quint32>> deletes;
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
    void deleteBeforeCursorUsesUtf8ByteLengths()
    {
        V3Keyboard::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        QVERIFY(!backend.deleteBeforeCursor(QStringLiteral("teh")));   // no context yet
        backend.setContext(&context);
        QVERIFY(backend.deleteBeforeCursor(QStringLiteral("teh")));
        QVERIFY(backend.deleteBeforeCursor(QStringLiteral("прив")));  // 2 bytes per letter

        // input-method-v1: index is relative to the cursor, both in bytes.
        QCOMPARE(context.deletes.size(), 2);
        QCOMPARE(context.deletes.at(0), qMakePair(qint32(-3), quint32(3)));
        QCOMPARE(context.deletes.at(1), qMakePair(qint32(-8), quint32(8)));
        QVERIFY(context.keySyms.isEmpty());
    }

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
