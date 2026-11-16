#include "TrayController.h"
#include "Icons.h"

#include "app/Application.h"
#include "app/CommandRouter.h"
#include "app/WorkSessionManager.h"
#include "core/Duration.h"
#include "core/TrayState.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace kaeru {

TrayController::TrayController(MainWindow* window, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    // Reflect effective state in icon/tooltip whenever inhibition state changes.
    connect(window->app()->inhibition(), &InhibitionCoordinator::stateChanged,
        this, &TrayController::updateIcon);
    connect(window->app()->appState(), &AppState::changed, this, [this] { rebuildMenu(); updateIcon(); });
    if (window->app()->work())
        connect(window->app()->work(), &WorkSessionManager::stateChanged,
            this, &TrayController::rebuildMenu);
}

void TrayController::ensureStarted()
{
    // Bounded retry/backoff (spec §7): 5 attempts, 1 s apart, then fallback.
    retryTray();
}

void TrayController::retryTray()
{
    if (m_available)
        return;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        if (!m_tray) {
            m_tray = new QSystemTrayIcon(Icons::stateIcon(IconState::Inactive), this);
            m_tray->setToolTip(tr("Kaeru"));
            rebuildMenu();
            connect(m_tray, &QSystemTrayIcon::activated, this, &TrayController::onTrayActivated);
        }
        m_tray->show();
        m_available = true;
        updateIcon();
        if (m_fallback)
            m_fallback->hide(); // tray usable again
        return;
    }
    if (++m_retries >= 5) {
        if (!m_fallback)
            createFallbackWindow();
        m_fallback->show();
        updateFallback();
        return;
    }
    QTimer::singleShot(1000, this, &TrayController::retryTray);
}

bool TrayController::trayAvailable() const
{
    return m_available;
}

void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        m_window->showWindow();
}

