// Kaeru unit tests — notification policy (spec §6.3)
// Pure decisions: quiet hours and enablement gates; the policy itself never
// touches the system DND (verified by construction — no side effects).
#include "core/NotifyPolicy.h"

#include <QtTest>

using namespace kaeru;

class TestNotify final : public QObject {
    Q_OBJECT

private slots:
    void focusDisabledByOption()
    {
        const NotifyDecision d = decideFocusIntervalNotification(false, false);
        QCOMPARE(d.notify, false);
        QCOMPARE(d.reason, QStringLiteral("focus notifications disabled"));
    }

    void focusSuppressedByQuietHours()
    {
        const NotifyDecision d = decideFocusIntervalNotification(true, true);
        QCOMPARE(d.notify, false);
        QCOMPARE(d.reason, QStringLiteral("quiet hours active"));
    }

    void focusNotifiesWhenAllowed()
    {
        const NotifyDecision d = decideFocusIntervalNotification(true, false);
        QCOMPARE(d.notify, true);
        QCOMPARE(d.reason, QString());
    }

    void keepAwakeExpiryGate()
    {
        QCOMPARE(decideKeepAwakeExpiryNotification(false).notify, false);
        QCOMPARE(decideKeepAwakeExpiryNotification(false).reason,
                 QStringLiteral("keep-awake notifications disabled"));
        QCOMPARE(decideKeepAwakeExpiryNotification(true).notify, true);
        QCOMPARE(decideKeepAwakeExpiryNotification(true).reason, QString());
    }
};

QTEST_MAIN(TestNotify)
#include "test_notify.moc"