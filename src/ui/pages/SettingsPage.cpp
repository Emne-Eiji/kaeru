// Kaeru — Settings page (initial release: Keep Awake only)
#include "SettingsPage.h"
#include "app/Paths.h"
#include "app/Settings.h"
#include "platform/AutostartBackend.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace kaeru {

SettingsPage::SettingsPage(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setAccessibleName(tr("Settings"));
    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    // ---- Startup (opt-in, separate switches; spec §7) ----
    QWidget* startupCard = sectionCard(tr("Startup"));
    auto* cardLayout = qobject_cast<QVBoxLayout*>(startupCard->layout());
    m_startAfterLogin = new QCheckBox(tr("Start Kaeru after login"), startupCard);
    connect(m_startAfterLogin, &QCheckBox::toggled, this, &SettingsPage::onStartupChanged);
    cardLayout->addWidget(m_startAfterLogin);
    m_keepAwakeOnLogin = new QCheckBox(tr("Start Keep Awake when Kaeru starts"), startupCard);
    connect(m_keepAwakeOnLogin, &QCheckBox::toggled, this, &SettingsPage::onStartupChanged);
    cardLayout->addWidget(m_keepAwakeOnLogin);
    auto* startupNote = new QLabel(
        tr("Kaeru starts hidden in the system tray. Autostart only creates Kaeru's own per-user entry."),
        startupCard);
    startupNote->setObjectName(QStringLiteral("Subtle"));
    startupNote->setWordWrap(true);
    cardLayout->addWidget(startupNote);
    root->addWidget(startupCard);

    // ---- Notifications ----
    QWidget* notifCard = sectionCard(tr("Notifications"));
    cardLayout = qobject_cast<QVBoxLayout*>(notifCard->layout());
    m_notifyEnabled = new QCheckBox(tr("Enable desktop notifications"), notifCard);
    connect(m_notifyEnabled, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_notifyEnabled);
    m_notifySound = new QCheckBox(tr("Play a sound with notifications"), notifCard);
    connect(m_notifySound, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_notifySound);
    m_keepAwakeNotify = new QCheckBox(tr("Notify when a timed Keep Awake ends"), notifCard);
    connect(m_keepAwakeNotify, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_keepAwakeNotify);
    root->addWidget(notifCard);

    // ---- Appearance ----
    QWidget* themeCard = sectionCard(tr("Appearance"));
    cardLayout = qobject_cast<QVBoxLayout*>(themeCard->layout());
    m_themeBox = new QComboBox(themeCard);
    m_themeBox->addItem(tr("System"));
    m_themeBox->addItem(tr("Light"));
    m_themeBox->addItem(tr("Dark"));
    m_themeBox->setAccessibleName(tr("Theme"));
    connect(m_themeBox, &QComboBox::currentIndexChanged, this, &SettingsPage::onThemeChanged);
    cardLayout->addWidget(m_themeBox);
    root->addWidget(themeCard);

    // ---- Data ----
    QWidget* dataCard = sectionCard(tr("Data"));
    cardLayout = qobject_cast<QVBoxLayout*>(dataCard->layout());
    m_configPath = new QLabel(dataCard);
    m_configPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_configPath->setWordWrap(true);
    cardLayout->addWidget(m_configPath);
    m_dataPath = new QLabel(dataCard);
    m_dataPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_dataPath->setWordWrap(true);
    cardLayout->addWidget(m_dataPath);
    auto* deleteRow = new QHBoxLayout();
    m_deleteButton = new QPushButton(tr("Delete all Kaeru data…"), dataCard);
    m_deleteButton->setObjectName(QStringLiteral("DangerButton"));
    m_deleteButton->setAccessibleName(tr("Delete all Kaeru data (asks twice)"));
    connect(m_deleteButton, &QPushButton::clicked, this, &SettingsPage::onDeleteData);
    deleteRow->addWidget(m_deleteButton);
    deleteRow->addStretch(1);
    cardLayout->addLayout(deleteRow);
    root->addWidget(dataCard);

    // About is now a separate navigation destination (WP-09); Settings keeps
    // only configuration. Quit remains in the tray menu.
    root->addStretch(1);
    scroll->setWidget(content);
    outerLayout->addWidget(scroll);

    refresh();
}

