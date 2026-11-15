// Kaeru — main window shell: sidebar navigation + six pages (spec §8.2)
#pragma once
#include "app/Application.h"

#include <QMainWindow>
#include <QSystemTrayIcon>

class QListWidget;
class QStackedWidget;
class QCloseEvent;

namespace kaeru {

class OverviewPage;
class FocusPage;
class WorkLogPage;
class ReportsPage;
class SettingsPage;
class DiagnosticsPage;
class TrayController;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Application* app, QWidget* parent = nullptr);

    void showWindow();
    void startHidden();

    Application* app() const { return m_app; }
    TrayController* tray() const { return m_tray; }

signals:
    void requestQuit();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void applyTheme();

    Application* m_app = nullptr;
    QListWidget* m_nav = nullptr;
    QStackedWidget* m_pages = nullptr;
    TrayController* m_tray = nullptr;

    OverviewPage* m_overview = nullptr;
    FocusPage* m_focus = nullptr;
    WorkLogPage* m_worklog = nullptr;
    ReportsPage* m_reports = nullptr;
    SettingsPage* m_settings = nullptr;
    DiagnosticsPage* m_diagnostics = nullptr;
};

} // namespace kaeru
