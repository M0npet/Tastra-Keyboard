// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "platform/kwin/kwininputmethodv1session.h"

class FakeContext final : public Tastra::KWin::InputMethodV1Context
{
public:
    void deleteSurroundingText(qint32 index, quint32 length) override
    {
        Q_UNUSED(index)
        Q_UNUSED(length)
    }

    void preeditString(quint32 serial, const QString &text, const QString &commit) override
    {
        Q_UNUSED(serial)
        Q_UNUSED(text)
        Q_UNUSED(commit)
    }

    void preeditCursor(qint32 index) override
    {
        Q_UNUSED(index)
    }

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

class KWinInputMethodV1SessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activationAndCommitStateDriveBackend()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        Tastra::KWin::KWinInputMethodV1Session session(backend);
        FakeContext context;

        session.activate(context);
        session.commitState(42);
        backend.commitText(QStringLiteral("a"));

        QCOMPARE(context.commitCount, 1);
        QCOMPARE(context.lastSerial, quint32(42));
        QCOMPARE(context.lastText, QStringLiteral("a"));
    }

    void deactivateStopsCommits()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        Tastra::KWin::KWinInputMethodV1Session session(backend);
        FakeContext context;

        session.activate(context);
        session.commitState(7);
        session.deactivate(context);
        backend.commitText(QStringLiteral("x"));

        QCOMPARE(context.commitCount, 0);
    }

    void staleDeactivateDoesNotClearNewContext()
    {
        Tastra::KWin::KWinInputMethodV1Backend backend;
        Tastra::KWin::KWinInputMethodV1Session session(backend);
        FakeContext oldContext;
        FakeContext newContext;

        session.activate(oldContext);
        session.activate(newContext);

        session.deactivate(oldContext);
        session.commitState(99);
        backend.commitText(QStringLiteral("b"));

        QCOMPARE(oldContext.commitCount, 0);
        QCOMPARE(newContext.commitCount, 1);
        QCOMPARE(newContext.lastSerial, quint32(99));
        QCOMPARE(newContext.lastText, QStringLiteral("b"));
    }
};

QTEST_APPLESS_MAIN(KWinInputMethodV1SessionTest)

#include "tst_kwininputmethodv1session.moc"
