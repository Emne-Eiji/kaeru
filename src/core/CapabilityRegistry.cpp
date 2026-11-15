#include "CapabilityRegistry.h"

namespace kaeru {

CapabilityRegistry::CapabilityRegistry(QObject* parent)
    : QObject(parent)
{
}

void CapabilityRegistry::update(const QString& id, const CapabilityInfo& info)
{
    m_info.insert(id, info);
    emit updated(id);
}

CapabilityInfo CapabilityRegistry::get(const QString& id) const
{
    return m_info.value(id);
}

QVector<CapabilityInfo> CapabilityRegistry::all() const
{
    return m_info.values().toVector();
}

QVariantMap CapabilityRegistry::toJson() const
{
    QVariantMap out;
    for (auto it = m_info.constBegin(); it != m_info.constEnd(); ++it) {
        out.insert(it.key(), QVariantMap {
            { QStringLiteral("title"), it.value().title },
            { QStringLiteral("status"), it.value().status },
            { QStringLiteral("backend"), it.value().backend },
            { QStringLiteral("detail"), it.value().detail },
        });
    }
    return out;
}

} // namespace kaeru
