#include "AppState.h"

namespace kaeru {

AppState::AppState(QObject* parent)
    : QObject(parent)
{
}

bool AppState::isModuleEnabled(Module m) const
{
    switch (m) {
    case Module::KeepAwake: return m_keepAwake;
    case Module::Focus: return m_focus;
    case Module::WorkLog: return m_workLog;
    }
    return false;
}

void AppState::setModuleEnabled(Module m, bool enabled)
{
    bool* field = nullptr;
    switch (m) {
    case Module::KeepAwake: field = &m_keepAwake; break;
    case Module::Focus: field = &m_focus; break;
    case Module::WorkLog: field = &m_workLog; break;
    }
    if (!field || *field == enabled)
        return;
    *field = enabled;
    emit moduleChanged(m, enabled);
    emit changed();
}

void AppState::setPausedAll(bool paused)
{
    if (m_pausedAll == paused)
        return;
    m_pausedAll = paused;
    emit pausedAllChanged(paused);
    emit changed();
}

QVariantMap AppState::toJson() const
{
    return {
        { QStringLiteral("keepAwake"), m_keepAwake },
        { QStringLiteral("focus"), m_focus },
        { QStringLiteral("workLog"), m_workLog },
        { QStringLiteral("pauseAll"), m_pausedAll },
    };
}

} // namespace kaeru
