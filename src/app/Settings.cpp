#include "Settings.h"
#include "Paths.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <cstdio>

namespace kaeru {

Settings::Settings(QObject* parent)
    : QObject(parent)
    , m_settings(Paths::settingsFile(), QSettings::IniFormat)
{
    QDir().mkpath(Paths::configDir());
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_SETTINGS")) {
        std::fprintf(stderr, "[kaeru] settings file: %s status=%d quietHours=%d\n",
                     qPrintable(Paths::settingsFile()), int(m_settings.status()),
                     m_settings.value(QStringLiteral("notifications/quietHoursEnabled"), false).toBool() ? 1 : 0);
    }
}

bool Settings::moduleEnabled(Module m) const
{
    switch (m) {
    case Module::KeepAwake: return m_settings.value(QStringLiteral("modules/keepAwake"), true).toBool();
    case Module::Focus: return m_settings.value(QStringLiteral("modules/focus"), true).toBool();
    case Module::WorkLog: return m_settings.value(QStringLiteral("modules/workLog"), true).toBool();
    }
    return false;
}

void Settings::setModuleEnabled(Module m, bool on)
{
    const QString key = (m == Module::KeepAwake) ? QStringLiteral("modules/keepAwake")
        : (m == Module::Focus) ? QStringLiteral("modules/focus")
        : QStringLiteral("modules/workLog");
    m_settings.setValue(key, on);
    m_settings.sync();
    emit changed();
}

bool Settings::startAfterLogin() const { return m_settings.value(QStringLiteral("startup/startAfterLogin"), false).toBool(); }
void Settings::setStartAfterLogin(bool on) { m_settings.setValue(QStringLiteral("startup/startAfterLogin"), on); m_settings.sync(); emit changed(); }
bool Settings::keepAwakeOnLogin() const { return m_settings.value(QStringLiteral("startup/keepAwakeOnLogin"), false).toBool(); }
void Settings::setKeepAwakeOnLogin(bool on) { m_settings.setValue(QStringLiteral("startup/keepAwakeOnLogin"), on); m_settings.sync(); emit changed(); }

bool Settings::notificationsEnabled() const { return m_settings.value(QStringLiteral("notifications/enabled"), true).toBool(); }
void Settings::setNotificationsEnabled(bool on) { m_settings.setValue(QStringLiteral("notifications/enabled"), on); m_settings.sync(); emit changed(); }
bool Settings::soundEnabled() const { return m_settings.value(QStringLiteral("notifications/sound"), false).toBool(); }
void Settings::setSoundEnabled(bool on) { m_settings.setValue(QStringLiteral("notifications/sound"), on); m_settings.sync(); emit changed(); }

QString Settings::theme() const { return m_settings.value(QStringLiteral("appearance/theme"), QStringLiteral("system")).toString(); }
void Settings::setTheme(const QString& theme) { m_settings.setValue(QStringLiteral("appearance/theme"), theme); m_settings.sync(); emit changed(); }

QString Settings::lastAwakeDurationToken() const
{
    return m_settings.value(QStringLiteral("keepAwake/lastDuration"),
                            QStringLiteral("1h")).toString();
}

void Settings::setLastAwakeDurationToken(const QString& token)
{
    m_settings.setValue(QStringLiteral("keepAwake/lastDuration"), token);
    m_settings.sync();
}

bool Settings::windowGeometry(QRect* rect) const
{
    if (!rect)
        return false;
    if (!m_settings.contains(QStringLiteral("window/width"))
        || !m_settings.contains(QStringLiteral("window/height")))
        return false;
    const int width = m_settings.value(QStringLiteral("window/width"), 0).toInt();
    const int height = m_settings.value(QStringLiteral("window/height"), 0).toInt();
    if (width <= 0 || height <= 0)
        return false;
    *rect = QRect(m_settings.value(QStringLiteral("window/x"), 0).toInt(),
                  m_settings.value(QStringLiteral("window/y"), 0).toInt(),
                  width, height);
    return true;
}

void Settings::setWindowGeometry(const QRect& rect)
{
    if (!rect.isValid() || rect.width() <= 0 || rect.height() <= 0)
        return;
    m_settings.setValue(QStringLiteral("window/x"), rect.x());
    m_settings.setValue(QStringLiteral("window/y"), rect.y());
    m_settings.setValue(QStringLiteral("window/width"), rect.width());
    m_settings.setValue(QStringLiteral("window/height"), rect.height());
    m_settings.sync();
}

bool Settings::pauseWorkWhenLocked() const { return m_settings.value(QStringLiteral("worklog/pauseWhenLocked"), true).toBool(); }
void Settings::setPauseWorkWhenLocked(bool on) { m_settings.setValue(QStringLiteral("worklog/pauseWhenLocked"), on); m_settings.sync(); emit changed(); }
QString Settings::defaultProject() const { return m_settings.value(QStringLiteral("worklog/defaultProject"), QStringLiteral("General")).toString(); }
void Settings::setDefaultProject(const QString& name) { m_settings.setValue(QStringLiteral("worklog/defaultProject"), name); m_settings.sync(); emit changed(); }

