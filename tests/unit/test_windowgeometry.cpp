// Kaeru unit tests — safe window restore (WP-09)
#include "ui/WindowGeometry.h"

#include <QRect>
#include <QtTest>

using namespace kaeru;

class TestWindowGeometry final : public QObject {
    Q_OBJECT

private slots:
    void defaultIsCentered()
    {
        const QRect r = fitWindowToAvailable(QRect(), QRect(0, 0, 1920, 1080),
                                             QSize(900, 650), QSize(1100, 760));
        QCOMPARE(r.size(), QSize(1100, 760));
        QCOMPARE(r.topLeft(), QPoint(410, 160));
    }

    void savedGeometryRestored()
    {
        const QRect saved(120, 90, 1100, 760);
        const QRect r = fitWindowToAvailable(saved, QRect(0, 0, 1920, 1080),
                                             QSize(900, 650), QSize(1100, 760));
        QCOMPARE(r, saved);
    }

    void offScreenGeometryClamped()
    {
        const QRect saved(5000, -4000, 1100, 760);
        const QRect r = fitWindowToAvailable(saved, QRect(0, 0, 1920, 1080),
                                             QSize(900, 650), QSize(1100, 760));
        QVERIFY(r.left() >= 0);
        QVERIFY(r.top() >= 0);
        QVERIFY(r.right() <= 1919);
        QVERIFY(r.bottom() <= 1079);
        QCOMPARE(r.size(), QSize(1100, 760));
    }

    void smallScreenRespected()
    {
        const QRect saved(0, 0, 1100, 760);
        const QRect r = fitWindowToAvailable(saved, QRect(0, 0, 1024, 600),
                                             QSize(900, 650), QSize(1100, 760));
        QCOMPARE(r.size(), QSize(1024, 600));
        QVERIFY(r.right() <= 1023);
        QVERIFY(r.bottom() <= 599);
    }

    void negativeDesktopCoordinates()
    {
        const QRect saved(-2000, -1500, 1100, 760);
        const QRect r = fitWindowToAvailable(saved, QRect(-1920, -1200, 1920, 1080),
                                             QSize(900, 650), QSize(1100, 760));
        QVERIFY(r.left() >= -1920);
        QVERIFY(r.top() >= -1200);
        QVERIFY(r.right() <= -1);
        QVERIFY(r.bottom() <= -121);
    }
};

QTEST_MAIN(TestWindowGeometry)
#include "test_windowgeometry.moc"
