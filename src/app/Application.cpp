#include "Application.h"
#include "Paths.h"

#include "platform/AutostartBackend.h"
#include "platform/LockMonitor.h"
#include "platform/LogindInhibitBackend.h"
#include "platform/LogindPowerMonitor.h"
#include "platform/Notifications.h"
#include "platform/PortalInhibitBackend.h"
#include "platform/ThemeWatcher.h"

#include "WorkSessionManager.h"
#include "core/NotifyPolicy.h"
#include "storage/Migrations.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QTime>

#include <cstdio>

namespace kaeru {

Application::~Application() = default;

Application::Application(bool backgroundMode, QObject* parent)
    : QObject(parent)
    , m_background(backgroundMode)
    , m_settings(std::make_unique<Settings>(this))
{
    QDir().mkpath(Paths::configDir());
    QDir().mkpath(Paths::dataDir());
    QDir().mkpath(Paths::cacheDir());

    // Settings → module switches
    m_state.setModuleEnabled(Module::KeepAwake, m_settings->moduleEnabled(Module::KeepAwake));
    m_state.setModuleEnabled(Module::Focus, m_settings->moduleEnabled(Module::Focus));
    m_state.setModuleEnabled(Module::WorkLog, m_settings->moduleEnabled(Module::WorkLog));

    m_focus = new FocusController(&m_timers, this);
    applyFocusSettingsFromSettings();

    setupBackends();
    setupStorage();

    // Focus controller wiring: keep awake during focus/breaks per settings,
    // notifications on interval completion (policy-gated: notify option +
    // quiet hours; never touches system DND).
    connect(m_focus, &FocusController::intervalStarted, this, [this](FocusController::Phase phase) {
        const bool isBreak = phase == FocusController::Phase::ShortBreak || phase == FocusController::Phase::LongBreak;
        const bool keepAwake = isBreak ? m_settings->focusKeepAwakeDuringBreaks() : m_settings->focusKeepAwakeDuringWork();
        applyFocusKeepAwake(keepAwake, isBreak ? QStringLiteral("Focus break") : QStringLiteral("Focus session"));
    });
    connect(m_focus, &FocusController::stoppedIdle, this, [this] { applyFocusKeepAwake(false, QString()); });
    connect(m_focus, &FocusController::intervalCompleted, this,
        [this](FocusController::Phase phase, qint64 planned, qint64 achieved) {
            // Optional link (spec §6.4): record completed work intervals into
            // focus_sessions (never into work-log segments — no double
            // counting) when the user enabled recording.
            if (phase == FocusController::Phase::Work && m_work
                && m_settings->focusRecordInWorkLog())
                m_work->recordFocusWork(planned, achieved, nullptr);
            emit focusIntervalFinished(phase);
        });

    // Focus interval notifications (WP-04): plain desktop notification with
    // optional XDG sound name; suppressed by the notify option / quiet hours.
    connect(this, &Application::focusIntervalFinished, this, [this](FocusController::Phase phase) {
        if (!m_notifications || !m_notifications->available())
            return;
        const bool notifyEnabled = m_settings->notificationsEnabled()
            && m_focus->options().notifyOnInterval;
        const bool quietActive = m_focus->quietHoursActive();
        const NotifyDecision decision = decideFocusIntervalNotification(notifyEnabled, quietActive);
        if (qEnvironmentVariableIsSet("KAERU_DEBUG_SETTINGS"))
            std::fprintf(stderr, "[kaeru] notify decision: want=%d quiet=%d → notify=%d (%s)\n",
                         notifyEnabled ? 1 : 0, quietActive ? 1 : 0,
                         decision.notify ? 1 : 0, qPrintable(decision.reason));
        if (!decision.notify)
            return;
        const QString sound = m_settings->soundEnabled() ? QStringLiteral("complete") : QString();
        if (phase == FocusController::Phase::Work
            && m_focus->mode() == FocusController::Mode::Custom) {
            m_notifications->notify(tr("Focus countdown complete"),
                tr("Your focus countdown has finished."), QString(), sound);
        } else if (phase == FocusController::Phase::Work) {
            const bool longBreak = m_focus->completedWorkIntervals() > 0
                && m_focus->completedWorkIntervals() % (m_focus->options().longBreakEvery <= 0
                                                            ? 4 : m_focus->options().longBreakEvery) == 0;
            m_notifications->notify(tr("Focus interval complete"),
                longBreak ? tr("Time for a long break.") : tr("Time for a short break."),
                QString(), sound);
        } else {
            m_notifications->notify(tr("Break over"),
                m_focus->options().autoNext
                    ? tr("The next focus interval has started.")
                    : tr("Ready for the next focus interval."),
                QString(), sound);
        }
    });

    // Timed keep-awake expiry notification (user setting).
    connect(this, &Application::keepAwakeExpired, this, [this](const QString& reason) {
        if (!m_notifications || !m_notifications->available())
            return;
        const NotifyDecision decision =
            decideKeepAwakeExpiryNotification(m_settings->notificationsEnabled()
                                              && m_settings->keepAwakeNotifications());
        if (!decision.notify)
            return;
        const QString sound = m_settings->soundEnabled() ? QStringLiteral("complete") : QString();
        m_notifications->notify(tr("Keep awake ended"), reason, QString(), sound);
    });

    // Platform monitors
    if (m_powerMonitor) {
        connect(m_powerMonitor.get(), &LogindPowerMonitor::prepareForSleep, this, &Application::onPrepareForSleep);
        m_capabilities.update(QStringLiteral("suspend-events"), {
            QStringLiteral("suspend-events"), QStringLiteral("Suspend/resume events"),
            m_powerMonitor->available() ? QStringLiteral("available") : QStringLiteral("unavailable"),
            QStringLiteral("logind"), m_powerMonitor->available()
                ? QStringLiteral("logind PrepareForSleep signal connected")
                : QStringLiteral("logind unavailable — suspend boundary not detected") });
    }
    if (m_lockMonitor) {
        connect(m_lockMonitor.get(), &LockMonitor::lockStateChanged, this, &Application::onLockChanged);
        m_capabilities.update(QStringLiteral("screen-lock-detection"), {
            QStringLiteral("screen-lock-detection"), QStringLiteral("Screen lock detection"),
            m_lockMonitor->available() ? QStringLiteral("available") : QStringLiteral("unavailable"),
            m_lockMonitor->available() ? m_lockMonitor->sourceName() : QString(),
            m_lockMonitor->available() ? QStringLiteral("lock events observed")
                                       : QStringLiteral("no lock signal source found") });
    }
    if (m_notifications) {
        m_capabilities.update(QStringLiteral("notifications"), {
            QStringLiteral("notifications"), QStringLiteral("Desktop notifications"),
            m_notifications->available() ? QStringLiteral("available") : QStringLiteral("unavailable"),
            QStringLiteral("org.freedesktop.Notifications"),
            m_notifications->available() ? QStringLiteral("ready") : QStringLiteral("notification daemon not found") });
    }
    m_capabilities.update(QStringLiteral("idle-detection"), {
        QStringLiteral("idle-detection"), QStringLiteral("Idle detection (input activity)"),
        QStringLiteral("unavailable"), QString(),
        QStringLiteral("out of scope for 1.0; manual tracking is unaffected") });

    // Timer completions (countdown expiry for keep awake; focus handled by controller)
    connect(&m_timers, &TimerEngine::completed, this, &Application::onTimerCompleted);

    startTickTimer();
}

void Application::setupStorage()
{
    m_db = std::make_unique<Database>();
    QString err;
    if (!m_db->open(&err) || !Migrations::apply(m_db->handle(), &err)) {
        m_storageReady = false;
        m_capabilities.update(QStringLiteral("work-log-storage"), {
            QStringLiteral("work-log-storage"), QStringLiteral("Work Log storage (SQLite)"),
            QStringLiteral("failed"), QString(),
            tr("Cannot open the work-log database: %1").arg(err) });
        return;
    }
    m_storageReady = true;
    m_projects = std::make_unique<ProjectRepository>(m_db->handle());
    m_sessions = std::make_unique<SessionRepository>(m_db->handle());
    m_work = std::make_unique<WorkSessionManager>(m_sessions.get(), m_projects.get(), this);
    m_work->setSettings(m_settings.get());

    // Conservative recovery of interrupted sessions (never invents work).
    const WorkSessionManager::RecoveryOutcome recovery = m_work->recoverInterrupted();
    for (const QString& note : recovery.notes)
        m_recoveryNotes << note;

    m_capabilities.update(QStringLiteral("work-log-storage"), {
        QStringLiteral("work-log-storage"), QStringLiteral("Work Log storage (SQLite)"),
        QStringLiteral("available"), QStringLiteral("QSQLITE + WAL"),
        tr("Database at %1; versioned migrations applied")
            .arg(m_db->filePath()) });
}

void Application::setupBackends()
{
    m_portalBackend = std::make_unique<PortalInhibitBackend>(this);
    m_logindBackend = std::make_unique<LogindInhibitBackend>(this);
    m_powerMonitor = std::make_unique<LogindPowerMonitor>(this);
    m_lockMonitor = std::make_unique<LockMonitor>(this);
    m_notifications = std::make_unique<Notifications>(this);
    m_autostart = std::make_unique<AutostartBackend>(this);
    m_themeWatcher = std::make_unique<ThemeWatcher>(this);

    // Portal preferred (session-integrated), logind fallback for system sleep.
    m_inhibition.addBackend(m_portalBackend.get(), 0);
    m_inhibition.addBackend(m_logindBackend.get(), 1);
    m_inhibition.setAppState(&m_state);

    // Capability registry (honest reporting)
    const Capabilities portalCaps = m_portalBackend->supportedCapabilities();
    const Capabilities logindCaps = m_logindBackend->supportedCapabilities();
    m_capabilities.update(QStringLiteral("system-sleep-inhibition"), {
        QStringLiteral("system-sleep-inhibition"), QStringLiteral("Prevent automatic system sleep"),
        (portalCaps.testFlag(Capability::SystemSleep) || logindCaps.testFlag(Capability::SystemSleep))
            ? QStringLiteral("available") : QStringLiteral("unavailable"),
        portalCaps.testFlag(Capability::SystemSleep) ? QStringLiteral("portal (logind fallback)")
                                                     : (logindCaps.testFlag(Capability::SystemSleep) ? QStringLiteral("logind") : QString()),
        portalCaps.testFlag(Capability::SystemSleep)
            ? m_portalBackend->availabilityDetail()
            : (logindCaps.testFlag(Capability::SystemSleep) ? m_logindBackend->availabilityDetail()
                                                            : QStringLiteral("no backend available")) });
    m_capabilities.update(QStringLiteral("session-idle-inhibition"), {
        QStringLiteral("session-idle-inhibition"), QStringLiteral("Keep display awake (idle inhibition)"),
        portalCaps.testFlag(Capability::SessionIdle) ? QStringLiteral("available") : QStringLiteral("unavailable"),
        portalCaps.testFlag(Capability::SessionIdle) ? QStringLiteral("portal") : QString(),
        portalCaps.testFlag(Capability::SessionIdle)
            ? m_portalBackend->availabilityDetail()
            : QStringLiteral("portal idle inhibition unavailable; logind cannot keep a display awake — not claimed") });
}

void Application::startTickTimer()
{
    m_tickTimer.setInterval(1000);
    connect(&m_tickTimer, &QTimer::timeout, this, &Application::onTick);
    m_tickTimer.start();
}

void Application::onTick()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    m_timers.tick();
    m_inhibition.tick(now);
    if (m_work)
        m_work->tick(); // 60 s recovery checkpoints while recording
}

