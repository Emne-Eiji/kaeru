// Kaeru unit tests — single manual Keep Awake session (initial release)
#include "core/ManualAwakeSession.h"

#include <QDateTime>
#include <QtTest>

using namespace kaeru;

class TestManualAwake final : public QObject {
    Q_OBJECT

private slots:
    void timedRequestBuildsDurationAndCapabilities()
    {
        const InhibitionRequest r =
            ManualAwakeSession::timedRequest(90 * 60 * 1000, QStringLiteral("both"));
        QCOMPARE(r.source, RequestSource::Countdown);
        QCOMPARE(r.expiration, ExpirationPolicy::Duration);
        QCOMPARE(r.durationMs, qint64(90 * 60 * 1000));
        QVERIFY(r.capabilities.testFlag(Capability::SystemSleep));
        QVERIFY(r.capabilities.testFlag(Capability::SessionIdle));
        QCOMPARE(r.priority, 10);
    }

    void modesSelectCapabilities()
    {
        QCOMPARE(ManualAwakeSession::timedRequest(1000, QStringLiteral("system")).capabilities,
                 Capabilities(Capability::SystemSleep));
        QCOMPARE(ManualAwakeSession::timedRequest(1000, QStringLiteral("display")).capabilities,
                 Capabilities(Capability::SessionIdle));
        QCOMPARE(ManualAwakeSession::indefiniteRequest(QStringLiteral("both")).capabilities,
                 Capabilities(Capability::SystemSleep | Capability::SessionIdle));
    }

    void startAssignsIdentityAndTimestamp()
    {
        ManualAwakeSession s;
        QVERIFY(!s.isActive());
        const QUuid id = s.start(ManualAwakeSession::timedRequest(60 * 1000, QStringLiteral("both")));
        QVERIFY(!id.isNull());
        QVERIFY(s.isActive());
        QCOMPARE(s.requestId(), id);
        QVERIFY(s.request().createdAt.isValid());
        QCOMPARE(s.isIndefinite(), false);
    }

    void secondStartReplacesFirst()
    {
        ManualAwakeSession s;
        const QUuid first = s.start(ManualAwakeSession::timedRequest(15 * 60 * 1000, QStringLiteral("both")));
        const QUuid second = s.start(ManualAwakeSession::timedRequest(120 * 60 * 1000, QStringLiteral("both")));
        QVERIFY(first != second);
        QCOMPARE(s.requestId(), second);
        QCOMPARE(s.request().durationMs, qint64(120 * 60 * 1000));
        QCOMPARE(s.remainingMs(QDateTime::currentDateTimeUtc()) > 119 * 60 * 1000, true);
    }

    void stopClearsSession()
    {
        ManualAwakeSession s;
        s.start(ManualAwakeSession::indefiniteRequest(QStringLiteral("both")));
        QVERIFY(s.isIndefinite());
        s.stop();
        QVERIFY(!s.isActive());
        QVERIFY(s.requestId().isNull());
        QCOMPARE(s.remainingMs(QDateTime::currentDateTimeUtc()), qint64(-1));
    }

    void remainingTimeTracksExpiry()
    {
        ManualAwakeSession s;
        const QDateTime t0 = QDateTime(QDate(2026, 11, 16), QTime(12, 0), Qt::UTC);
        InhibitionRequest r = ManualAwakeSession::timedRequest(2 * 60 * 60 * 1000,
            QStringLiteral("both"));
        r.createdAt = t0;
        s.start(r);
        QCOMPARE(s.remainingMs(t0), qint64(2 * 60 * 60 * 1000));
        QCOMPARE(s.remainingMs(t0.addSecs(90 * 60)), qint64(30 * 60 * 1000));
        QCOMPARE(s.remainingMs(t0.addSecs(3 * 60 * 60)), qint64(0));
    }

    void indefiniteHasNoRemainingTime()
    {
        ManualAwakeSession s;
        s.start(ManualAwakeSession::indefiniteRequest(QStringLiteral("both")));
        QCOMPARE(s.remainingMs(QDateTime::currentDateTimeUtc()), qint64(-1));
    }

    void untilRequestExpiresAtWallInstant()
    {
        const QDateTime until = QDateTime(QDate(2026, 11, 16), QTime(18, 0), Qt::UTC);
        ManualAwakeSession s;
        s.start(ManualAwakeSession::untilRequest(until, QStringLiteral("both")));
        QCOMPARE(s.request().expiration, ExpirationPolicy::UntilTime);
        QCOMPARE(s.request().until, until);
        QCOMPARE(s.remainingMs(until.addSecs(-60)), qint64(60 * 1000));
        QCOMPARE(s.remainingMs(until.addSecs(60)), qint64(0));
    }

    void invalidRequestsAreRejected()
    {
        ManualAwakeSession s;
        InhibitionRequest empty;
        QVERIFY(s.start(empty).isNull());
        QVERIFY(!s.isActive());
        QVERIFY(s.start(ManualAwakeSession::timedRequest(0, QStringLiteral("both"))).isNull());
        InhibitionRequest badUntil = ManualAwakeSession::untilRequest(QDateTime(), QStringLiteral("both"));
        QVERIFY(s.start(badUntil).isNull());
    }
};

QTEST_MAIN(TestManualAwake)
#include "test_manualawake.moc"
