// Kaeru — Settings page (initial release: Keep Awake only)
// Sections: Startup, Notifications, Appearance, Data, About.
#pragma once
#include "app/Application.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace kaeru {

class SettingsPage final : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(Application* app, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onStartupChanged();
    void onNotificationsChanged();
    void onThemeChanged(int index);
    void onDeleteData();

private:
    void loadFromSettings();
    QWidget* sectionCard(const QString& title);
    void refreshStoragePaths();

    Application* m_app = nullptr;

    QCheckBox* m_startAfterLogin = nullptr;
    QCheckBox* m_keepAwakeOnLogin = nullptr;
    QCheckBox* m_notifyEnabled = nullptr;
    QCheckBox* m_notifySound = nullptr;
    QCheckBox* m_keepAwakeNotify = nullptr;
    QComboBox* m_themeBox = nullptr;
    QLabel* m_configPath = nullptr;
    QLabel* m_dataPath = nullptr;
    QPushButton* m_deleteButton = nullptr;
};

} // namespace kaeru
