#include "TrayController.h"
#include "Icons.h"

#include "app/Application.h"
#include "core/Duration.h"
#include "core/TrayState.h"

#include <QAction>
#include <QApplication>
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
    // Both the icon and the expanded menu follow the coordinator's
    // authoritative state. stateChanged is emitted after activation,
    // failure/denial, manual release and expiry; AppState::changed covers
    // Pause All and module switches. Refreshing only the icon here was the
    // cause of the stale "inactive" menu line and disabled off action.
    const auto refresh = [this] {
        rebuildMenu();
        updateIcon();
    };
    connect(window->app()->inhibition(), &InhibitionCoordinator::stateChanged,
        this, refresh);
    connect(window->app()->appState(), &AppState::changed, this, refresh);
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
        return; // retryTray() calls this again once a tray is available

    if (!m_menu) {
        Application* app = m_window->app();
        m_menu = new QMenu(m_window);

        m_statusAction = m_menu->addAction(QString());
        // A status line, not a command: keep it enabled so tray hosts do not
        // paint it as permanently unavailable/grey. Plain text carries the
        // state; color is never the only signal.
        m_statusAction->setEnabled(true);
        m_statusAction->setCheckable(false);
        m_menu->addSeparator();

        auto* awakeMenu = m_menu->addMenu(tr("Kaeru"));
        awakeMenu->addAction(tr("15 minutes"), app, [app] { app->awakeOn(15 * 60 * 1000, QStringLiteral("both")); });
        awakeMenu->addAction(tr("30 minutes"), app, [app] { app->awakeOn(30 * 60 * 1000, QStringLiteral("both")); });
        awakeMenu->addAction(tr("1 hour"), app, [app] { app->awakeOn(60 * 60 * 1000, QStringLiteral("both")); });
        awakeMenu->addAction(tr("2 hours"), app, [app] { app->awakeOn(120 * 60 * 1000, QStringLiteral("both")); });
        awakeMenu->addAction(tr("Indefinitely"), app, [app] { app->awakeIndefinite(QString()); });
        awakeMenu->addSeparator();
        m_offAction = awakeMenu->addAction(tr("Turn off Kaeru"), app, [app] { app->awakeOff(); });

        m_menu->addSeparator();
        m_menu->addAction(tr("Open Kaeru"), m_window, [this] { m_window->showWindow(); });
        m_menu->addAction(tr("Settings"), m_window, [this] { m_window->showSettings(); });
        m_menu->addSeparator();
        m_menu->addAction(tr("Quit Kaeru"), m_window, [this] {
            m_window->app()->prepareQuit();
            qApp->quit();
        });

        m_tray->setContextMenu(m_menu);
    }

    refreshMenu();
}

void TrayController::refreshMenu()
{
    if (!m_menu || !m_statusAction || !m_offAction)
        return;

    const EffectiveAwakeState state = m_window->app()->awakeState();
    const TrayMenuState menuState = decideTrayMenuState(state.status);
    m_statusAction->setText(menuState.statusLine);
    // Never let a host re-enable a stale off action: only confirmed active
    // inhibition may be turned off. Pending/error states stay disabled.
    m_offAction->setEnabled(menuState.offActionEnabled);

    QString detail = menuState.statusLine;
    if (state.remainingMs >= 0)
        detail += QStringLiteral(" · ") + formatRemainingMs(state.remainingMs);
    else if (menuState.offActionEnabled)
        detail += QStringLiteral(" · ") + tr("until turned off");
    if (!state.errorDetail.isEmpty())
        detail += QStringLiteral("\n") + state.errorDetail;
    else if (!state.limitation.isEmpty())
        detail += QStringLiteral("\n") + state.limitation;
    m_statusAction->setToolTip(detail);
    m_statusAction->setStatusTip(detail);
}

void TrayController::updateIcon()
{
    if (!m_tray || !m_available)
        return;
    Application* app = m_window->app();
    const EffectiveAwakeState state = app->awakeState();

    // Official frog identity: Awake (protection held), Sleep (Kaeru inactive).
    // Focus is deferred in this release, so no focused expression is shown.
    const TrayPresentation tp = decideTrayPresentation(
        state.status, false, app->appState()->isPausedAll(),
        state.errorDetail, state.limitation);

    IconState icon = IconState::Inactive;
    switch (tp.icon) {
    case TrayPresentation::Icon::Awake: icon = IconState::Awake; break;
    case TrayPresentation::Icon::Focused: icon = IconState::Focus; break;
    case TrayPresentation::Icon::Sleep:
    default: icon = IconState::Inactive; break;
    }

    QString tooltip = tr("Kaeru — %1").arg(tp.summary);
    if (state.remainingMs >= 0)
        tooltip += QStringLiteral(" · %1").arg(formatRemainingMs(state.remainingMs));
    else if (tp.keepAwakeActive || tp.keepAwakePartial)
        tooltip += QStringLiteral(" · ") + tr("until turned off");
    if (tp.keepAwakePartial && !tp.detail.isEmpty())
        tooltip += QStringLiteral(" · %1").arg(tp.detail);
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
    w->resize(320, 170);

    auto* layout = new QVBoxLayout(w);
    auto* note = new QLabel(tr("The system tray is unavailable. Use these controls or the main window."), w);
    note->setWordWrap(true);
    m_fallbackStatus = new QLabel(w);
    m_fallbackToggle = new QPushButton(tr("Keep awake for 1 hour"), w);
    auto* open = new QPushButton(tr("Open Kaeru"), w);
    auto* quit = new QPushButton(tr("Quit Kaeru"), w);

    layout->addWidget(note);
    layout->addWidget(m_fallbackStatus);
    layout->addWidget(m_fallbackToggle);
    layout->addWidget(open);
    layout->addWidget(quit);

    Application* app = m_window->app();
    connect(m_fallbackToggle, &QPushButton::clicked, app, [app] {
        const EffectiveAwakeState st = app->awakeState();
        if (st.status == EffectiveStatus::Active || st.status == EffectiveStatus::PartiallyActive)
            app->awakeOff();
        else
            app->awakeOn(60 * 60 * 1000, QStringLiteral("both"));
    });
    connect(open, &QPushButton::clicked, m_window, [this] { m_window->showWindow(); });
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
    const TrayMenuState menuState = decideTrayMenuState(state.status);
    if (m_fallbackStatus) {
        QString text = menuState.statusLine;
        if (state.remainingMs >= 0)
            text += QStringLiteral(" · ") + formatRemainingMs(state.remainingMs);
        m_fallbackStatus->setText(text);
    }
    if (m_fallbackToggle) {
        m_fallbackToggle->setText(menuState.offActionEnabled
                ? tr("Turn off Kaeru")
                : tr("Keep awake for 1 hour"));
    }
}

} // namespace kaeru
