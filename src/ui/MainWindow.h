// Kaeru — main window shell: sidebar navigation + initial-release pages
#pragma once
#include "app/Application.h"

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QCloseEvent;

namespace kaeru {

class OverviewPage;
class SettingsPage;
class DiagnosticsPage;
class TrayController;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Application* app, QWidget* parent = nullptr);

    void showWindow();
    void showSettings();
    void startHidden();

    Application* app() const { return m_app; }
    TrayController* tray() const { return m_tray; }

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
    SettingsPage* m_settings = nullptr;
    DiagnosticsPage* m_diagnostics = nullptr;
};

} // namespace kaeru