void TrayController::rebuildMenu()
{
    if (!m_tray)
        return;
    auto* menu = new QMenu();
    Application* app = m_window->app();
    CommandRouter router(app);

    const EffectiveAwakeState state = app->awakeState();
    QString stateLine = tr("Keep Awake: %1").arg(effectiveStatusName(state.status));
    if (!state.reason.isEmpty())
        stateLine += QStringLiteral("\n") + tr("Reason: %1").arg(state.reason);
    if (state.remainingMs >= 0)
        stateLine += QStringLiteral("\n") + tr("Remaining: %1").arg(formatRemainingMs(state.remainingMs));

    auto* statusAction = menu->addAction(stateLine);
    statusAction->setEnabled(false);
    menu->addSeparator();

    auto* awakeMenu = menu->addMenu(tr("Keep Awake"));
    awakeMenu->addAction(tr("15 min"), app, [app] { app->resumeAutomaticRules(); app->awakeOn(15 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("30 min"), app, [app] { app->resumeAutomaticRules(); app->awakeOn(30 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("1 hour"), app, [app] { app->resumeAutomaticRules(); app->awakeOn(60 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("2 hours"), app, [app] { app->resumeAutomaticRules(); app->awakeOn(120 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("Indefinitely"), app, [app] { app->resumeAutomaticRules(); app->awakeIndefinite(QString()); });
    awakeMenu->addSeparator();
    awakeMenu->addAction(tr("Turn off Keep Awake now"), app, [app] { CommandRouter(app).awakeOff(); });
    awakeMenu->addAction(tr("Resume automatic rules"), app, [app] { CommandRouter(app).resumeAutomatic(); });

    auto* focusMenu = menu->addMenu(tr("Focus"));
    focusMenu->addAction(tr("Start (Pomodoro)"), app, [app] { CommandRouter(app).focusCommand(QStringLiteral("start"), {}); });
    focusMenu->addAction(tr("Pause"), app, [app] { CommandRouter(app).focusCommand(QStringLiteral("pause"), {}); });
    focusMenu->addAction(tr("Resume"), app, [app] { CommandRouter(app).focusCommand(QStringLiteral("resume"), {}); });
    focusMenu->addAction(tr("Skip interval"), app, [app] { CommandRouter(app).focusCommand(QStringLiteral("skip"), {}); });
    focusMenu->addAction(tr("Stop"), app, [app] { CommandRouter(app).focusCommand(QStringLiteral("stop"), {}); });

    auto* workMenu = menu->addMenu(tr("Work Log"));
    if (app->storageReady() && app->work()) {
        const qint64 todayMs = app->work()->todayWorkMs();
        auto* todayAction = workMenu->addAction(
            tr("Today: %1").arg(formatRemainingMs(todayMs)));
        todayAction->setEnabled(false);
        workMenu->addSeparator();
    }
    workMenu->addAction(tr("Start"), app, [app] { CommandRouter(app).workCommand(QStringLiteral("start"), {}); });
    workMenu->addAction(tr("Pause"), app, [app] { CommandRouter(app).workCommand(QStringLiteral("pause"), {}); });
    workMenu->addAction(tr("Resume"), app, [app] { CommandRouter(app).workCommand(QStringLiteral("resume"), {}); });
    workMenu->addAction(tr("Stop"), app, [app] { CommandRouter(app).workCommand(QStringLiteral("stop"), {}); });

    menu->addSeparator();
    menu->addAction(tr("Pause All"), app, [app] { CommandRouter(app).pauseAll(); });
    menu->addSeparator();
    menu->addAction(tr("Open Kaeru"), m_window, [this] { m_window->showWindow(); });
    menu->addAction(tr("Settings"), m_window, [this] { m_window->showWindow(); });
    menu->addAction(tr("Quit"), m_window, [this] {
        m_window->app()->prepareQuit();
        qApp->quit();
    });

    m_tray->setContextMenu(menu);
    m_tray->setToolTip(tr("Kaeru — %1").arg(effectiveStatusName(state.status)));
}

void TrayController::updateIcon()
{
    if (!m_tray || !m_available)
        return;
    Application* app = m_window->app();
    const EffectiveAwakeState state = app->awakeState();
    const FocusController* focus = app->focus();
    const bool focusActive = focus
        && (focus->phase() == FocusController::Phase::Work
            || focus->phase() == FocusController::Phase::ShortBreak
            || focus->phase() == FocusController::Phase::LongBreak);

    // Official frog identity: Awake (protection held), Focused (focus running),
    // Sleep (Kaeru inactive). Precedence + honest tooltip per ADR-008.
    const TrayPresentation tp = decideTrayPresentation(
        state.status, focusActive, app->appState()->isPausedAll(),
        state.errorDetail, state.limitation);

    IconState icon = IconState::Inactive;
    switch (tp.icon) {
    case TrayPresentation::Icon::Awake: icon = IconState::Awake; break;
    case TrayPresentation::Icon::Focused: icon = IconState::Focus; break;
    case TrayPresentation::Icon::Sleep:
    default: icon = IconState::Inactive; break;
    }

    QString tooltip = tr("Kaeru — %1").arg(tp.summary);
    if (!state.reason.isEmpty() && (tp.keepAwakeActive || tp.keepAwakePartial))
        tooltip += QStringLiteral(" · %1").arg(state.reason);
    if (tp.keepAwakePartial && !tp.detail.isEmpty())
        tooltip += QStringLiteral(" · %1").arg(tp.detail);
    if (tp.focusActive) {
        tooltip += QStringLiteral(" · %1").arg(focus->phaseLabel());
        if (focus->remainingMs() >= 0)
            tooltip += QStringLiteral(" (%1 left)")
                .arg(formatRemainingMs(focus->remainingMs()));
    }
    if (app->storageReady() && app->work() && app->work()->isActive())
        tooltip += tr("\nRecording work · today %1")
            .arg(formatRemainingMs(app->work()->todayWorkMs()));
    if (tp.error && !tp.detail.isEmpty())
        tooltip += QStringLiteral("\n%1").arg(tp.detail);

    m_tray->setIcon(Icons::stateIcon(icon));
    m_tray->setToolTip(tooltip);
    updateFallback();
}

void TrayController::createFallbackWindow()
{
    auto* w = new QWidget(nullptr, Qt::Window | Qt::WindowStaysOnTopHint);
    w->setWindowTitle(tr("Kaeru controls"));
    w->resize(300, 140);

    auto* layout = new QVBoxLayout(w);
    auto* note = new QLabel(tr("The system tray is unavailable. Use these controls or the main window."), w);
    note->setWordWrap(true);
    m_fallbackStatus = new QLabel(w);
    m_fallbackToggle = new QPushButton(tr("Start Keep Awake"), w);
    auto* quit = new QPushButton(tr("Quit Kaeru"), w);

    layout->addWidget(note);
    layout->addWidget(m_fallbackStatus);
    layout->addWidget(m_fallbackToggle);
    layout->addWidget(quit);

    Application* app = m_window->app();
    connect(m_fallbackToggle, &QPushButton::clicked, app, [app] {
        CommandRouter router(app);
        const EffectiveAwakeState st = app->awakeState();
        if (st.status == EffectiveStatus::Active)
            router.awakeOff();
        else
            router.awakeOn(QVariantMap {
                { QStringLiteral("durationMs"), 60 * 60 * 1000 },
                { QStringLiteral("mode"), QStringLiteral("both") } });
    });
    connect(quit, &QPushButton::clicked, app, [app] {
        app->prepareQuit();
        qApp->quit();
    });

    // Keep fallback state fresh
    auto* refresh = new QTimer(w);
    connect(refresh, &QTimer::timeout, w, [this] { updateFallback(); });
    refresh->start(1000);

    m_fallback = w;
}

void TrayController::updateFallback()
{
    if (m_fallback.isNull())
        return;
    const EffectiveAwakeState state = m_window->app()->awakeState();
    if (m_fallbackStatus)
        m_fallbackStatus->setText(tr("Keep Awake: %1").arg(effectiveStatusName(state.status)));
    if (m_fallbackToggle)
        m_fallbackToggle->setText(state.status == EffectiveStatus::Active ? tr("Turn off now") : tr("Keep awake for 1 hour"));
}

} // namespace kaeru
