// Kaeru — application state: module switches, Pause All, suppression (spec §7)
#pragma once
#include "Types.h"

#include <QObject>

namespace kaeru {

class AppState final : public QObject {
    Q_OBJECT
public:
    explicit AppState(QObject* parent = nullptr);

    bool isModuleEnabled(Module m) const;
    void setModuleEnabled(Module m, bool enabled);

    bool isPausedAll() const { return m_pausedAll; }
    // Pause All: stops active timers/recording/inhibition and suspends
    // automatic activation. Leaving Pause All returns to safe inactive state.
    void setPausedAll(bool paused);

    QVariantMap toJson() const;

signals:
    void changed();
    void moduleChanged(Module m, bool enabled);
    void pausedAllChanged(bool paused);

private:
    bool m_keepAwake = true;
    bool m_focus = true;
    bool m_workLog = true;
    bool m_pausedAll = false;
};

} // namespace kaeru
