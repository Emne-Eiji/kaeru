#include "TrayController.h"
#include "Icons.h"
#include "Theme.h"
#include "ui/CustomDurationDialog.h"

#include "app/Application.h"
#include "core/AwakeDuration.h"
#include "core/TrayState.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace kaeru {

namespace {

QString statusLabel(const TrayMenuState& state, bool exportedDbusMenu)
{
    // DBusMenu (GNOME/KDE appindicator) exports QAction::text() verbatim and
    // cannot render rich text. Use plain, two-line text there and carry the
    // ON/OFF color in the status icon. Qt-hosted menus get rich text: normal
    // "Kaeru:", a green/red state word and a smaller gray detail line.
    if (exportedDbusMenu)
        return state.statusLine + QStringLiteral("\n") + state.detailLine;

    const bool dark = Theme::instance().isDark();
    const QString prefixColor = dark ? QStringLiteral("#e6e6e6") : QStringLiteral("#24292f");
    const QString detailColor = dark ? QStringLiteral("#a6a8ad") : QStringLiteral("#5f6672");
    const QString stateColor = state.active   ? QStringLiteral("#2e7d32")
                             : state.error    ? QStringLiteral("#c0392b")
                             : state.statusLine.contains(QStringLiteral("ACTIVATING"))
                                   ? QStringLiteral("#b7791f")
                                   : QStringLiteral("#c0392b");
    const QString word = state.statusLine.startsWith(QStringLiteral("Kaeru: "))
        ? state.statusLine.mid(7)
        : state.statusLine;
    return QStringLiteral("<span style='color:%1'>Kaeru: </span>"
                          "<span style='color:%2;font-weight:600;'>%3</span>"
                          "<br><span style='color:%4;font-size:9pt;'>%5</span>")
        .arg(prefixColor, stateColor, word.toHtmlEscaped(),
             detailColor, state.detailLine.toHtmlEscaped());
}

QIcon statusDot(const TrayMenuState& state)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QColor color = state.active ? QColor(0x2e, 0x7d, 0x32)
                       : state.error  ? QColor(0xc0, 0x39, 0x2b)
                       : state.statusLine.contains(QStringLiteral("ACTIVATING"))
                             ? QColor(0xb7, 0x79, 0x1f)
                             : QColor(0xc0, 0x39, 0x2b);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(color));
    painter.drawEllipse(3, 3, 10, 10);
    painter.end();
    return QIcon(pixmap);
}

} // namespace

TrayController::TrayController(MainWindow* window, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    // Both the icon/tooltip and the compact menu follow the coordinator's
    // authoritative state. stateChanged is emitted after activation, failure,
    // denial, manual release and expiry; AppState::changed covers Pause All.
    const auto refresh = [this] {
        refreshMenu();
        updateIcon();
    };
    connect(window->app()->inhibition(), &InhibitionCoordinator::stateChanged,
            this, refresh);
    connect(window->app()->appState(), &AppState::changed, this, refresh);
    connect(&Theme::instance(), &Theme::changed, this, [this](bool) { refreshMenu(); });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000); // remaining time in the status detail
    connect(m_refreshTimer, &QTimer::timeout, this, [this] {
        refreshMenu();
        updateFallback();
    });
    m_refreshTimer->start();
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
            QDBusInterface watcher(QStringLiteral("org.kde.StatusNotifierWatcher"),
                                   QStringLiteral("/StatusNotifierWatcher"),
                                   QStringLiteral("org.freedesktop.DBus.Properties"),
                                   QDBusConnection::sessionBus(), this);
            const QDBusReply<QVariant> host = watcher.call(
                QStringLiteral("Get"),
                QStringLiteral("org.kde.StatusNotifierWatcher"),
                QStringLiteral("IsStatusNotifierHostRegistered"));
            m_dbusMenuHost = host.isValid() && host.value().toBool();

            m_tray = new QSystemTrayIcon(Icons::stateIcon(IconState::Inactive), this);
            m_tray->setToolTip(tr("Kaeru"));
            rebuildMenu();
            // Left-click toggles, right-click is reserved for the native menu.
            connect(m_tray, &QSystemTrayIcon::activated,
                    this, &TrayController::onTrayActivated);
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
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // Exactly one toggle per single left-click. DoubleClick is ignored
        // below to prevent a second toggle from the follow-up event.
        m_window->app()->toggleKeepAwake();
        break;
    case QSystemTrayIcon::Context:
        // The native tray host opens the exported context menu on right-click.
        // Calling popup() here would open a second menu on X11.
        break;
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::MiddleClick:
    default:
        break;
    }
}

