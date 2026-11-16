#include "FocusController.h"
#include "TimerEngine.h"

namespace kaeru {

FocusController::FocusController(TimerEngine* timers, QObject* parent)
    : QObject(parent)
    , m_timers(timers)
{
    if (m_timers) {
        connect(m_timers, &TimerEngine::completed, this, [this](const QString& id, const QString& label) {
            Q_UNUSED(label)
            if (id == m_countdownId) {
                // Capture planned/achieved before finishInterval() clears the
                // timer id (work-log recording + honest reporting).
                qint64 plannedMs = 0, achievedMs = 0;
                for (const TimerEngine::TimerInfo& t : m_timers->timers()) {
                    if (t.id == id) {
                        plannedMs = t.durationMs;
                        achievedMs = t.elapsedMs;
                        break;
                    }
                }
                finishInterval(plannedMs, achievedMs);
            }
        });
    }
}

void FocusController::setOptions(const Options& o)
{
    m_options = o;
    emit stateChanged();
}

bool FocusController::quietHoursActive() const
{
    if (!m_options.quietHoursEnabled)
        return false;
    const QTime now = QTime::currentTime();
    const int minutes = now.hour() * 60 + now.minute();
    if (m_options.quietFromMinutes == m_options.quietToMinutes)
        return false; // zero-length window = disabled
    if (m_options.quietFromMinutes < m_options.quietToMinutes)
        return minutes >= m_options.quietFromMinutes && minutes < m_options.quietToMinutes;
    // window wraps midnight
    return minutes >= m_options.quietFromMinutes || minutes < m_options.quietToMinutes;
}

void FocusController::requestKeepAwake(bool on)
{
    // Application wires this signal to the coordinator; core stays decoupled.
    Q_UNUSED(on)
}

void FocusController::beginInterval(Phase phase, qint64 plannedMs)
{
    m_phase = phase;
    m_pausedFrom = Phase::Idle;
    if (phase == Phase::Work)
        ++m_intervalNumber;
    const QString label = phaseLabel();
    m_countdownId = m_timers->startCountdown(plannedMs, label);
    emit intervalStarted(phase);
    emit stateChanged();
}

bool FocusController::startCustom(qint64 durationMs)
{
    if (m_phase != Phase::Idle || durationMs <= 0)
        return false;
    m_mode = Mode::Custom;
    m_completedWork = 0;
    m_intervalNumber = 0;
    beginInterval(Phase::Work, durationMs);
    return true;
}

bool FocusController::startPomodoro()
{
    if (m_phase != Phase::Idle)
        return false;
    m_mode = Mode::Pomodoro;
    m_completedWork = 0;
    m_intervalNumber = 0;
    qint64 work = m_options.workMs;
    if (m_upcomingWorkOverrideMs > 0) {
        work = m_upcomingWorkOverrideMs;
        m_upcomingWorkOverrideMs = 0;
    }
    beginInterval(Phase::Work, work);
    return true;
}

void FocusController::finishInterval(qint64 plannedMs, qint64 achievedMs)
{
    if (m_phase == Phase::Idle || m_phase == Phase::Paused)
        return;
    const Phase finished = m_phase;
    m_countdownId.clear();
    emit intervalCompleted(finished, plannedMs, achievedMs);

    if (m_mode == Mode::Custom) {
        m_phase = Phase::Idle;
        m_intervalNumber = 0;
        emit stoppedIdle();
        emit stateChanged();
        return;
    }
    if (finished == Phase::Work)
        ++m_completedWork;

    const Phase next = nextPhaseAfter(finished);
    if (m_options.autoNext) {
        beginInterval(next, next == Phase::Work ? m_options.workMs
                                                : (next == Phase::ShortBreak ? m_options.shortBreakMs
                                                                             : m_options.longBreakMs));
    } else {
        m_phase = Phase::Idle;
        emit stateChanged();
    }
}

FocusController::Phase FocusController::nextPhaseAfter(Phase finished) const
{
    if (finished == Phase::Work) {
        if (m_completedWork > 0 && m_completedWork % (m_options.longBreakEvery <= 0 ? 4 : m_options.longBreakEvery) == 0)
            return Phase::LongBreak;
        return Phase::ShortBreak;
    }
    return Phase::Work;
}

bool FocusController::pause()
{
    if (m_phase == Phase::Idle || m_phase == Phase::Paused)
        return false;
    m_pausedFrom = m_phase;
    // Freeze remaining time BEFORE switching to Paused (remainingMs() for a
    // paused controller reads the frozen value).
    const qint64 remaining = remainingMs();
    m_phase = Phase::Paused;
    // Countdown semantics: pausing freezes the interval; we model this by
    // cancelling the timer and re-creating it with the remaining time on
    // resume (timers are boot-clock anchored).
    if (m_timers && !m_countdownId.isEmpty()) {
        m_timers->cancel(m_countdownId);
        m_pausedRemainingMs = remaining < 0 ? 0 : remaining;
        m_countdownId.clear();
    }
    emit stateChanged();
    return true;
}

bool FocusController::resume()
{
    if (m_phase != Phase::Paused)
        return false;
    m_phase = m_pausedFrom;
    m_pausedFrom = Phase::Idle;
    m_countdownId = m_timers->startCountdown(m_pausedRemainingMs, phaseLabel());
    emit stateChanged();
    return true;
}

bool FocusController::stop()
{
    if (m_phase == Phase::Idle)
        return false;
    clearCurrent();
    m_phase = Phase::Idle;
    m_pausedFrom = Phase::Idle;
    emit stoppedIdle();
    emit stateChanged();
    return true;
}

void FocusController::clearCurrent()
{
    if (m_timers && !m_countdownId.isEmpty()) {
        m_timers->cancel(m_countdownId);
        m_countdownId.clear();
    }
    m_pausedRemainingMs = 0;
}

bool FocusController::skipInterval()
{
    if (m_phase == Phase::Idle || m_phase == Phase::Paused)
        return false;
    if (m_mode == Mode::Custom) {
        return stop();
    }
    // Pomodoro: advance as if the interval completed (achieved time is what
    // actually elapsed; counted time excludes suspend per recorder policy).
    qint64 plannedMs = 0, achievedMs = 0;
    for (const TimerEngine::TimerInfo& t : m_timers->timers()) {
        if (t.id == m_countdownId) {
            plannedMs = t.durationMs;
            achievedMs = t.elapsedMs;
            break;
        }
    }
    finishInterval(plannedMs, achievedMs);
    return true;
}

bool FocusController::extendInterval(qint64 ms)
{
    if (m_phase == Phase::Idle || m_phase == Phase::Paused || ms <= 0)
        return false;
    if (m_timers && !m_countdownId.isEmpty()) {
        const qint64 remaining = remainingMs();
        m_timers->cancel(m_countdownId);
        m_countdownId = m_timers->startCountdown(remaining + ms, phaseLabel());
        emit stateChanged();
        return true;
    }
    return false;
}

bool FocusController::setUpcomingWorkMs(qint64 ms)
{
    if (ms <= 0)
        return false;
    m_upcomingWorkOverrideMs = ms;
    emit stateChanged();
    return true;
}

qint64 FocusController::remainingMs() const
{
    if (m_phase == Phase::Idle)
        return -1;
    if (m_phase == Phase::Paused)
        return m_pausedRemainingMs;
    if (!m_timers || m_countdownId.isEmpty())
        return -1;
    // Remaining = planned - elapsed for the active countdown. The engine
    // stores anchors internally; approximate via timers() listing.
    for (const TimerEngine::TimerInfo& t : m_timers->timers()) {
        if (t.id != m_countdownId)
            continue;
        const qint64 remaining = t.durationMs - t.elapsedMs;
        return remaining > 0 ? remaining : 0;
    }
    return -1;
}

QString FocusController::phaseLabel() const
{
    switch (m_phase) {
    case Phase::Work: return Mode::Pomodoro == m_mode ? QStringLiteral("Work interval") : QStringLiteral("Countdown");
    case Phase::ShortBreak: return QStringLiteral("Short break");
    case Phase::LongBreak: return QStringLiteral("Long break");
    case Phase::Paused: return QStringLiteral("Paused");
    case Phase::Idle: return QStringLiteral("Idle");
    }
    return QString();
}

} // namespace kaeru
