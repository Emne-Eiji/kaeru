// Kaeru — capability registry (diagnostics, spec §6.1)
#pragma once
#include "Types.h"

#include <QVariantMap>
#include <QVector>

namespace kaeru {

// Describes one platform capability and how well it is supported.
struct CapabilityInfo {
    QString id;          // e.g. "system-sleep-inhibition"
    QString title;
    QString status;      // available | unavailable | degraded | denied | error
    QString backend;
    QString detail;      // plain-language explanation
};

class CapabilityRegistry final : public QObject {
    Q_OBJECT
public:
    explicit CapabilityRegistry(QObject* parent = nullptr);

    void update(const QString& id, const CapabilityInfo& info);
    CapabilityInfo get(const QString& id) const;
    QVector<CapabilityInfo> all() const;
    QVariantMap toJson() const;

signals:
    void updated(const QString& id);

private:
    QHash<QString, CapabilityInfo> m_info;
};

} // namespace kaeru
