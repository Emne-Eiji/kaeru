// Kaeru — fake backend for unit tests (never counts as native evidence; spec §14)
#pragma once
#include "InhibitionBackend.h"

namespace kaeru {

class FakeInhibitionBackend final : public InhibitionBackend {
    Q_OBJECT
public:
    explicit FakeInhibitionBackend(QObject* parent = nullptr);

    QString backendId() const override { return QStringLiteral("fake"); }
    Capabilities supportedCapabilities() const override { return m_supported; }
    QString availabilityDetail() const override { return QStringLiteral("test backend"); }
    bool serviceReachable() const override { return m_reachable; }

    QString acquire(Capabilities caps, const QString& reason) override;
    void release(const QString& bindingId) override;

    // Test controls
    void setSupported(Capabilities caps) { m_supported = caps; }
    void setReachable(bool r) { m_reachable = r; }
    void failNext(bool permissionDenied, const QString& detail);
    void simulateLost(const QString& bindingId, const QString& reason);
    void simulateActivated(const QString& bindingId);

    QStringList acquiredIds() const { return m_bindings; }

private:
    Capabilities m_supported = Capability::SystemSleep | Capability::SessionIdle;
    bool m_reachable = true;
    int m_failCounter = 0;
    bool m_failDenied = false;
    QString m_failDetail;
    QStringList m_bindings;
};

} // namespace kaeru
