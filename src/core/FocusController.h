// Kaeru — Focus module state machine (spec §6.3, WP-04)
//
// Routines: single custom countdown or Pomodoro (work → short break … long
// break after N work intervals). Interval progression uses suspend-inclusive
// countdowns; recorded work accrual is suspend-exclusive (handled via signals
// consumed by Application, which forwards to WorkSessionManager).
#pragma once
#include "Types.h"

#include <QObject>

namespace kaeru {

class TimerEngine;

class FocusController final : public QObject {
    Q_OBJECT
public:
    enum class Phase : quint8 { Idle, Work, ShortBreak, LongBreak, Paused };
    enum class Mode : quint8 { Custom, Pomodoro };

    struct Options {
        qint64 workMs = 25 * 60 * 1000;
        qint64 shortBreakMs = 5 * 60 * 1000;
        qint64 longBreakMs = 15 * 60 * 1000;
        int longBreakEvery = 4;
        bool autoNext = false;          // off by default (spec)
        bool keepAwakeDuringWork = false;
        bool keepAwakeDuringBreaks = false;
        bool recordInWorkLog = false;   // off until enabled (spec)
        bool notifyOnInterval = true;
        bool soundOnNotify = false;
        bool quietHoursEnabled = false;
        int quietFromMinutes = 0;       // minutes from local midnight
        int quietToMinutes = 0;
    };

    explicit FocusController(TimerEngine* timers, QObject* parent = nullptr);

    void setOptions(const Options& o);
    const Options& options() const { return m_options; }

    Phase phase() const { return m_phase; }
    Phase pausedFrom() const { return m_pausedFrom; }
    Mode mode() const { return m_mode; }
    int completedWorkIntervals() const { return m_completedWork; }
    int currentIntervalNumber() const { return m_intervalNumber; }

    // Commands (all return false when not applicable)
    bool startCustom(qint64 durationMs);
    bool startPomodoro();
    bool pause();
    bool resume();
    bool stop();
    bool skipInterval();            // advance to next interval immediately
    bool extendInterval(qint64 ms); // extend current interval (default +5 min)
    bool setUpcomingWorkMs(qint64 ms); // change duration of the NEXT work interval

    qint64 remainingMs() const;     // -1 when idle
    QString phaseLabel() const;

    // Quiet-hours evaluation (respects user setting; never changes system DND)
    bool quietHoursActive() const;

signals:
    void stateChanged();
    void intervalCompleted(Phase finishedPhase, qint64 plannedMs, qint64 achievedMs);
    void intervalStarted(Phase phase);
    void stoppedIdle();

private:
    void beginInterval(Phase phase, qint64 plannedMs);
    void finishInterval(qint64 plannedMs = 0, qint64 achievedMs = 0);
    void clearCurrent();
    Phase nextPhaseAfter(Phase finished) const;
    void requestKeepAwake(bool on);

    TimerEngine* m_timers = nullptr;
    Options m_options;
    Phase m_phase = Phase::Idle;
    Phase m_pausedFrom = Phase::Idle;
    Mode m_mode = Mode::Custom;
    int m_completedWork = 0;
    int m_intervalNumber = 0;
    QString m_countdownId;          // TimerEngine countdown id
    QUuid m_awakeRequestId;         // InhibitionCoordinator request (via signal)
    qint64 m_upcomingWorkOverrideMs = 0;
    qint64 m_pausedRemainingMs = 0;
};

} // namespace kaeru
