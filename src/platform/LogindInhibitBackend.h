// Kaeru — systemd-logind inhibition backend (spec §6.1, ADR-002)
// Manager.Inhibit(what, who, why, mode) → pipe_fd; lock lives while the fd is
// open. FDs are marked close-on-exec so children never inherit inhibitors.
#pragma once
#include "core/InhibitionBackend.h"

#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>

namespace kaeru {

class LogindInhibitBackend final : public InhibitionBackend {
    Q_OBJECT
public:
    explicit LogindInhibitBackend(QObject* parent = nullptr);

    QString backendId() const override { return QStringLiteral("logind"); }
    Capabilities supportedCapabilities() const override;
    QString availabilityDetail() const override { return m_detail; }
    bool serviceReachable() const override { return m_reachable; }

    QString acquire(Capabilities capabilities, const QString& reason) override;
    void release(const QString& bindingId) override;

private slots:
    void onServiceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner);

private:
    QString whatFor(Capabilities caps) const;
    void markLost(const QString& reason);

    QDBusServiceWatcher* m_watcher = nullptr;
    // bindingId → QDBusUnixFileDescriptor owned by us (released on erase)
    QHash<QString, QDBusUnixFileDescriptor> m_fds;
    QHash<QString, quint32> m_flags; // bindingId → capability bits
    bool m_reachable = false;
    QString m_detail;
};

} // namespace kaeru