void Application::onTimerCompleted(const QString& id, const QString& label)
{
    Q_UNUSED(id)
    // Countdown completions here are keep-awake timers created via awakeUntil
    // / indefinite helpers. Focus intervals are completed via FocusController.
    if (label.startsWith(QStringLiteral("Keep awake")))
        emit keepAwakeExpired(label);
}

void Application::onPrepareForSleep(bool start)
{
    m_timers.onPrepareForSleep(start);
    if (m_work)
        m_work->onSuspendBoundary(start);
    if (start) {
        // Explicitly requested awake sessions may legally persist — the
        // inhibitor fd stays owned by the kernel lock across suspend. Per
        // spec we do NOT force-release on suspend; reevaluate on wake.
    } else {
        // Reevaluate awake deadlines after resume.
        m_inhibition.tick(QDateTime::currentDateTimeUtc());
    }
}

void Application::onLockChanged(bool locked)
{
    if (m_work)
        m_work->onLockChanged(locked);
}

void Application::awakeOn(qint64 durationMs, const QString& mode)
{
    if (!m_state.isModuleEnabled(Module::KeepAwake))
        return;
    InhibitionRequest r;
    r.source = RequestSource::Countdown;
    r.reason = QStringLiteral("Keep awake (timed)");
    r.priority = 10;
    if (durationMs > 0) {
        r.expiration = ExpirationPolicy::Duration;
        r.durationMs = durationMs;
    }
    if (mode == QStringLiteral("system"))
        r.capabilities = Capability::SystemSleep;
    else if (mode == QStringLiteral("display"))
        r.capabilities = Capability::SessionIdle;
    else
        r.capabilities = Capability::SystemSleep | Capability::SessionIdle;
    m_inhibition.addRequest(r);
}