int Settings::focusWorkMinutes() const { return m_settings.value(QStringLiteral("focus/workMinutes"), 25).toInt(); }
void Settings::setFocusWorkMinutes(int minutes) { m_settings.setValue(QStringLiteral("focus/workMinutes"), minutes); m_settings.sync(); emit changed(); }
int Settings::focusShortBreakMinutes() const { return m_settings.value(QStringLiteral("focus/shortBreakMinutes"), 5).toInt(); }
void Settings::setFocusShortBreakMinutes(int minutes) { m_settings.setValue(QStringLiteral("focus/shortBreakMinutes"), minutes); m_settings.sync(); emit changed(); }
int Settings::focusLongBreakMinutes() const { return m_settings.value(QStringLiteral("focus/longBreakMinutes"), 15).toInt(); }
void Settings::setFocusLongBreakMinutes(int minutes) { m_settings.setValue(QStringLiteral("focus/longBreakMinutes"), minutes); m_settings.sync(); emit changed(); }
int Settings::focusLongBreakEvery() const { return m_settings.value(QStringLiteral("focus/longBreakEvery"), 4).toInt(); }
void Settings::setFocusLongBreakEvery(int n) { m_settings.setValue(QStringLiteral("focus/longBreakEvery"), n); m_settings.sync(); emit changed(); }
bool Settings::focusAutoNext() const { return m_settings.value(QStringLiteral("focus/autoNext"), false).toBool(); }
void Settings::setFocusAutoNext(bool on) { m_settings.setValue(QStringLiteral("focus/autoNext"), on); m_settings.sync(); emit changed(); }
bool Settings::focusKeepAwakeDuringWork() const { return m_settings.value(QStringLiteral("focus/keepAwakeDuringWork"), false).toBool(); }
void Settings::setFocusKeepAwakeDuringWork(bool on) { m_settings.setValue(QStringLiteral("focus/keepAwakeDuringWork"), on); m_settings.sync(); emit changed(); }
bool Settings::focusKeepAwakeDuringBreaks() const { return m_settings.value(QStringLiteral("focus/keepAwakeDuringBreaks"), false).toBool(); }
void Settings::setFocusKeepAwakeDuringBreaks(bool on) { m_settings.setValue(QStringLiteral("focus/keepAwakeDuringBreaks"), on); m_settings.sync(); emit changed(); }
bool Settings::focusRecordInWorkLog() const { return m_settings.value(QStringLiteral("focus/recordInWorkLog"), false).toBool(); }
void Settings::setFocusRecordInWorkLog(bool on) { m_settings.setValue(QStringLiteral("focus/recordInWorkLog"), on); m_settings.sync(); emit changed(); }

bool Settings::quietHoursEnabled() const { return m_settings.value(QStringLiteral("notifications/quietHoursEnabled"), false).toBool(); }
void Settings::setQuietHoursEnabled(bool on)
{
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_SETTINGS"))
        std::fprintf(stderr, "[kaeru] setQuietHoursEnabled(%d)\n", on ? 1 : 0);
    m_settings.setValue(QStringLiteral("notifications/quietHoursEnabled"), on);
    m_settings.sync();
    emit changed();
}
QString Settings::quietFrom() const { return m_settings.value(QStringLiteral("notifications/quietFrom"), QStringLiteral("22:00")).toString(); }
void Settings::setQuietFrom(const QString& v) { m_settings.setValue(QStringLiteral("notifications/quietFrom"), v); m_settings.sync(); emit changed(); }
QString Settings::quietTo() const { return m_settings.value(QStringLiteral("notifications/quietTo"), QStringLiteral("07:00")).toString(); }
void Settings::setQuietTo(const QString& v)
{
    if (qEnvironmentVariableIsSet("KAERU_DEBUG_SETTINGS"))
        std::fprintf(stderr, "[kaeru] setQuietTo(%s)\n", qPrintable(v));
    m_settings.setValue(QStringLiteral("notifications/quietTo"), v);
    m_settings.sync();
    emit changed();
}

bool Settings::keepAwakeNotifications() const { return m_settings.value(QStringLiteral("keepAwake/notifyOnExpiry"), true).toBool(); }
void Settings::setKeepAwakeNotifications(bool on) { m_settings.setValue(QStringLiteral("keepAwake/notifyOnExpiry"), on); m_settings.sync(); emit changed(); }

void Settings::sync()
{
    m_settings.sync();
    // User-only permissions for private data (spec §10 security).
    QFile::setPermissions(Paths::settingsFile(),
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

} // namespace kaeru
