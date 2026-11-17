// Kaeru unit tests — tray presentation mapping (ADR-008 frog identity, WP-08)
#include "core/TrayState.h"

#include <QtTest>

using namespace kaeru;

class TestTrayState final : public QObject {
    Q_OBJECT

private slots:
    void neitherActiveSleeps()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Inactive, false, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Sleep);
        QCOMPARE(p.summary, QStringLiteral("inactive"));
        QCOMPARE(p.keepAwakeActive, false);
        QCOMPARE(p.focusActive, false);
    }

    void keepAwakeActiveShowsAwake()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Active, false, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Awake);
        QCOMPARE(p.keepAwakeActive, true);
        QCOMPARE(p.summary, QStringLiteral("keep awake active"));
    }

    void partialAwakeShowsAwakeWithLimitation()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::PartiallyActive, false, false, QString(),
            QStringLiteral("not prevented: session-idle"));
        QCOMPARE(p.icon, TrayPresentation::Icon::Awake);
        QCOMPARE(p.keepAwakePartial, true);
        QCOMPARE(p.summary, QStringLiteral("keep awake (partial)"));
        QCOMPARE(p.detail, QStringLiteral("not prevented: session-idle"));
    }

    void focusOnlyShowsFocused()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Inactive, true, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Focused);
        QCOMPARE(p.focusActive, true);
        QCOMPARE(p.keepAwakeActive, false);
    }

    void bothActiveShowsAwakeSafetyFirst()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Active, true, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Awake);
        QCOMPARE(p.keepAwakeActive, true);
        QCOMPARE(p.focusActive, true);
    }

    void pausedAllSleeps()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Active, true, true, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Sleep);
        QCOMPARE(p.pausedAll, true);
        QCOMPARE(p.summary, QStringLiteral("paused (Pause All)"));
        QCOMPARE(p.keepAwakeActive, false);
    }

    void failureIsHonestNotCalm()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Failed, false, false,
            QStringLiteral("portal declined"), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Sleep);
        QCOMPARE(p.error, true);
        QCOMPARE(p.summary, QStringLiteral("keep awake failed"));
        QCOMPARE(p.detail, QStringLiteral("portal declined"));
    }

    void deniedIsDistinct()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::PermissionDenied, false, false,
            QStringLiteral("not allowed"), QString());
        QCOMPARE(p.error, true);
        QCOMPARE(p.summary, QStringLiteral("keep awake denied"));
    }

    void activatingIsNotYetAwake()
    {
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Activating, false, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Sleep);
        QCOMPARE(p.keepAwakeActive, false);
        QCOMPARE(p.summary, QStringLiteral("activating…"));
    }

    void menuState_data()
    {
        QTest::addColumn<int>("status");
        QTest::addColumn<qint64>("remainingMs");
        QTest::addColumn<QString>("statusLine");
        QTest::addColumn<QString>("detailLine");
        QTest::addColumn<bool>("active");
        QTest::addColumn<bool>("error");

        QTest::newRow("inactive") << int(EffectiveStatus::Inactive) << qint64(-1)
                                  << QStringLiteral("Kaeru: OFF")
                                  << QStringLiteral("Inactive") << false << false;
        QTest::newRow("activating") << int(EffectiveStatus::Activating) << qint64(-1)
                                    << QStringLiteral("Kaeru: ACTIVATING…")
                                    << QStringLiteral("Starting…") << false << false;
        QTest::newRow("active-timed") << int(EffectiveStatus::Active)
                                      << qint64(84 * 60 * 1000)
                                      << QStringLiteral("Kaeru: ON")
                                      << QStringLiteral("1h 24m remaining") << true << false;
        QTest::newRow("active-indefinite") << int(EffectiveStatus::Active) << qint64(-1)
                                          << QStringLiteral("Kaeru: ON")
                                          << QStringLiteral("Indefinite") << true << false;
        QTest::newRow("partially-active") << int(EffectiveStatus::PartiallyActive)
                                          << qint64(-1)
                                          << QStringLiteral("Kaeru: ON")
                                          << QStringLiteral("Indefinite") << true << false;
        QTest::newRow("unsupported") << int(EffectiveStatus::Unsupported) << qint64(-1)
                                     << QStringLiteral("Kaeru: ERROR")
                                     << QStringLiteral("Inactive") << false << true;
        QTest::newRow("denied") << int(EffectiveStatus::PermissionDenied) << qint64(-1)
                                << QStringLiteral("Kaeru: ERROR")
                                << QStringLiteral("Inactive") << false << true;
        QTest::newRow("failed") << int(EffectiveStatus::Failed) << qint64(-1)
                                << QStringLiteral("Kaeru: ERROR")
                                << QStringLiteral("Inactive") << false << true;
    }

    void menuState()
    {
        QFETCH(int, status);
        QFETCH(qint64, remainingMs);
        QFETCH(QString, statusLine);
        QFETCH(QString, detailLine);
        QFETCH(bool, active);
        QFETCH(bool, error);

        const TrayMenuState m = decideTrayMenuState(
            static_cast<EffectiveStatus>(status), remainingMs);
        QCOMPARE(m.statusLine, statusLine);
        QCOMPARE(m.detailLine, detailLine);
        QCOMPARE(m.active, active);
        QCOMPARE(m.error, error);
    }
};

QTEST_MAIN(TestTrayState)
#include "test_traystate.moc"