void Application::awakeOff()
{
    // "Turn off Keep Awake now": stop protection immediately and suppress
    // automatic reactivation until the user resumes (spec §6.2).
    m_inhibition.suppressAll(QStringLiteral("turned off manually; use 'awake on' or Resume to re-enable"));
}

void Application::awakeIndefinite(const QString& reason)
{
    if (!m_state.isModuleEnabled(Module::KeepAwake))
        return;
    InhibitionRequest r;
    r.source = RequestSource::Manual;
    r.reason = reason.isEmpty() ? QStringLiteral("Keep awake (indefinite)") : reason;
    r.priority = 10;
    r.expiration = ExpirationPolicy::None;
    r.capabilities = Capability::SystemSleep | Capability::SessionIdle;
    m_inhibition.addRequest(r);
}

void Application::awakeUntil(const QDateTime& localUntil, const QString& reason)
{
    if (!m_state.isModuleEnabled(Module::KeepAwake) || !localUntil.isValid())
        return;
    InhibitionRequest r;
    r.source = RequestSource::Manual;
    r.reason = reason.isEmpty() ? QStringLiteral("Keep awake (until %1)").arg(localUntil.toLocalTime().toString(QStringLiteral("HH:mm"))) : reason;
    r.priority = 10;
    r.expiration = ExpirationPolicy::UntilTime;
    r.until = localUntil.toUTC();
    r.capabilities = Capability::SystemSleep | Capability::SessionIdle;
    m_inhibition.addRequest(r);
}

