// Kaeru unit tests — focus state machine (spec §6.3)
#include "core/Clock.h"
#include "core/FocusController.h"
#include "core/TimerEngine.h"

#include <QtTest>

using namespace kaeru;

class TestFocus final : public QObject {
    Q_OBJECT

private slots:
    void init() // fresh engine per test function
    {
        m_boot = std::make_unique<FakeTimeSource>(0);
        m_mono = std::make_unique<FakeTimeSource>(0);
        m_wall = std::make_unique<FakeTimeSource>(0);
        m_engine = std::make_unique<TimerEngine>();
        ClockSet clocks;
        clocks.boot = m_boot.get();
        clocks.monotonic = m_mono.get();
        clocks.wall = m_wall.get();
        m_engine->setClocks(clocks);
        m_focus = std::make_unique<FocusController>(m_engine.get());
    }

    void defaultsAreConservative()
    {
        // Recording stays off until the user enables it (spec).
        QCOMPARE(m_focus->options().recordInWorkLog, false);
        QCOMPARE(m_focus->options().autoNext, false);
        QCOMPARE(m_focus->options().quietHoursEnabled, false);
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
        QCOMPARE(m_focus->remainingMs(), qint64(-1));
    }

    void customCountdownLifecycle()
    {
        QVERIFY(m_focus->startCustom(10 * 60 * 1000));
        QCOMPARE(m_focus->phase(), FocusController::Phase::Work);
        QVERIFY(m_focus->remainingMs() > 0);

        m_boot->advanceMs(5 * 60 * 1000);
        m_engine->tick();
        QCOMPARE(m_focus->remainingMs(), qint64(5 * 60 * 1000));

        QSignalSpy completedSpy(m_focus.get(), &FocusController::intervalCompleted);
        QSignalSpy idleSpy(m_focus.get(), &FocusController::stoppedIdle);
        m_boot->advanceMs(5 * 60 * 1000);
        m_engine->tick(); // completes the countdown → custom mode returns to idle
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(idleSpy.count(), 1);
    }

    void pauseFreezesAndResumeRestarts()
    {
        QVERIFY(m_focus->startCustom(10 * 60 * 1000));
        m_boot->advanceMs(4 * 60 * 1000);
        m_engine->tick();

        QVERIFY(m_focus->pause());
        QCOMPARE(m_focus->phase(), FocusController::Phase::Paused);
        QCOMPARE(m_focus->remainingMs(), qint64(6 * 60 * 1000));

        // Time passes while paused; remaining stays frozen.
        m_boot->advanceMs(2 * 60 * 1000);
        QCOMPARE(m_focus->remainingMs(), qint64(6 * 60 * 1000));

        QVERIFY(m_focus->resume());
        QCOMPARE(m_focus->phase(), FocusController::Phase::Work);
        QCOMPARE(m_focus->remainingMs(), qint64(6 * 60 * 1000));
        m_boot->advanceMs(6 * 60 * 1000);
        m_engine->tick();
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
    }

    void pauseResumeInvalidWhenNotApplicable()
    {
        QVERIFY(!m_focus->pause());
        QVERIFY(!m_focus->resume());
        QVERIFY(!m_focus->stop());
        QVERIFY(!m_focus->skipInterval());
        QVERIFY(!m_focus->extendInterval(60000));
        QVERIFY(!m_focus->startCustom(0));
        QVERIFY(!m_focus->startCustom(-5));
    }

    void pomodoroShortLongBreaks()
    {
        // One Pomodoro routine with autoNext: progression is
        // Work → ShortBreak → Work → LongBreak (long break after 2 work).
        FocusController::Options o;
        o.workMs = 10 * 60 * 1000;
        o.shortBreakMs = 1 * 60 * 1000;
        o.longBreakMs = 2 * 60 * 1000;
        o.longBreakEvery = 2;
        o.autoNext = true;
        m_focus->setOptions(o);

        QSignalSpy startedSpy(m_focus.get(), &FocusController::intervalStarted);
        QVERIFY(m_focus->startPomodoro()); // Work 1

        m_boot->advanceMs(o.workMs);
        m_engine->tick();
        QCOMPARE(m_focus->phase(), FocusController::Phase::ShortBreak);

        m_boot->advanceMs(o.shortBreakMs);
        m_engine->tick();
        QCOMPARE(m_focus->phase(), FocusController::Phase::Work); // Work 2

        m_boot->advanceMs(o.workMs);
        m_engine->tick();
        QCOMPARE(m_focus->phase(), FocusController::Phase::LongBreak);
        QCOMPARE(m_focus->completedWorkIntervals(), 2);
        QCOMPARE(startedSpy.count(), 4); // Work, ShortBreak, Work, LongBreak
    }

