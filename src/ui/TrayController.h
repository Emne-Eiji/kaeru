// Kaeru — tray controller (spec §8.4, ADR-006, WP-08)
// QSystemTrayIcon (Qt's SNI path on Linux) + compact visible fallback window
// when no tray is available. The status item itself toggles Keep Awake; the
// compact duration menu shares the application's timer-selection logic.
#pragma once
#include "MainWindow.h"

#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QWidget>

class QAction;
class QLabel;
class QMenu;
class QPushButton;
class QTimer;

namespace kaeru {

class TrayController final : public QObject {
    Q_OBJECT
public:
    explicit TrayController(MainWindow* window, QObject* parent = nullptr);

    void ensureStarted();              // bounded retry/backoff for tray discovery
    bool trayAvailable() const;

    void updateIcon();                 // icon + tooltip reflect effective state
    void rebuildMenu();                // build once, then refresh from live state

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void retryTray();
    void refreshMenu();

private:
    void createFallbackWindow();
    void updateFallback();
    void showCustomDurationDialog();

    MainWindow* m_window = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_menu = nullptr;
    QAction* m_statusAction = nullptr;
    QTimer* m_refreshTimer = nullptr;
    bool m_available = false;
    bool m_dbusMenuHost = false; // GNOME/KDE render exported DBusMenu labels
    int m_retries = 0;
    QPointer<QWidget> m_fallback;
    QLabel* m_fallbackStatus = nullptr;
    QPushButton* m_fallbackToggle = nullptr;
};

} // namespace kaeru
