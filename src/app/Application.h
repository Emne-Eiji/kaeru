// Kaeru — application core wiring (spec §6/§7)
#pragma once
#include "Settings.h"
#include "core/AppState.h"
#include "core/CapabilityRegistry.h"
#include "core/FocusController.h"
#include "core/InhibitionCoordinator.h"
#include "core/TimerEngine.h"
#include "core/Types.h"

#include <QObject>
#include <QTimer>

namespace kaeru {

class LogindPowerMonitor;
class LockMonitor;
class Notifications;
class AutostartBackend;
class ThemeWatcher;
class PortalInhibitBackend;
class LogindInhibitBackend;

class Application final : public QObject {
    Q_OBJECT
public:
    explicit Application(bool backgroundMode, QObject* parent = nullptr);
    ~Application() override;

    // Access for UI / CLI / control service
    AppState* appState() { return &m_state; }
    Settings* settings() const { return m_settings.get(); }
    CapabilityRegistry* capabilities() { return &m_capabilities; }
    InhibitionCoordinator* inhibition() { return &m_inhibition; }
    FocusController* focus() const { return m_focus; }
    TimerEngine* timers() { return &m_timers; }
    LogindPowerMonitor* powerMonitor() const { return m_powerMonitor.get(); }
    LockMonitor* lockMonitor() const { return m_lockMonitor.get(); }
    Notifications* notifications() const { return m_notifications.get(); }
    AutostartBackend* autostart() const { return m_autostart.get(); }
    ThemeWatcher* themeWatcher() const { return m_themeWatcher.get(); }
    bool backgroundMode() const { return m_background; }

    // Keep Awake command surface (shared by UI/CLI/tray via CommandRouter)
    void awakeOn(qint64 durationMs, const QString& mode);   // mode: system|display|both
    void awakeOff();                                        // suppression semantics
    void awakeIndefinite(const QString& reason);
    void awakeUntil(const QDateTime& localUntil, const QString& reason);
    void releaseKeepAwake(const QUuid& requestId);
    void resumeAutomaticRules();

    void pauseAll();   // spec §7
    void unpauseAll();

    void prepareQuit(); // release inhibitors, close recording; called before quit

    // Re-seed the focus controller from persisted preferences (called after
    // settings changes so quiet hours / notifications apply immediately).
    void applyFocusSettingsFromSettings();

    // Focus ↔ keep-awake wiring per settings
    void applyFocusKeepAwake(bool active, const QString& reason);

    EffectiveAwakeState awakeState() const;

    // Diagnostics snapshot (safe fields only — no private content)
    QVariantMap diagnosticsJson() const;
    QVariantMap statusJson() const;

signals:
    void keepAwakeExpired(const QString& reason);
    void focusIntervalFinished(kaeru::FocusController::Phase phase);
    void showWindowRequested();
    void quitRequested();

private slots:
    void onTick();
    void onTimerCompleted(const QString& id, const QString& label);
    void onPrepareForSleep(bool start);
    void onLockChanged(bool locked);

private:
    void setupBackends();
    void startTickTimer();

    bool m_background = false;
    AppState m_state;
    CapabilityRegistry m_capabilities;
    std::unique_ptr<Settings> m_settings;
    TimerEngine m_timers;
    InhibitionCoordinator m_inhibition;
    FocusController* m_focus = nullptr;

    std::unique_ptr<PortalInhibitBackend> m_portalBackend;
    std::unique_ptr<LogindInhibitBackend> m_logindBackend;
    std::unique_ptr<LogindPowerMonitor> m_powerMonitor;
    std::unique_ptr<LockMonitor> m_lockMonitor;
    std::unique_ptr<Notifications> m_notifications;
    std::unique_ptr<AutostartBackend> m_autostart;
    std::unique_ptr<ThemeWatcher> m_themeWatcher;

    QTimer m_tickTimer;
};

} // namespace kaeru
