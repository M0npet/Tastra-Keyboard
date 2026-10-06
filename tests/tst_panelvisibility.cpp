// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest/QTest>
#include <QSignalSpy>

#include "app/panelvisibility.h"

class PanelVisibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsAtOnceAndRidesOutAShortDeactivation()
    {
        // Rich web editors briefly disable text input while re-rendering; if
        // the panel vanished, the next tap would land on the page and steal
        // the focus (seen with other on-screen keyboards on such sites).
        Tastra::PanelVisibility panel(250);
        QSignalSpy spy(&panel, &Tastra::PanelVisibility::visibleChanged);
        panel.setActive(true);
        QVERIFY(panel.visible());
        QCOMPARE(spy.count(), 1);
        panel.setActive(false);
        QTest::qWait(100);
        panel.setActive(true);                       // re-activated within the grace time
        QTest::qWait(300);
        QVERIFY(panel.visible());
        QCOMPARE(spy.count(), 1);                    // never flickered
    }

    void hidesAfterTheGraceTimeWhenReallyLeft()
    {
        Tastra::PanelVisibility panel(250);
        QSignalSpy spy(&panel, &Tastra::PanelVisibility::visibleChanged);
        panel.setActive(true);
        panel.setActive(false);
        QVERIFY(panel.visible());
        QTRY_VERIFY_WITH_TIMEOUT(!panel.visible(), 1000);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.last().at(0).toBool(), false);
    }
};

QTEST_GUILESS_MAIN(PanelVisibilityTest)
#include "tst_panelvisibility.moc"
