// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <QPair>

#include "platform/kwin/kwininputmethodv1backend.h"

class FakeInputMethodV1Context final : public Tastra::KWin::InputMethodV1Context
{
public:
    void commitString(quint32 serial, const QString &text) override
    {
        ++commitCount;
        lastSerial = serial;
        lastText = text;
        if (keepAll) allText.append(text);
    }
    bool keepAll = false;
    QStringList allText;

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

    void preeditString(quint32 serial, const QString &text, const QString &commit) override
    {
        preedits.append({text, commit});
        order.append(QStringLiteral("string"));
        lastSerial = serial;
    }

    void preeditCursor(qint32 index) override
    {
        cursors.append(index);
        order.append(QStringLiteral("cursor"));
    }

    QList<QPair<QString, QString>> preedits;
    QList<qint32> cursors;
    QStringList order;
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
        Tastra::KWin::KWinInputMethodV1Backend backend;
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

    void upAndDownAreSentAsKeysyms()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;
        backend.setContext(&context);
        backend.moveUp();
        backend.moveDown();
        QVERIFY(context.keySyms.contains(XKB_KEY_Up));
        QVERIFY(context.keySyms.contains(XKB_KEY_Down));
    }

    void deleteAroundCursorCoversBothSidesInBytes()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;
        QVERIFY(!backend.deleteAroundCursor(QStringLiteral("wr"), QStringLiteral("old")));   // no context
        backend.setContext(&context);
        QVERIFY(backend.deleteAroundCursor(QStringLiteral("wr"), QStringLiteral("old")));
        QVERIFY(backend.deleteAroundCursor(QStringLiteral("пр"), QStringLiteral("ивет")));    // UTF-8
        // KWin maps (index, length) to before = -index, after = index + length.
        QCOMPARE(context.deletes.at(0), qMakePair(qint32(-2), quint32(5)));
        QCOMPARE(context.deletes.at(1), qMakePair(qint32(-4), quint32(12)));
    }

    void preeditCarriesCommitFallbackAndByteCursor()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;
        QVERIFY(!backend.setPreedit(QStringLiteral("x")));   // no context
        backend.setContext(&context);
        backend.setLatestSerial(7);

        QVERIFY(backend.setPreedit(QStringLiteral("при")));
        QVERIFY(backend.setPreedit(QString()));

        // The commit argument is what KWin commits by itself when keyboard
        // focus moves to another surface, so an unfinished word is kept.
        QCOMPARE(context.preedits.at(0), qMakePair(QStringLiteral("при"), QStringLiteral("при")));
        QCOMPARE(context.cursors.at(0), 6);                       // bytes, cursor at end
        QCOMPARE(context.order.mid(0, 2), QStringList({QStringLiteral("cursor"), QStringLiteral("string")}));
        QCOMPARE(context.preedits.at(1), qMakePair(QString(), QString()));
        QCOMPARE(context.lastSerial, quint32(7));
    }

    void commitUsesLatestSerial()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(42);
        backend.commitText(QStringLiteral("a"));

        QCOMPARE(context.commitCount, 1);
        QCOMPARE(context.lastSerial, quint32(42));
        QCOMPARE(context.lastText, QStringLiteral("a"));
    }

    void aLongPasteIsCommittedInPieces()
    {
        // libwayland (before 1.23) disconnects a client whose message is over
        // 4096 bytes; a pasted page must not take the keyboard down.
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;
        backend.setContext(&context);
        QString text;
        for (int i = 0; i < 1500; ++i) text += QStringLiteral("aä€😀");   // 1, 2, 3 and 4 UTF-8 bytes
        context.keepAll = true;
        backend.commitText(text);
        QVERIFY(context.commitCount > 1);
        QCOMPARE(context.allText.join(QString()), text);
        for (const QString &piece : std::as_const(context.allText)) {
            QVERIFY(piece.toUtf8().size() <= 3000);
            QVERIFY(!piece.front().isLowSurrogate() && !piece.back().isHighSurrogate());
        }
        context.allText.clear();
        context.commitCount = 0;
        backend.commitText(QStringLiteral("short"));
        QCOMPARE(context.commitCount, 1);
    }

    void commitWithoutContextIsIgnored()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;

        backend.setLatestSerial(99);
        backend.commitText(QStringLiteral("a"));

        QVERIFY(true);
    }

    void deactivationStopsCommits()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        FakeInputMethodV1Context context;

        backend.setContext(&context);
        backend.setLatestSerial(7);
        backend.setContext(nullptr);
        backend.commitText(QStringLiteral("x"));

        QCOMPARE(context.commitCount, 0);
    }

    void backspaceGeneratesBackspaceKeysym()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
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
        Tastra::KWin::KWinInputMethodV1Backend backend;
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
