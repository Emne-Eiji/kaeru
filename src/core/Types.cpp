#include "Types.h"

namespace kaeru {

QVariantMap EffectiveAwakeState::toJson() const
{
    QVariantMap m;
    m[QStringLiteral("status")] = effectiveStatusName(status);
    QStringList req, eff;
    if (requested.testFlag(Capability::SystemSleep)) req << capabilityName(Capability::SystemSleep);
    if (requested.testFlag(Capability::SessionIdle)) req << capabilityName(Capability::SessionIdle);
    if (effective.testFlag(Capability::SystemSleep)) eff << capabilityName(Capability::SystemSleep);
    if (effective.testFlag(Capability::SessionIdle)) eff << capabilityName(Capability::SessionIdle);
    m[QStringLiteral("requested")] = req;
    m[QStringLiteral("effective")] = eff;
    m[QStringLiteral("backend")] = backend;
    m[QStringLiteral("reason")] = reason;
    m[QStringLiteral("remainingMs")] = remainingMs;
    if (!limitation.isEmpty()) m[QStringLiteral("limitation")] = limitation;
    if (!errorDetail.isEmpty()) m[QStringLiteral("error")] = errorDetail;
    return m;
}

} // namespace kaeru
