#include "MainWindow.h"
#include "Theme.h"
#include "Icons.h"
#include "TrayController.h"

#include "pages/OverviewPage.h"
#include "pages/FocusPage.h"
#include "pages/ReportsPage.h"
#include "pages/SettingsPage.h"
#include "pages/DiagnosticsPage.h"
#include "pages/WorkLogPage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
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
    resize(860, 620);
    setMinimumSize(720, 520);

    buildUi();
    applyTheme();

    m_tray = new TrayController(this);
    m_tray->ensureStarted();

    connect(&Theme::instance(), &Theme::changed, this, [this](bool) { applyTheme(); });

    // Keyboard shortcuts (accessibility: full page reachability)
    auto* helpShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+?")), this);
    connect(helpShortcut, &QShortcut::activated, this, [] {});
    for (int i = 1; i <= 6; ++i) {
        auto* sc = new QShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i)), this);
        connect(sc, &QShortcut::activated, this, [this, i] { m_nav->setCurrentRow(i - 1); });
    }
    statusBar()->showMessage(tr("Ready"), 4000);
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
    addNavItem(tr("Overview"));
    addNavItem(tr("Focus"));
    addNavItem(tr("Work Log"));
    addNavItem(tr("Reports"));
    addNavItem(tr("Settings"));
    addNavItem(tr("Diagnostics"));
    m_nav->setCurrentRow(0);

    m_pages = new QStackedWidget(central);

    m_overview = new OverviewPage(m_app, this);
    m_focus = new FocusPage(m_app, this);
    m_worklog = new WorkLogPage(m_app, this);
    m_reports = new ReportsPage(m_app, this);
    m_settings = new SettingsPage(m_app, this);
    m_diagnostics = new DiagnosticsPage(m_app, this);

    m_pages->addWidget(m_overview);
    m_pages->addWidget(m_focus);
    m_pages->addWidget(m_worklog);
    m_pages->addWidget(m_reports);
    m_pages->addWidget(m_settings);
    m_pages->addWidget(m_diagnostics);

    connect(m_nav, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        switch (row) {
        case 3: m_reports->refresh(); break;
        case 4: m_settings->refresh(); break;
        case 5: m_diagnostics->refresh(); break;
        default: break;
        }
    });

    layout->addWidget(m_nav);
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);
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

void MainWindow::startHidden()
{
    // Remain invisible; the tray (or fallback controls) owns visibility.
    hide();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
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