void Application::releaseKeepAwake(const QUuid& requestId)
{
    m_inhibition.removeRequest(requestId);
}

void Application::resumeAutomaticRules()
{
    m_inhibition.resumeAutomatic();
}

void Application::pauseAll()
{
    m_state.setPausedAll(true);
    if (m_focus)
        m_focus->stop();
    m_inhibition.releaseAll();
    // Work Log pause is wired in WP-05.
}

void Application::unpauseAll()
{
    m_state.setPausedAll(false);
    // Returns to safe inactive state; interrupted manual sessions are not
    // restarted automatically (spec §7) — user starts them explicitly.
}

void Application::applyFocusKeepAwake(bool active, const QString& reason)
{
    static QUuid focusRequestId;
    if (active) {
        InhibitionRequest r;
        r.source = RequestSource::Focus;
        r.reason = reason;
        r.priority = 20;
        r.expiration = ExpirationPolicy::None;
        r.capabilities = Capability::SystemSleep | Capability::SessionIdle;
        focusRequestId = m_inhibition.addRequest(r);
    } else if (!focusRequestId.isNull()) {
        m_inhibition.removeRequest(focusRequestId);
        focusRequestId = QUuid();
    }
}

void Application::applyFocusSettingsFromSettings()
{
    // Seed the controller from persisted preferences (quiet hours included;
    // the UI page preserves quiet fields when it re-applies its widgets).
    FocusController::Options o;
    o.workMs = m_settings->focusWorkMinutes() * 60LL * 1000;
    o.shortBreakMs = m_settings->focusShortBreakMinutes() * 60LL * 1000;
    o.longBreakMs = m_settings->focusLongBreakMinutes() * 60LL * 1000;
    o.longBreakEvery = m_settings->focusLongBreakEvery();
    o.autoNext = m_settings->focusAutoNext();
    o.keepAwakeDuringWork = m_settings->focusKeepAwakeDuringWork();
    o.keepAwakeDuringBreaks = m_settings->focusKeepAwakeDuringBreaks();
    o.recordInWorkLog = m_settings->focusRecordInWorkLog();
    o.notifyOnInterval = m_settings->notificationsEnabled();
    o.soundOnNotify = m_settings->soundEnabled();
    o.quietHoursEnabled = m_settings->quietHoursEnabled();
    const QTime from = QTime::fromString(m_settings->quietFrom(), QStringLiteral("HH:mm"));
    const QTime to = QTime::fromString(m_settings->quietTo(), QStringLiteral("HH:mm"));
    o.quietFromMinutes = from.isValid() ? from.hour() * 60 + from.minute() : 0;
    o.quietToMinutes = to.isValid() ? to.hour() * 60 + to.minute() : 0;
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_SETTINGS"))
        std::fprintf(stderr, "[kaeru] focus options: quiet=%d from=%d to=%d\n",
                     o.quietHoursEnabled ? 1 : 0, o.quietFromMinutes, o.quietToMinutes);
    if (m_focus)
        m_focus->setOptions(o);
}

