#include "MainWindow.h"
#include "Theme.h"
#include "Icons.h"
#include "TrayController.h"
#include "WindowGeometry.h"

#include "pages/AboutPage.h"
#include "pages/OverviewPage.h"
#include "pages/SettingsPage.h"
#include "pages/DiagnosticsPage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QScreen>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

namespace kaeru {

MainWindow::MainWindow(Application* app, QWidget* parent)
    : QMainWindow(parent)
    , m_app(app)
{
    setObjectName(QStringLiteral("KaeruMainWindow"));
    setWindowTitle(tr("Kaeru"));
    setWindowIcon(Icons::appIcon());
    setMinimumSize(900, 650);
    resize(1100, 760);

    buildUi();
    restoreWindowGeometry();
    applyTheme();

    m_tray = new TrayController(this);
    m_tray->ensureStarted();

    // Theme changes must take effect immediately, without a restart:
    //  • Settings → Appearance writes the preference.
    //  • The desktop portal reports a System scheme change.
    connect(m_app->settings(), &Settings::changed, this, [this] { applyTheme(); });
    connect(m_app->themeWatcher(), &ThemeWatcher::schemeChanged,
            this, [this](ColorScheme) { applyTheme(); });
    connect(&Theme::instance(), &Theme::changed, this, [this](bool) { applyTheme(); });
    statusBar()->showMessage(tr("Ready"), 4000);
}

MainWindow::~MainWindow()
{
    saveWindowGeometry();
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_nav = new QListWidget(central);
    m_nav->setObjectName(QStringLiteral("Nav"));
    m_nav->setFixedWidth(190);
    m_nav->setViewMode(QListView::ListMode);
    m_nav->setMovement(QListView::Static);
    m_nav->setUniformItemSizes(true);
    m_nav->setAccessibleName(tr("Main navigation"));

    auto addNavItem = [&](const QString& label) {
        auto* item = new QListWidgetItem(label, m_nav);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return item;
    };
    addNavItem(tr("Keep Awake"));
    addNavItem(tr("Settings"));
    addNavItem(tr("Diagnostics"));
    addNavItem(tr("About"));
    m_nav->setCurrentRow(0);

    m_pages = new QStackedWidget(central);

    m_overview = new OverviewPage(m_app, this);
    m_settings = new SettingsPage(m_app, this);
    m_diagnostics = new DiagnosticsPage(m_app, this);
    m_about = new AboutPage(m_app, this);

    m_pages->addWidget(m_overview);
    m_pages->addWidget(m_settings);
    m_pages->addWidget(m_diagnostics);
    m_pages->addWidget(m_about);

    connect(m_nav, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        switch (row) {
        case 1: m_settings->refresh(); break;
        case 2: m_diagnostics->refresh(); break;
        default: break;
        }
    });

    layout->addWidget(m_nav);
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);

    // Keyboard shortcuts (accessibility: full page reachability). The File
    // menu was removed; Settings remains reachable with Ctrl+, and Quit is
    // intentionally owned by the tray only.
    for (int i = 1; i <= 4; ++i) {
        auto* sc = new QShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i)), this);
        connect(sc, &QShortcut::activated, this, [this, i] { m_nav->setCurrentRow(i - 1); });
    }
    auto* settingsShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+,")), this);
    connect(settingsShortcut, &QShortcut::activated, this, &MainWindow::showSettings);
}

void MainWindow::restoreWindowGeometry()
{
    QRect saved;
    const bool hasSaved = m_app->settings()->windowGeometry(&saved);

    QScreen* screen = nullptr;
    if (hasSaved)
        screen = QGuiApplication::screenAt(saved.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry()
                                   : QRect(0, 0, 1280, 800);
    setGeometry(fitWindowToAvailable(saved, available, minimumSize(),
                                     QSize(1100, 760)));
}

void MainWindow::saveWindowGeometry() const
{
    QRect geometry = isMaximized() ? normalGeometry() : this->geometry();
    if (geometry.isValid() && geometry.width() > 0 && geometry.height() > 0)
        m_app->settings()->setWindowGeometry(geometry);
}

void MainWindow::applyTheme()
{
    Theme::instance().apply(m_app->settings()->theme(), m_app->themeWatcher()->scheme());
}

void MainWindow::showWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::showSettings()
{
    m_nav->setCurrentRow(1);
    showWindow();
}

void MainWindow::startHidden()
{
    // Remain invisible; the tray (or fallback controls) owns visibility.
    hide();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowGeometry();
    // Closing hides only when a usable tray control exists; otherwise keep a
    // visible control path (spec §7).
    if (m_tray && m_tray->trayAvailable()) {
        hide();
        event->ignore();
        return;
    }
    event->ignore(); // stay visible (fallback window remains usable)
}

} // namespace kaeru