void TrayController::rebuildMenu()
{
    if (!m_tray)
        return; // retryTray() calls this again once a tray is available

    if (!m_menu) {
        Application* app = m_window->app();
        m_menu = new QMenu(m_window);

        m_statusAction = m_menu->addAction(QString());
        m_statusAction->setCheckable(false);
        m_statusAction->setEnabled(true);
        m_statusAction->setToolTip(tr("Toggle Keep Awake using the last selected duration"));
        connect(m_statusAction, &QAction::triggered, app, [app] {
            app->toggleKeepAwake();
        });
        m_menu->addSeparator();

        auto* awakeMenu = m_menu->addMenu(tr("Kaeru"));
        const auto addDuration = [this, app, awakeMenu](const QString& label,
                                                        AwakeDuration::Kind kind) {
            awakeMenu->addAction(label, app, [app, kind] {
                app->selectAwakeDuration(AwakeDuration::standard(kind));
            });
        };
        addDuration(tr("15 minutes"), AwakeDuration::Kind::Minutes15);
        addDuration(tr("30 minutes"), AwakeDuration::Kind::Minutes30);
        addDuration(tr("1 hour"), AwakeDuration::Kind::Hour1);
        addDuration(tr("2 hours"), AwakeDuration::Kind::Hours2);
        addDuration(tr("Indefinitely"), AwakeDuration::Kind::Indefinite);
        awakeMenu->addSeparator();
        awakeMenu->addAction(tr("Custom…"), this, [this] {
            // Defer so the platform menu closes before the modal dialog.
            QTimer::singleShot(0, this, [this] { showCustomDurationDialog(); });
        });

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
    if (!m_statusAction)
        return;

    Application* app = m_window->app();
    const EffectiveAwakeState state = app->awakeState();
    const TrayMenuState menuState = decideTrayMenuState(state.status, state.remainingMs);
    m_statusAction->setText(statusLabel(menuState, m_dbusMenuHost));
    m_statusAction->setIcon(statusDot(menuState));

    QString tooltip = menuState.statusLine + QStringLiteral(" · ") + menuState.detailLine;
    if (!state.errorDetail.isEmpty())
        tooltip += QStringLiteral("\n") + state.errorDetail;
    else if (!state.limitation.isEmpty())
        tooltip += QStringLiteral("\n") + state.limitation;
    m_statusAction->setToolTip(tooltip);
    m_statusAction->setStatusTip(tooltip);
    m_statusAction->setIconText(menuState.statusLine + QStringLiteral(", ")
                                + menuState.detailLine);
}

void TrayController::showCustomDurationDialog()
{
    const AwakeDuration last = m_window->app()->lastAwakeDuration();
    CustomDurationDialog dialog(m_window,
        last.isCustom() ? last.customMs : 90LL * 60 * 1000);
    if (dialog.exec() == QDialog::Accepted)
        m_window->app()->selectAwakeDuration(dialog.selection());
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
    m_fallbackToggle = new QPushButton(tr("Start Keep Awake"), w);
    auto* open = new QPushButton(tr("Open Kaeru"), w);
    auto* quit = new QPushButton(tr("Quit Kaeru"), w);

    layout->addWidget(note);
    layout->addWidget(m_fallbackStatus);
    layout->addWidget(m_fallbackToggle);
    layout->addWidget(open);
    layout->addWidget(quit);

    Application* app = m_window->app();
    connect(m_fallbackToggle, &QPushButton::clicked, app, [app] {
        app->toggleKeepAwake();
    });
    connect(open, &QPushButton::clicked, m_window, [this] { m_window->showWindow(); });
    connect(quit, &QPushButton::clicked, app, [app] {
        app->prepareQuit();
        qApp->quit();
    });

    m_fallback = w;
}

void TrayController::updateFallback()
{
    if (m_fallback.isNull())
        return;
    const EffectiveAwakeState state = m_window->app()->awakeState();
    const TrayMenuState menuState = decideTrayMenuState(state.status, state.remainingMs);
    if (m_fallbackStatus)
        m_fallbackStatus->setText(menuState.statusLine + QStringLiteral(" · ")
                                  + menuState.detailLine);
    if (m_fallbackToggle)
        m_fallbackToggle->setText(menuState.active ? tr("Turn Keep Awake off")
                                                    : tr("Start Keep Awake"));
}

} // namespace kaeru