void Application::prepareQuit()
{
    // Quit stops recording (close segment + session — graceful shutdown, so
    // no recovery is needed on next start) and releases every owned inhibitor.
    if (m_work)
        m_work->stop(nullptr);
    if (m_focus)
        m_focus->stop();
    m_inhibition.releaseAll();
    m_state.setPausedAll(false);
    m_tickTimer.stop();
    m_settings->sync();
}

EffectiveAwakeState Application::awakeState() const
{
    return m_inhibition.effectiveState(QDateTime::currentDateTimeUtc());
}

QVariantMap Application::statusJson() const
{
    QVariantMap m;
    m[QStringLiteral("app")] = QStringLiteral("kaeru");
    m[QStringLiteral("version")] = QStringLiteral(KAERU_VERSION);
    m[QStringLiteral("modules")] = m_state.toJson();
    m[QStringLiteral("keepAwake")] = awakeState().toJson();

    QVariantMap focus;
    focus[QStringLiteral("phase")] = m_focus ? m_focus->phaseLabel() : QStringLiteral("idle");
    focus[QStringLiteral("remainingMs")] = m_focus ? m_focus->remainingMs() : qint64(-1);
    m[QStringLiteral("focus")] = focus;

    QVariantMap work;
    if (m_work && m_work->isActive()) {
        const SessionRecord s = m_work->session();
        work[QStringLiteral("active")] = true;
        work[QStringLiteral("startedAt")] = s.startedUtc.toString(Qt::ISODate);
        work[QStringLiteral("todayWorkMs")] = m_work->todayWorkMs();
    } else {
        work[QStringLiteral("active")] = false;
        work[QStringLiteral("todayWorkMs")] = m_work ? m_work->todayWorkMs() : qint64(0);
    }
    work[QStringLiteral("recoveryNotes")] = QVariant(m_recoveryNotes);
    m[QStringLiteral("workLog")] = work;

    m[QStringLiteral("background")] = m_background;
    return m;
}

QVariantMap Application::diagnosticsJson() const
{
    QVariantMap m;
    m[QStringLiteral("version")] = QStringLiteral(KAERU_VERSION);
    m[QStringLiteral("capabilities")] = m_capabilities.toJson();
    m[QStringLiteral("keepAwake")] = awakeState().toJson();

    QVariantList bindings;
    for (const InhibitionBinding& b : m_inhibition.bindings()) {
        bindings.append(QVariantMap {
            { QStringLiteral("id"), b.id },
            { QStringLiteral("backend"), b.backendId },
            { QStringLiteral("state"), bindingStateName(b.state) },
            { QStringLiteral("error"), b.errorDetail },
        });
    }
    m[QStringLiteral("bindings")] = bindings;
    return m;
}

} // namespace kaeru
