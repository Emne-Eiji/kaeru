// Kaeru unit tests — tray presentation mapping (ADR-008 frog identity)
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
        // Awake + focus: protection wins the icon; focus stays in the
        // structured result so the tooltip can state it (ADR-008).
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
        QCOMPARE(p.keepAwakeActive, false); // Pause All released everything
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
        // Activation pending must never show the awake face (no false claim).
        const TrayPresentation p = decideTrayPresentation(
            EffectiveStatus::Activating, false, false, QString(), QString());
        QCOMPARE(p.icon, TrayPresentation::Icon::Sleep);
        QCOMPARE(p.keepAwakeActive, false);
        QCOMPARE(p.summary, QStringLiteral("activating…"));
    }
};

QTEST_MAIN(TestTrayState)
#include "test_traystate.moc"