    void autoNextProgression()
    {
        FocusController::Options o;
        o.workMs = 1000;
        o.shortBreakMs = 1000;
        o.longBreakMs = 1000;
        o.longBreakEvery = 2;
        o.autoNext = true;
        m_focus->setOptions(o);

        QSignalSpy startedSpy(m_focus.get(), &FocusController::intervalStarted);
        QVERIFY(m_focus->startPomodoro());
        m_boot->advanceMs(1000);
        m_engine->tick();
        QTRY_COMPARE(m_focus->phase(), FocusController::Phase::ShortBreak);
        m_boot->advanceMs(1000);
        m_engine->tick();
        QTRY_COMPARE(m_focus->phase(), FocusController::Phase::Work); // work 2
        m_boot->advanceMs(1000);
        m_engine->tick();
        QTRY_COMPARE(m_focus->phase(), FocusController::Phase::LongBreak);
        // intervalStarted fires for: Work, ShortBreak, Work, LongBreak
        QCOMPARE(startedSpy.count(), 4);
    }

    void skipInPomodoroAdvances()
    {
        FocusController::Options o;
        o.workMs = 60 * 60 * 1000;
        m_focus->setOptions(o);
        QVERIFY(m_focus->startPomodoro());
        m_boot->advanceMs(5 * 60 * 1000);
        m_engine->tick();
        QVERIFY(m_focus->skipInterval()); // treat as completed, advance
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
        QCOMPARE(m_focus->completedWorkIntervals(), 1);
    }

    void skipInCustomStops()
    {
        QVERIFY(m_focus->startCustom(60 * 60 * 1000));
        m_boot->advanceMs(1000);
        QVERIFY(m_focus->skipInterval());
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
    }

    void extendAddsToRemaining()
    {
        QVERIFY(m_focus->startCustom(10 * 60 * 1000));
        m_boot->advanceMs(1 * 60 * 1000);
        m_engine->tick();
        QCOMPARE(m_focus->remainingMs(), qint64(9 * 60 * 1000));
        QVERIFY(m_focus->extendInterval(5 * 60 * 1000));
        QCOMPARE(m_focus->remainingMs(), qint64(14 * 60 * 1000));
    }

    void setUpcomingWorkChangesNextInterval()
    {
        FocusController::Options o;
        o.workMs = 1000;
        m_focus->setOptions(o);
        QVERIFY(m_focus->setUpcomingWorkMs(42 * 60 * 1000));
        QVERIFY(m_focus->startPomodoro());
        // First interval uses the override.
        QCOMPARE(m_focus->remainingMs(), qint64(42 * 60 * 1000));
    }

    void stopCancelsEverything()
    {
        QVERIFY(m_focus->startCustom(10 * 60 * 1000));
        m_boot->advanceMs(1000);
        m_engine->tick();
        QSignalSpy idleSpy(m_focus.get(), &FocusController::stoppedIdle);
        QVERIFY(m_focus->stop());
        QCOMPARE(m_focus->phase(), FocusController::Phase::Idle);
        QCOMPARE(idleSpy.count(), 1);
        QCOMPARE(m_engine->timers().isEmpty(), true); // no orphan countdowns
    }

    void quietHoursWindow()
    {
        FocusController::Options o = m_focus->options();
        o.quietHoursEnabled = true;
        o.quietFromMinutes = 22 * 60;
        o.quietToMinutes = 7 * 60; // wraps midnight
        m_focus->setOptions(o);
        // Result depends on the current wall time; only the wrap cases verify.
        const QTime now = QTime::currentTime();
        const int minutes = now.hour() * 60 + now.minute();
        const bool expected = minutes >= 22 * 60 || minutes < 7 * 60;
        QCOMPARE(m_focus->quietHoursActive(), expected);

        // Zero-length window is disabled even when enabled.
        o.quietFromMinutes = 0;
        o.quietToMinutes = 0;
        m_focus->setOptions(o);
        QCOMPARE(m_focus->quietHoursActive(), false);
        // Disabled flag short-circuits.
        o.quietHoursEnabled = false;
        o.quietFromMinutes = 0;
        o.quietToMinutes = 60;
        m_focus->setOptions(o);
        QCOMPARE(m_focus->quietHoursActive(), false);
    }

private:
    std::unique_ptr<FakeTimeSource> m_boot, m_mono, m_wall;
    std::unique_ptr<TimerEngine> m_engine;
    std::unique_ptr<FocusController> m_focus;
};

QTEST_MAIN(TestFocus)
#include "test_focus.moc"
