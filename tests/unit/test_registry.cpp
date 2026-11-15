// Kaeru unit tests — inhibition request registry (spec §6.2)
#include "core/InhibitionRegistry.h"
#include "core/Types.h"

#include <QDateTime>
#include <QtTest>

using namespace kaeru;

class TestRegistry final : public QObject {
    Q_OBJECT

private slots:
    void addAssignsIdsAndDefaults()
    {
        InhibitionRegistry r;
        InhibitionRequest req;
        req.source = RequestSource::Countdown;
        req.capabilities = Capability::SystemSleep;
        req.reason = QStringLiteral("test");
        req.expiration = ExpirationPolicy::Duration;
        req.durationMs = 1000;

        const QUuid id = r.add(req);
        QVERIFY(!id.isNull());
        QCOMPARE(r.requests().size(), 1);
        QCOMPARE(r.contains(id), true);
        // Defaults filled: id + created_at
        QCOMPARE(r.requests().first().id, id);
        QVERIFY(r.requests().first().createdAt.isValid());
        QCOMPARE(r.requests().first().source, RequestSource::Countdown);
    }

    void removeIsExact()
    {
        InhibitionRegistry r;
        InhibitionRequest req;
        req.capabilities = Capability::SessionIdle;
        const QUuid id = r.add(req);
        QCOMPARE(r.remove(id), true);
        QCOMPARE(r.remove(id), false); // idempotent remove
        QCOMPARE(r.requests().isEmpty(), true);
        QCOMPARE(r.contains(id), false);
    }

    void expireElapsedDuration()
    {
        InhibitionRegistry r;
        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::Duration;
        req.durationMs = 1000;
        req.createdAt = QDateTime(QDate(2026, 11, 15), QTime(12, 0), Qt::UTC);
        r.add(req);

        const QDateTime before = req.createdAt.addMSecs(999);
        QVERIFY(r.expireElapsed(before).isEmpty());

        const QDateTime at = req.createdAt.addMSecs(1000);
        const QList<QUuid> expired = r.expireElapsed(at);
        QCOMPARE(expired.size(), 1);
        QCOMPARE(r.requests().isEmpty(), true);
    }

    void expireElapsedUntilTime()
    {
        InhibitionRegistry r;
        InhibitionRequest a;
        a.capabilities = Capability::SystemSleep;
        a.expiration = ExpirationPolicy::UntilTime;
        a.until = QDateTime(QDate(2026, 11, 15), QTime(17, 0), Qt::UTC);
        const QUuid idA = r.add(a);

        InhibitionRequest b = a;
        b.until = QDateTime(QDate(2026, 11, 15), QTime(18, 0), Qt::UTC);
        const QUuid idB = r.add(b);

        const QList<QUuid> gone = r.expireElapsed(QDateTime(QDate(2026, 11, 15), QTime(17, 0, 1), Qt::UTC));
        QCOMPARE(gone, QList<QUuid> { idA });
        QCOMPARE(r.contains(idB), true);
    }

    void earliestRemaining()
    {
        InhibitionRegistry r;
        QCOMPARE(r.earliestRemainingMs(QDateTime::currentDateTimeUtc()), qint64(-1));

        const QDateTime now = QDateTime(QDate(2026, 11, 15), QTime(12, 0), Qt::UTC);
        InhibitionRequest indefinite;
        indefinite.capabilities = Capability::SystemSleep;
        indefinite.expiration = ExpirationPolicy::None;
        r.add(indefinite);
        QCOMPARE(r.earliestRemainingMs(now), qint64(-1)); // indefinite never expires

        InhibitionRequest soon;
        soon.capabilities = Capability::SessionIdle;
        soon.expiration = ExpirationPolicy::Duration;
        soon.createdAt = now;
        soon.durationMs = 5000;
        r.add(soon);
        QCOMPARE(r.earliestRemainingMs(now), qint64(5000));

        InhibitionRequest sooner;
        sooner.capabilities = Capability::SessionIdle;
        sooner.expiration = ExpirationPolicy::Duration;
        sooner.createdAt = now;
        sooner.durationMs = 2000;
        r.add(sooner);
        QCOMPARE(r.earliestRemainingMs(now), qint64(2000));
    }

    void primaryReasonByPriority()
    {
        InhibitionRegistry r;
        QCOMPARE(r.primaryReason(), QString());

        InhibitionRequest low;
        low.capabilities = Capability::SystemSleep;
        low.reason = QStringLiteral("low");
        low.priority = 1;
        r.add(low);
        QCOMPARE(r.primaryReason(), QStringLiteral("low"));

        InhibitionRequest high;
        high.capabilities = Capability::SessionIdle;
        high.reason = QStringLiteral("high");
        high.priority = 50;
        r.add(high);
        QCOMPARE(r.primaryReason(), QStringLiteral("high"));
    }

    void clearAndChangedSignal()
    {
        InhibitionRegistry r;
        QSignalSpy spy(&r, &InhibitionRegistry::changed);
        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        const QUuid id = r.add(req);
        QCOMPARE(spy.count(), 1);
        r.clear();
        QCOMPARE(spy.count(), 2);
        QCOMPARE(r.contains(id), false);
    }
};

QTEST_MAIN(TestRegistry)
#include "test_registry.moc"
