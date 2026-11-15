// Kaeru — settings repository (QSettings-backed, $XDG_CONFIG_HOME/kaeru)
#pragma once
#include "core/Types.h"

#include <QObject>
#include <QSettings>

namespace kaeru {

class Settings final : public QObject {
    Q_OBJECT
public:
    explicit Settings(QObject* parent = nullptr);

    // Module switches
    bool moduleEnabled(Module m) const;
    void setModuleEnabled(Module m, bool on);

    // Startup (spec §7: opt-in, off by default, separate switches)
    bool startAfterLogin() const;
    void setStartAfterLogin(bool on);
    bool keepAwakeOnLogin() const;
    void setKeepAwakeOnLogin(bool on);

    // Notifications
    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool on);
    bool soundEnabled() const;
    void setSoundEnabled(bool on);

    // Appearance
    QString theme() const; // "system" | "light" | "dark"
    void setTheme(const QString& theme);

    // Work log defaults
    bool pauseWorkWhenLocked() const;
    void setPauseWorkWhenLocked(bool on);
    QString defaultProject() const;
    void setDefaultProject(const QString& name);

    // Focus defaults (spec §6.3)
    int focusWorkMinutes() const;
    void setFocusWorkMinutes(int minutes);
    int focusShortBreakMinutes() const;
    void setFocusShortBreakMinutes(int minutes);
    int focusLongBreakMinutes() const;
    void setFocusLongBreakMinutes(int minutes);
    int focusLongBreakEvery() const;
    void setFocusLongBreakEvery(int n);
    bool focusAutoNext() const;
    void setFocusAutoNext(bool on);
    bool focusKeepAwakeDuringWork() const;
    void setFocusKeepAwakeDuringWork(bool on);
    bool focusKeepAwakeDuringBreaks() const;
    void setFocusKeepAwakeDuringBreaks(bool on);
    bool focusRecordInWorkLog() const;
    void setFocusRecordInWorkLog(bool on);

    // Quiet hours
    bool quietHoursEnabled() const;
    void setQuietHoursEnabled(bool on);
    QString quietFrom() const; // "HH:mm"
    void setQuietFrom(const QString& v);
    QString quietTo() const;
    void setQuietTo(const QString& v);

    // Diagnostics/keep-awake history flags
    bool keepAwakeNotifications() const;
    void setKeepAwakeNotifications(bool on);

    void sync();

signals:
    void changed();

private:
    QSettings m_settings;
};

} // namespace kaeru
