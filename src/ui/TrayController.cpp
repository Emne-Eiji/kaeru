#include "TrayController.h"
#include "Icons.h"

#include "app/Application.h"
#include "core/Duration.h"
#include "core/TrayState.h"

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
    // Reflect effective state in icon/tooltip whenever inhibition state changes.
    connect(window->app()->inhibition(), &InhibitionCoordinator::stateChanged,
        this, &TrayController::updateIcon);
    connect(window->app()->appState(), &AppState::changed, this, [this] {
        rebuildMenu();
        updateIcon();
    });
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

    const EffectiveAwakeState state = app->awakeState();
    QString stateLine = tr("Keep Awake: %1").arg(effectiveStatusName(state.status));
    if (state.remainingMs >= 0)
        stateLine += QStringLiteral(" · ") + formatRemainingMs(state.remainingMs);
    else if (state.status == EffectiveStatus::Active
        || state.status == EffectiveStatus::PartiallyActive)
        stateLine += QStringLiteral(" · ") + tr("until turned off");

    auto* statusAction = menu->addAction(stateLine);
    statusAction->setEnabled(false);
    menu->addSeparator();

    auto* awakeMenu = menu->addMenu(tr("Keep Awake"));
    awakeMenu->addAction(tr("15 minutes"), app, [app] { app->awakeOn(15 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("30 minutes"), app, [app] { app->awakeOn(30 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("1 hour"), app, [app] { app->awakeOn(60 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("2 hours"), app, [app] { app->awakeOn(120 * 60 * 1000, QStringLiteral("both")); });
    awakeMenu->addAction(tr("Indefinitely"), app, [app] { app->awakeIndefinite(QString()); });
    awakeMenu->addSeparator();
    auto* offAction = awakeMenu->addAction(tr("Turn off Keep Awake now"), app, [app] { app->awakeOff(); });
    offAction->setEnabled(state.status != EffectiveStatus::Inactive);

    menu->addSeparator();
    menu->addAction(tr("Open Kaeru"), m_window, [this] { m_window->showWindow(); });
    menu->addAction(tr("Settings"), m_window, [this] { m_window->showSettings(); });
    menu->addSeparator();
    menu->addAction(tr("Quit Kaeru"), m_window, [this] {
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
    if (m_fallbackStatus) {
        QString text = tr("Keep Awake: %1").arg(effectiveStatusName(state.status));
        if (state.remainingMs >= 0)
            text += QStringLiteral(" · ") + formatRemainingMs(state.remainingMs);
        m_fallbackStatus->setText(text);
    }
    if (m_fallbackToggle) {
        const bool active = state.status == EffectiveStatus::Active
            || state.status == EffectiveStatus::PartiallyActive;
        m_fallbackToggle->setText(active ? tr("Turn off Keep Awake now")
                                         : tr("Keep awake for 1 hour"));
    }
}

} // namespace kaeru
