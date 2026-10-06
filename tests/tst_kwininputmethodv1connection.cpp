// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "platform/kwin/kwininputmethodv1connection.h"

#include <type_traits>

class KWinInputMethodV1ConnectionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasExpectedConstructionContract()
    {
        using Connection = Tastra::KWin::KWinInputMethodV1Connection;
        using Backend = Tastra::KWin::KWinInputMethodV1Backend;

        QVERIFY((std::is_constructible_v<Connection, Backend &>));
        QVERIFY((std::is_base_of_v<QtWayland::zwp_input_method_v1, Connection>));
    }
};

QTEST_APPLESS_MAIN(KWinInputMethodV1ConnectionTest)

#include "tst_kwininputmethodv1connection.moc"
