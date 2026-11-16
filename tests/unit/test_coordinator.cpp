// Kaeru unit tests — inhibition coordinator (spec §6.1/§6.2, ADR-002)
// Uses FakeInhibitionBackend only; fake results never count as native evidence.
#include "core/AppState.h"
#include "core/FakeInhibitionBackend.h"
#include "core/InhibitionCoordinator.h"

#include <QtTest>

using namespace kaeru;

class TestCoordinator final : public QObject {
    Q_OBJECT

private slots:
    void requestedIsNotActiveUntilConfirmed()
    {
        // Spec §14: a submitted request is NOT proof of active inhibition.
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.reason = QStringLiteral("manual");
        c.addRequest(req);

        EffectiveAwakeState s = c.effectiveState(QDateTime::currentDateTimeUtc());
        QCOMPARE(s.status, EffectiveStatus::Activating);
        QCOMPARE(s.requested.testFlag(Capability::SystemSleep), true);
        QCOMPARE(s.effective.testFlag(Capability::SystemSleep), false);

        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);
        QCOMPARE(backend.acquiredIds().size(), 1);
    }

    void expireRemovesBindingAndReleases()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::Duration;
        req.createdAt = QDateTime::currentDateTimeUtc();
        req.durationMs = 500;
        req.reason = QStringLiteral("Keep awake (test)");
        const QUuid id = c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);

        bool expiredFired = false;
        InhibitionRequest expiredPayload;
        connect(&c, &InhibitionCoordinator::expired,
                [&expiredFired, &expiredPayload, &id](const QUuid& r, const InhibitionRequest& request) {
                    expiredFired = (r == id);
                    expiredPayload = request;
                });

        // Before expiry the request is still held.
        c.tick(QDateTime::currentDateTimeUtc().addMSecs(499));
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                 EffectiveStatus::Active);
        QCOMPARE(expiredFired, false);

        // At expiry the binding is released (async activation has settled) and
        // the signal carries the request details, not an empty placeholder.
        c.tick(QDateTime::currentDateTimeUtc().addMSecs(600));
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Inactive);
        QCOMPARE(expiredFired, true);
        QCOMPARE(expiredPayload.id, id);
        QCOMPARE(expiredPayload.reason, QStringLiteral("Keep awake (test)"));
        QCOMPARE(expiredPayload.expiration, ExpirationPolicy::Duration);
        QCOMPARE(backend.acquiredIds().isEmpty(), true);
    }

    void suppressTurnsOffAndResumeRestores()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep | Capability::SessionIdle;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);

        c.suppressAll(QStringLiteral("turned off manually"));
        QCOMPARE(c.suppressed(), true);
        QCOMPARE(backend.acquiredIds().isEmpty(), true);
        EffectiveAwakeState off = c.effectiveState(QDateTime::currentDateTimeUtc());
        QCOMPARE(off.status, EffectiveStatus::Inactive);
        QCOMPARE(off.limitation, QStringLiteral("turned off manually"));

        // While suppressed, new requests are ignored (desired = none).
        InhibitionRequest ignored;
        ignored.capabilities = Capability::SystemSleep;
        c.addRequest(ignored);
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                 EffectiveStatus::Inactive);

        c.resumeAutomatic();
        EffectiveAwakeState back = c.effectiveState(QDateTime::currentDateTimeUtc());
        QCOMPARE(back.requested.testFlag(Capability::SystemSleep), true);
        QCOMPARE(backend.acquiredIds().isEmpty(), false);
    }

    void moduleDisableReleasesAll()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);

        state.setModuleEnabled(Module::KeepAwake, false);
        QCOMPARE(backend.acquiredIds().isEmpty(), true);
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                 EffectiveStatus::Inactive);
    }

    void pauseAllOverridesEverything()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);

        state.setPausedAll(true);
        QCOMPARE(backend.acquiredIds().isEmpty(), true);
        state.setPausedAll(false);
        // Pause All returns to safe inactive state; nothing auto-restarts (spec §7).
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                 EffectiveStatus::Inactive);
        QCOMPARE(c.requests().isEmpty(), true);
    }

    void failureIsReportedHonestlyWithFallback()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend portal;
        portal.setBackendId(QStringLiteral("portal"));
        portal.setSupported(Capability::SystemSleep);
        FakeInhibitionBackend logind;
        logind.setBackendId(QStringLiteral("logind"));
        logind.setSupported(Capability::SystemSleep);
        portal.failNext(false, QStringLiteral("portal rejected"));
        c.addBackend(&portal, 0);
        c.addBackend(&logind, 1);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);

        // Fallback backend takes over; effective becomes active via logind.
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).backend,
                 QStringLiteral("logind"));
    }

    void failureWithoutFallbackIsHonest()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend only;
        only.setSupported(Capability::SessionIdle);
        only.failNext(false, QStringLiteral("daemon declined"));
        c.addBackend(&only, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SessionIdle;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);

        // Failures are delivered asynchronously (queued): wait for honesty.
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Failed);
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).errorDetail,
                 QStringLiteral("daemon declined"));
        QCOMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).effective.testFlag(Capability::SessionIdle),
                 false);
    }

    void permissionDeniedIsDistinct()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend only;
        only.setSupported(Capability::SystemSleep);
        only.failNext(true, QStringLiteral("not allowed"));
        c.addBackend(&only, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);
        // The denial arrives asynchronously via the queued failure signal.
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::PermissionDenied);
    }

    void unsupportedCapabilityNeverFake()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        backend.setSupported(Capability::SystemSleep); // idle NOT supported
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep | Capability::SessionIdle;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);

        // System-sleep is acquired; idle is never faked (no backend supports it).
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::PartiallyActive);
        EffectiveAwakeState s = c.effectiveState(QDateTime::currentDateTimeUtc());
        QCOMPARE(s.effective.testFlag(Capability::SystemSleep), true);
        QCOMPARE(s.effective.testFlag(Capability::SessionIdle), false);
        QVERIFY(s.limitation.contains(QStringLiteral("session-idle")));
        QCOMPARE(backend.acquiredIds().size(), 1);
    }

    void bindingLostReacquires()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);

        const QStringList held = backend.acquiredIds();
        QVERIFY(!held.isEmpty());
        backend.simulateLost(held.first(), QStringLiteral("backend restart"));
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);
        QCOMPARE(backend.acquiredIds().size(), 1); // a fresh binding was acquired
    }

    void removalReleasesBinding()
    {
        AppState state;
        InhibitionCoordinator c;
        c.setAppState(&state);
        FakeInhibitionBackend backend;
        c.addBackend(&backend, 0);

        InhibitionRequest req;
        req.capabilities = Capability::SystemSleep;
        req.expiration = ExpirationPolicy::None;
        const QUuid id = c.addRequest(req);
        QTRY_COMPARE(c.effectiveState(QDateTime::currentDateTimeUtc()).status,
                     EffectiveStatus::Active);
        QCOMPARE(backend.acquiredIds().size(), 1);

        c.removeRequest(id);
        QTRY_COMPARE(backend.acquiredIds().isEmpty(), true);
    }
};

QTEST_MAIN(TestCoordinator)
#include "test_coordinator.moc"
