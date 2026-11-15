// Kaeru — tray controller (spec §8.4, ADR-006)
// QSystemTrayIcon (Qt's SNI path on Linux) + compact visible fallback window
// when no tray is available. Menu built from CommandRouter semantics.
#pragma once
#include "MainWindow.h"

#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QWidget>

class QLabel;
class QPushButton;

namespace kaeru {

class TrayController final : public QObject {
    Q_OBJECT
public:
    explicit TrayController(MainWindow* window, QObject* parent = nullptr);

    void ensureStarted();              // bounded retry/backoff for tray discovery
    bool trayAvailable() const;

    void updateIcon();                 // icon + tooltip reflect effective state
    void rebuildMenu();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void retryTray();

private:
    void createFallbackWindow();
    void updateFallback();

    MainWindow* m_window = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    bool m_available = false;
    int m_retries = 0;
    QPointer<QWidget> m_fallback;
    QLabel* m_fallbackStatus = nullptr;
    QPushButton* m_fallbackToggle = nullptr;
};

} // namespace kaeru
