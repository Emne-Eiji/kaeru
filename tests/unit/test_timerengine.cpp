// Kaeru unit tests — timer engine semantics (ADR-003)
// Deterministic FakeTimeSource injection; suspend simulation via the
// suspend-boundary API, never via real suspend.
#include "core/Clock.h"
#include "core/TimerEngine.h"

#include <QDateTime>
#include <QtTest>

using namespace kaeru;

class TestTimerEngine final : public QObject {
    Q_OBJECT

private slots:
    void countdownCompletesOnceOnBootClock()
    {
        FakeTimeSource boot(0);
        TimerEngine e;
        ClockSet clocks;
        clocks.boot = &boot;
        e.setClocks(clocks);

        QSignalSpy completedSpy(&e, &TimerEngine::completed);
        const QString id = e.startCountdown(10000, QStringLiteral("Keep awake (test)"));
        QVERIFY(!id.isEmpty());
        QCOMPARE(e.timers().size(), 1);
        QCOMPARE(e.timers().first().kind, TimerEngine::Kind::SuspendInclusiveCountdown);

        boot.advanceMs(9999);
        e.tick();
        QCOMPARE(completedSpy.count(), 0); // not yet

        boot.advanceMs(1); // 10000 ms reached
        e.tick();
        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("Keep awake (test)"));
        QVERIFY(e.timers().isEmpty()); // completed timers are removed

        e.tick(); // extra ticks do not double-fire
        QCOMPARE(completedSpy.count(), 1);
    }

    void countdownFiresAfterSuspend()
    {
        // Suspend-inclusive countdown: completes exactly once after resume.
        FakeTimeSource boot(0);
        TimerEngine e;
        ClockSet clocks;
        clocks.boot = &boot;
        e.setClocks(clocks);

        QSignalSpy completedSpy(&e, &TimerEngine::completed);
        e.startCountdown(5000, QStringLiteral("suspend test"));
        boot.advanceMs(2000);
        e.tick();

        // Simulated suspend: boot clock does not advance during suspend.
        boot.advanceMs(0);
        e.onPrepareForSleep(true);
        boot.advanceMs(5000); // resumes past the deadline
        e.onPrepareForSleep(false);
        e.tick();
        QCOMPARE(completedSpy.count(), 1);
        e.tick();
        QCOMPARE(completedSpy.count(), 1);
    }

    void stopwatchAccruesOnlyWhileRunning()
    {
        FakeTimeSource mono(0);
        TimerEngine e;
        ClockSet clocks;
        clocks.monotonic = &mono;
        e.setClocks(clocks);

        const QString id = e.startStopwatch(QStringLiteral("work segment"));
        mono.advanceMs(10000);
        QCOMPARE(e.stopwatchElapsedMs(id), qint64(10000));

        // Suspend boundary: accrual pauses (suspend is not work).
        e.onPrepareForSleep(true);
        mono.advanceMs(3000); // "suspended" time is not counted
        QCOMPARE(e.stopwatchElapsedMs(id), qint64(10000));
        e.onPrepareForSleep(false);
        // Spec: after resume the stopwatch stays paused by default (the
        // caller starts a new segment); elapsed remains frozen.
        mono.advanceMs(2000);
        QCOMPARE(e.stopwatchElapsedMs(id), qint64(10000));
        QCOMPARE(e.timers().first().suspended, true);
    }

    void stopwatchStaysPausedAfterResumeByDefault()
    {
        // Spec: after resume the stopwatch stays paused (caller decides).
        FakeTimeSource mono(0);
        TimerEngine e;
        ClockSet clocks;
        clocks.monotonic = &mono;
        e.setClocks(clocks);

        const QString id = e.startStopwatch(QStringLiteral("segment"));
        mono.advanceMs(5000);
        e.onPrepareForSleep(true);
        mono.advanceMs(7000);
        e.onPrepareForSleep(false);
        QCOMPARE(e.stopwatchElapsedMs(id), qint64(5000));
        QCOMPARE(e.timers().first().suspended, true);

        mono.advanceMs(1000); // still paused after resume
        QCOMPARE(e.stopwatchElapsedMs(id), qint64(5000));
    }

    void wallDeadlineFiresOnPassing()
    {
        FakeTimeSource wall(QDateTime(QDate(2026, 11, 15), QTime(10, 0), Qt::UTC)
                                .toMSecsSinceEpoch());
        TimerEngine e;
        ClockSet clocks;
        clocks.wall = &wall;
        e.setClocks(clocks);

        QSignalSpy completedSpy(&e, &TimerEngine::completed);
        const QDateTime deadline = QDateTime(QDate(2026, 11, 15), QTime(10, 30), Qt::UTC);
        const QString id = e.startDeadline(deadline, QStringLiteral("until deadline"),
                                           QStringLiteral("UTC"));
        QVERIFY(!id.isEmpty());

        wall.advanceMs(29 * 60 * 1000);
        e.tick();
        QCOMPARE(completedSpy.count(), 0);
        wall.advanceMs(60 * 1000 + 1);
        e.tick();
        QCOMPARE(completedSpy.count(), 1);
    }

    void cancelRemovesTimer()
    {
        FakeTimeSource boot(0);
        TimerEngine e;
        ClockSet clocks;
        clocks.boot = &boot;
        e.setClocks(clocks);

        QSignalSpy completedSpy(&e, &TimerEngine::completed);
        const QString id = e.startCountdown(1000, QStringLiteral("x"));
        e.cancel(id);
        QVERIFY(e.timers().isEmpty());
        boot.advanceMs(2000);
        e.tick();
        QCOMPARE(completedSpy.count(), 0);
    }

    void invalidInputsRejected()
    {
        TimerEngine e;
        QCOMPARE(e.startCountdown(0, QStringLiteral("x")), QString());
        QCOMPARE(e.startCountdown(-1, QStringLiteral("x")), QString());
        QCOMPARE(e.startDeadline(QDateTime(), QStringLiteral("x"), QStringLiteral("UTC")), QString());
    }
};

QTEST_MAIN(TestTimerEngine)
#include "test_timerengine.moc"
