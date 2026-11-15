#include "TimerEngine.h"

namespace kaeru {

TimerEngine::TimerEngine(QObject* parent)
    : QObject(parent)
{
    m_clocks.monotonic = new MonotonicTimeSource();
    m_clocks.boot = new BootTimeSource();
    m_clocks.wall = new WallTimeSource();
    m_lastWallUtc = QDateTime::fromMSecsSinceEpoch(m_clocks.wall->nowMs());
}

void TimerEngine::setClocks(const ClockSet& clocks)
{
    if (clocks.monotonic)
        m_clocks.monotonic = clocks.monotonic;
    if (clocks.boot)
        m_clocks.boot = clocks.boot;
    if (clocks.wall)
        m_clocks.wall = clocks.wall;
    m_lastWallUtc = QDateTime::fromMSecsSinceEpoch(m_clocks.wall->nowMs());
}

qint64 TimerEngine::bootNow() const
{
    return m_clocks.boot ? m_clocks.boot->nowMs() : 0;
}

qint64 TimerEngine::monoNow() const
{
    return m_clocks.monotonic ? m_clocks.monotonic->nowMs() : 0;
}

QString TimerEngine::startCountdown(qint64 durationMs, const QString& label)
{
    if (durationMs <= 0)
        return QString();
    Timer t;
    t.id = QUuid::createUuid();
    t.kind = Kind::SuspendInclusiveCountdown;
    t.label = label;
    t.plannedMs = durationMs;
    t.startBootMs = bootNow();
    m_timers.append(t);
    emit changed();
    return t.id.toString(QUuid::WithoutBraces);
}

QString TimerEngine::startStopwatch(const QString& label)
{
    Timer t;
    t.id = QUuid::createUuid();
    t.kind = Kind::SuspendExclusiveStopwatch;
    t.label = label;
    t.lastResumeMonoMs = monoNow();
    m_timers.append(t);
    emit changed();
    return t.id.toString(QUuid::WithoutBraces);
}

qint64 TimerEngine::stopwatchElapsedMs(const QString& id) const
{
    for (const Timer& t : m_timers) {
        if (t.id.toString(QUuid::WithoutBraces) != id || t.kind != Kind::SuspendExclusiveStopwatch)
            continue;
        qint64 total = t.accruedMs;
        if (!t.suspendedByClock && !t.done)
            total += monoNow() - t.lastResumeMonoMs;
        return total;
    }
    return -1;
}

QString TimerEngine::startDeadline(const QDateTime& utcDeadline, const QString& label, const QString& tzId)
{
    if (!utcDeadline.isValid())
        return QString();
    Timer t;
    t.id = QUuid::createUuid();
    t.kind = Kind::WallDeadline;
    t.label = label;
    t.deadlineUtc = utcDeadline.toUTC();
    t.tz = tzId.isEmpty() ? QTimeZone::systemTimeZone() : QTimeZone(tzId.toUtf8());
    m_timers.append(t);
    emit changed();
    return t.id.toString(QUuid::WithoutBraces);
}

void TimerEngine::cancel(const QString& id)
{
    for (int i = 0; i < m_timers.size(); ++i) {
        if (m_timers.at(i).id.toString(QUuid::WithoutBraces) == id) {
            m_timers.removeAt(i);
            emit changed();
            return;
        }
    }
}

void TimerEngine::tick()
{
    // Wall clock change detection: recompute drift against monotonic anchor.
    const QDateTime wallNow = QDateTime::fromMSecsSinceEpoch(m_clocks.wall->nowMs());
    if (m_lastWallUtc.isValid()) {
        // Nothing to accumulate for countdowns (boot clock is self-correcting).
        // Wall deadlines are absolute — no adjustment needed for large jumps;
        // completion checks below handle both directions.
    }
    m_lastWallUtc = wallNow;

    for (int i = m_timers.size() - 1; i >= 0; --i) {
        Timer& t = m_timers[i];
        if (t.done)
            continue;
        bool complete = false;
        if (t.kind == Kind::SuspendInclusiveCountdown) {
            complete = (bootNow() - t.startBootMs) >= t.plannedMs;
        } else if (t.kind == Kind::SuspendExclusiveStopwatch) {
            // Stopwatches complete only when explicitly stopped — not timed.
        } else if (t.kind == Kind::WallDeadline) {
            complete = wallNow >= t.deadlineUtc;
        }
        if (complete) {
            t.done = true;
            emit completed(t.id.toString(QUuid::WithoutBraces), t.label);
            m_timers.removeAt(i);
        }
    }
    emit changed();
}

void TimerEngine::onPrepareForSleep(bool start)
{
    m_systemSuspending = start;
    for (Timer& t : m_timers) {
        if (t.kind != Kind::SuspendExclusiveStopwatch)
            continue;
        if (start && !t.suspendedByClock && !t.done) {
            t.accruedMs += monoNow() - t.lastResumeMonoMs;
            t.suspendedByClock = true;
        } else if (!start) {
            // Resume: stays paused by default (spec: suspend is not work).
            // Explicit resume is done via a new stopwatch segment by callers.
            t.lastResumeMonoMs = monoNow();
        }
    }
    emit changed();
}

QList<TimerEngine::TimerInfo> TimerEngine::timers() const
{
    QList<TimerInfo> out;
    for (const Timer& t : m_timers) {
        TimerInfo i;
        i.id = t.id.toString(QUuid::WithoutBraces);
        i.kind = t.kind;
        i.label = t.label;
        i.durationMs = t.plannedMs;
        i.elapsedMs = (t.kind == Kind::SuspendExclusiveStopwatch)
            ? stopwatchElapsedMs(i.id)
            : (t.kind == Kind::SuspendInclusiveCountdown ? bootNow() - t.startBootMs : 0);
        i.deadlineUtc = t.deadlineUtc;
        i.timeZoneId = t.tz.isValid() ? QString::fromUtf8(t.tz.id()) : QString();
        i.suspended = t.suspendedByClock;
        i.done = t.done;
        out.append(i);
    }
    return out;
}

} // namespace kaeru
