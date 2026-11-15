// Kaeru — XDG Desktop Portal Inhibit backend (spec §6.1, ADR-002)
// Portal Inhibit v3: Inhibit(window, flags, options) → request handle.
// Activation is confirmed by the Response signal on the handle (code 0).
// Release by calling Request.Close() on the handle.
#pragma once
#include "core/InhibitionBackend.h"

#include <QDBusContext>
#include <QDBusServiceWatcher>

namespace kaeru {

class PortalInhibitBackend final : public InhibitionBackend, protected QDBusContext {
    Q_OBJECT
public:
    explicit PortalInhibitBackend(QObject* parent = nullptr);

    QString backendId() const override { return QStringLiteral("portal"); }
    Capabilities supportedCapabilities() const override;
    QString availabilityDetail() const override { return m_detail; }
    bool serviceReachable() const override { return m_reachable; }

    QString acquire(Capabilities capabilities, const QString& reason) override;
    void release(const QString& bindingId) override;

private slots:
    void onResponse(const uint responseCode, const QVariantMap& results);
    void onServiceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner);

private:
    struct PendingBinding {
        QString id;
        QString handlePath;
        quint32 flags = 0;
        Capabilities capabilities;
        QString reason;
    };

    quint32 flagsFor(Capabilities caps) const;
    void subscribeResponse(const QString& handlePath, const QString& bindingId);
    void markLost(const QString& reason);

    QDBusServiceWatcher* m_watcher = nullptr;
    QHash<QString, PendingBinding> m_pending;    // handlePath → pending
    QHash<QString, QString> m_active;            // bindingId → handlePath
    bool m_reachable = false;
    QString m_detail;
    int m_nextToken = 0;
};

} // namespace kaeru
