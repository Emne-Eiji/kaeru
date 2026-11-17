// Kaeru unit tests — persistent preferences (WP-08/WP-09)
#include "app/Paths.h"
#include "app/Settings.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace kaeru;

class TestSettings final : public QObject {
    Q_OBJECT

public:
    explicit TestSettings(QObject* parent = nullptr) : QObject(parent) {}

private slots:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        Paths::applyTestSandbox(m_home.path());
    }

    void themePersists()
    {
        {
            Settings s;
            QCOMPARE(s.theme(), QStringLiteral("system"));
            QSignalSpy spy(&s, &Settings::changed);
            s.setTheme(QStringLiteral("dark"));
            QCOMPARE(spy.count(), 1);
            QCOMPARE(s.theme(), QStringLiteral("dark"));
        }
        {
            Settings reopened;
            QCOMPARE(reopened.theme(), QStringLiteral("dark"));
            reopened.setTheme(QStringLiteral("light"));
        }
        {
            Settings reopened;
            QCOMPARE(reopened.theme(), QStringLiteral("light"));
            reopened.setTheme(QStringLiteral("system"));
        }
    }

    void lastDurationPersists()
    {
        {
            Settings s;
            QCOMPARE(s.lastAwakeDurationToken(), QStringLiteral("1h"));
            s.setLastAwakeDurationToken(QStringLiteral("15m"));
        }
        {
            Settings s;
            QCOMPARE(s.lastAwakeDurationToken(), QStringLiteral("15m"));
            s.setLastAwakeDurationToken(QStringLiteral("custom:5400000"));
        }
        {
            Settings s;
            QCOMPARE(s.lastAwakeDurationToken(), QStringLiteral("custom:5400000"));
            s.setLastAwakeDurationToken(QStringLiteral("indefinite"));
        }
        {
            Settings s;
            QCOMPARE(s.lastAwakeDurationToken(), QStringLiteral("indefinite"));
            s.setLastAwakeDurationToken(QStringLiteral("1h"));
        }
    }

    void windowGeometryPersists()
    {
        QRect none;
        {
            Settings s;
            QVERIFY(!s.windowGeometry(&none));
            s.setWindowGeometry(QRect(120, 80, 1100, 760));
        }
        {
            Settings s;
            QRect restored;
            QVERIFY(s.windowGeometry(&restored));
            QCOMPARE(restored, QRect(120, 80, 1100, 760));
            s.setWindowGeometry(QRect());
            QRect still;
            // Invalid input is ignored; the previous value remains.
            QVERIFY(s.windowGeometry(&still));
            QCOMPARE(still, restored);
        }
    }

private:
    QTemporaryDir m_home;
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
