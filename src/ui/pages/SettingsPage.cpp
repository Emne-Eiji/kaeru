// Kaeru — Settings page (spec §8.2, uiux §7)
#include "SettingsPage.h"
#include "app/Paths.h"
#include "app/Settings.h"
#include "platform/AutostartBackend.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
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

    // ---- Modules ----
    QVBoxLayout* cardLayout = nullptr;
    QWidget* modulesCard = sectionCard(tr("Modules"));
    cardLayout = qobject_cast<QVBoxLayout*>(modulesCard->layout());
    const QStringList moduleNames = { tr("Keep Awake"), tr("Focus"), tr("Work Log") };
    for (int i = 0; i < moduleNames.size(); ++i) {
        auto* cb = new QCheckBox(moduleNames.at(i), modulesCard);
        cb->setAccessibleName(tr("%1 module enabled").arg(moduleNames.at(i)));
        const Module m = static_cast<Module>(i);
        connect(cb, &QCheckBox::toggled, this, [this, m](bool on) {
            m_app->settings()->setModuleEnabled(m, on);
            m_app->appState()->setModuleEnabled(m, on);
        });
        cardLayout->addWidget(cb);
        m_moduleChecks.append(cb);
    }
    root->addWidget(modulesCard);

    // ---- Startup (opt-in, separate switches; spec §7) ----
    QWidget* startupCard = sectionCard(tr("Startup"));
    cardLayout = qobject_cast<QVBoxLayout*>(startupCard->layout());
    m_startAfterLogin = new QCheckBox(tr("Start Kaeru after login"), startupCard);
    connect(m_startAfterLogin, &QCheckBox::toggled, this, &SettingsPage::onStartupChanged);
    cardLayout->addWidget(m_startAfterLogin);
    m_keepAwakeOnLogin = new QCheckBox(tr("Keep awake on login"), startupCard);
    connect(m_keepAwakeOnLogin, &QCheckBox::toggled, this, &SettingsPage::onStartupChanged);
    cardLayout->addWidget(m_keepAwakeOnLogin);
    root->addWidget(startupCard);

    // ---- Notifications ----
    QWidget* notifCard = sectionCard(tr("Notifications"));
    cardLayout = qobject_cast<QVBoxLayout*>(notifCard->layout());
    m_notifyEnabled = new QCheckBox(tr("Enable desktop notifications"), notifCard);
    connect(m_notifyEnabled, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_notifyEnabled);
    m_notifySound = new QCheckBox(tr("Play sound on notifications"), notifCard);
    connect(m_notifySound, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_notifySound);
    m_quietHours = new QCheckBox(tr("Respect quiet hours (focus notifications only)"), notifCard);
    connect(m_quietHours, &QCheckBox::toggled, this, &SettingsPage::onNotificationsChanged);
    cardLayout->addWidget(m_quietHours);
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
    cardLayout->addWidget(m_configPath);
    m_dataPath = new QLabel(dataCard);
    m_dataPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
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

    // ---- About ----
    QWidget* aboutCard = sectionCard(tr("About"));
    cardLayout = qobject_cast<QVBoxLayout*>(aboutCard->layout());
    auto* aboutLabel = new QLabel(
        tr("Kaeru %1 — GPL-3.0-or-later.\nLocal-first keep awake, focus and work log. "
           "No telemetry; private fields (tasks/tags/notes) are never logged.")
            .arg(QStringLiteral(KAERU_VERSION)),
        aboutCard);
    aboutLabel->setWordWrap(true);
    cardLayout->addWidget(aboutLabel);
    root->addWidget(aboutCard);

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
    for (int i = 0; i < m_moduleChecks.size(); ++i) {
        const Module m = static_cast<Module>(i);
        m_moduleChecks.at(i)->setChecked(s->moduleEnabled(m));
    }
    m_startAfterLogin->setChecked(s->startAfterLogin());
    m_keepAwakeOnLogin->setChecked(s->keepAwakeOnLogin());
    m_notifyEnabled->setChecked(s->notificationsEnabled());
    m_notifySound->setChecked(s->soundEnabled());
    m_quietHours->setChecked(s->quietHoursEnabled());
    const QString theme = s->theme();
    m_themeBox->setCurrentIndex(theme == QLatin1String("light") ? 1
                              : theme == QLatin1String("dark")  ? 2
                                                                : 0);
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
    // Autostart only manages Kaeru's own XDG entry (opt-in; never lingering).
    if (afterLogin)
        m_app->autostart()->enable(awakeOnLogin ? QStringLiteral("--awake-on-login") : QString());
    else
        m_app->autostart()->disable();
}

void SettingsPage::onNotificationsChanged()
{
    Settings* s = m_app->settings();
    s->setNotificationsEnabled(m_notifyEnabled->isChecked());
    s->setSoundEnabled(m_notifySound->isChecked());
    s->setQuietHoursEnabled(m_quietHours->isChecked());
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
           "• the work log database\n"
           "• settings\n"
           "• cache files\n\n"
           "Recorded sessions and preferences cannot be recovered.\n"
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
