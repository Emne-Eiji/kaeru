// Kaeru — timer engine (spec §6.3/§6.6, ADR-003)
//
// Three timer semantics:
//  - CountdownTimer (suspend-inclusive, BootClock): keeps running across
//    suspend, completes exactly once after resume.
//  - StopwatchTimer (suspend-exclusive, MonotonicClock): accrues work time;
//    on suspend the accrual pauses and stays paused after resume (default).
//  - WallDeadlineTimer (calendar deadline): fires when the wall clock passes
//    the stored UTC instant; re-derived from the deadline, never accumulated.
//
// Engines are driven by tick() from a single low-frequency QTimer (1 s when
// active); no high-frequency polling (spec resource targets).
#pragma once
#include "Clock.h"
#include "Types.h"

#include <QDateTime>
#include <QObject>
#include <QTimeZone>
#include <QUuid>

namespace kaeru {

class TimerEngine final : public QObject {
    Q_OBJECT
public:
    // kind: suspendInclusive countdown | suspendExclusive stopwatch | wallDeadline
    enum class Kind : quint8 { SuspendInclusiveCountdown, SuspendExclusiveStopwatch, WallDeadline };

    explicit TimerEngine(QObject* parent = nullptr);
    void setClocks(const ClockSet& clocks);

    // Countdown: completes when elapsed >= durationMs (measured on BootClock,
    // suspend-inclusive). Fires Completed once.
    QString startCountdown(qint64 durationMs, const QString& label);

    // Stopwatch: accrues while running except during suspend. Suspend pauses
    // accrual (recorded via suspendBoundary) — caller decides resume policy.
    QString startStopwatch(const QString& label);
    qint64 stopwatchElapsedMs(const QString& id) const;

    // Wall deadline: completes once wall clock passes the instant.
    QString startDeadline(const QDateTime& utcDeadline, const QString& label, const QString& tzId);

    void cancel(const QString& id);
    void tick(); // drive all timers; also detects wall-clock changes

    struct TimerInfo {
        QString id;
        Kind kind;
        QString label;
        qint64 durationMs = 0;      // countdown: planned
        qint64 elapsedMs = 0;       // stopwatch: accrued
        QDateTime deadlineUtc;      // deadline timers
        QString timeZoneId;
        bool suspended = false;     // stopwatch paused due to suspend
        bool done = false;
    };
    QList<TimerInfo> timers() const;

    // Suspend boundary handling (called by PowerMonitor):
    void onPrepareForSleep(bool start);

signals:
    void completed(const QString& id, const QString& label);
    void changed();

private:
    struct Timer {
        QUuid id;
        Kind kind;
        QString label;
        qint64 plannedMs = 0;      // countdown
        qint64 startBootMs = 0;    // countdown anchor
        qint64 accruedMs = 0;      // stopwatch
        qint64 lastResumeMonoMs = 0;
        bool suspendedByClock = false;
        QDateTime deadlineUtc;
        QTimeZone tz;
        bool done = false;
    };
    qint64 bootNow() const;
    qint64 monoNow() const;

    ClockSet m_clocks;
    QList<Timer> m_timers;
    QDateTime m_lastWallUtc;
    bool m_systemSuspending = false;
};

} // namespace kaeru
