// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

class SmokeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseline()
    {
        QVERIFY(true);
    }
};

QTEST_APPLESS_MAIN(SmokeTest)

#include "smoke.moc"