QWidget* SettingsPage::sectionCard(const QString& title)
{
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("Card"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);
    auto* header = new QLabel(title, frame);
    header->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(header);
    return frame;
}

void SettingsPage::refresh()
{
    loadFromSettings();
    refreshStoragePaths();
}

void SettingsPage::loadFromSettings()
{
    Settings* s = m_app->settings();
    // Loading the view must NEVER write settings: every setter below fires a
    // change signal (toggled / currentIndexChanged) whose slot persists widget
    // state — block all of them while seeding.
    {
        QSignalBlocker b1(m_startAfterLogin);
        QSignalBlocker b2(m_keepAwakeOnLogin);
        QSignalBlocker b3(m_notifyEnabled);
        QSignalBlocker b4(m_notifySound);
        QSignalBlocker b5(m_keepAwakeNotify);
        QSignalBlocker b6(m_themeBox);
        m_startAfterLogin->setChecked(s->startAfterLogin());
        m_keepAwakeOnLogin->setChecked(s->keepAwakeOnLogin());
        m_keepAwakeOnLogin->setEnabled(s->startAfterLogin());
        m_notifyEnabled->setChecked(s->notificationsEnabled());
        m_notifySound->setChecked(s->soundEnabled());
        m_keepAwakeNotify->setChecked(s->keepAwakeNotifications());
        const QString theme = s->theme();
        m_themeBox->setCurrentIndex(theme == QLatin1String("light") ? 1
                                  : theme == QLatin1String("dark")  ? 2
                                                                    : 0);
    }
}

void SettingsPage::refreshStoragePaths()
{
    m_configPath->setText(tr("Configuration: %1").arg(Paths::configDir()));
    m_dataPath->setText(tr("Data (database, backups): %1").arg(Paths::dataDir()));
}

void SettingsPage::onStartupChanged()
{
    Settings* s = m_app->settings();
    const bool afterLogin = m_startAfterLogin->isChecked();
    const bool awakeOnLogin = m_keepAwakeOnLogin->isChecked();
    s->setStartAfterLogin(afterLogin);
    s->setKeepAwakeOnLogin(awakeOnLogin);
    m_keepAwakeOnLogin->setEnabled(afterLogin);

    // Autostart only manages Kaeru's own XDG entry (opt-in; never lingering).
    // Always background the startup so login does not pop a window.
    if (afterLogin) {
        QString args = QStringLiteral("--background");
        if (awakeOnLogin)
            args += QStringLiteral(" --awake-on-login");
        m_app->autostart()->enable(args);
    } else {
        m_app->autostart()->disable();
    }
}

void SettingsPage::onNotificationsChanged()
{
    Settings* s = m_app->settings();
    s->setNotificationsEnabled(m_notifyEnabled->isChecked());
    s->setSoundEnabled(m_notifySound->isChecked());
    s->setKeepAwakeNotifications(m_keepAwakeNotify->isChecked());
}

void SettingsPage::onThemeChanged(int index)
{
    const QString theme = index == 1 ? QStringLiteral("light")
                        : index == 2 ? QStringLiteral("dark")
                                     : QStringLiteral("system");
    m_app->settings()->setTheme(theme);
}

void SettingsPage::onDeleteData()
{
    // Explicit double confirmation; states exactly what is deleted (uiux §6).
    const int first = QMessageBox::warning(this, tr("Delete all Kaeru data?"),
        tr("This deletes Kaeru's own data only:\n"
           "• stored application data\n"
           "• settings\n"
           "• cache files\n\n"
           "Deleted data cannot be recovered.\n"
           "Kaeru will stop before deleting. Continue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (first != QMessageBox::Yes)
        return;
    const int second = QMessageBox::warning(this, tr("Confirm deletion"),
        tr("Final confirmation: everything under\n%1\n%2\nwill be deleted permanently.")
            .arg(Paths::configDir(), Paths::dataDir()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (second != QMessageBox::Yes)
        return;

    m_app->prepareQuit();
    QDir(Paths::dataDir()).removeRecursively();
    QDir(Paths::configDir()).removeRecursively();
    QDir(Paths::cacheDir()).removeRecursively();
    refreshStoragePaths();
    QMessageBox::information(this, tr("Kaeru data deleted"),
        tr("Kaeru data was deleted. Restart Kaeru to start fresh."));
}

} // namespace kaeru
