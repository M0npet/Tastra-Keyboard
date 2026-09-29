// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "app/keyboardlayoutmetrics.h"

class KeyboardLayoutMetricsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void portraitIsFullerAndTaller()
    {
        const auto portrait = V3Keyboard::KeyboardLayoutMetrics::portrait();
        const auto landscape = V3Keyboard::KeyboardLayoutMetrics::landscape();

        QVERIFY(portrait.contentWidthRatio > landscape.contentWidthRatio);
        QVERIFY(portrait.keyHeight > landscape.keyHeight);
        QVERIFY(portrait.panelHeight() > landscape.panelHeight());
    }

    void landscapeStaysCompact()
    {
        const auto metrics = V3Keyboard::KeyboardLayoutMetrics::landscape();

        QVERIFY(metrics.panelHeight() < 300.0);
        QVERIFY(metrics.maxContentWidth >= 1300.0);
        QVERIFY(metrics.keyGap >= 6.0);
    }

    void portraitFitsTabletDockedUse()
    {
        const auto metrics = V3Keyboard::KeyboardLayoutMetrics::portrait();

        QVERIFY(metrics.panelHeight() < 350.0);
        QVERIFY(metrics.contentWidthRatio >= 0.95);
        QVERIFY(metrics.keyHeight >= 68.0);
    }
};

QTEST_APPLESS_MAIN(KeyboardLayoutMetricsTest)

#include "tst_keyboardlayoutmetrics.moc"
