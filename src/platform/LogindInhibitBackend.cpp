#include "LogindInhibitBackend.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QUuid>
#include <fcntl.h>
#include <unistd.h>

namespace kaeru {

static constexpr const char* kLogindService = "org.freedesktop.login1";
static constexpr const char* kLogindPath = "/org/freedesktop/login1";
static constexpr const char* kLogindManager = "org.freedesktop.login1.Manager";

LogindInhibitBackend::LogindInhibitBackend(QObject* parent)
    : InhibitionBackend(parent)
    , m_watcher(new QDBusServiceWatcher(QString::fromUtf8(kLogindService),
          QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
{
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged,
        this, &LogindInhibitBackend::onServiceOwnerChanged);

    QDBusInterface manager(QString::fromUtf8(kLogindService), QString::fromUtf8(kLogindPath),
        QString::fromUtf8(kLogindManager), QDBusConnection::systemBus(), this);
    m_reachable = manager.isValid();
    m_detail = m_reachable
        ? QStringLiteral("org.freedesktop.login1.Manager available")
        : QStringLiteral("logind service unreachable");
}

Capabilities LogindInhibitBackend::supportedCapabilities() const
{
    if (!m_reachable)
        return {};
    // logind `sleep` prevents automatic suspend; logind `idle` affects only
    // logind's own idle-suspend logic, NOT screen blanking — we therefore do
    // NOT claim SessionIdle from logind (honest reporting, ADR-002).
    return Capability::SystemSleep;
}

QString LogindInhibitBackend::whatFor(Capabilities caps) const
{
    if (caps.testFlag(Capability::SystemSleep))
        return QStringLiteral("sleep");
    return QString();
}

QString LogindInhibitBackend::acquire(Capabilities capabilities, const QString& reason)
{
    const QString bindingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!m_reachable) {
        emit bindingFailed(bindingId, false, m_detail);
        return bindingId;
    }
    const QString what = whatFor(capabilities);
    if (what.isEmpty()) {
        emit bindingFailed(bindingId, false, QStringLiteral("capability not supported by logind"));
        return bindingId;
    }

    QDBusInterface manager(QString::fromUtf8(kLogindService), QString::fromUtf8(kLogindPath),
        QString::fromUtf8(kLogindManager), QDBusConnection::systemBus(), this);
    QDBusPendingReply<QDBusUnixFileDescriptor> reply =
        manager.asyncCall(QStringLiteral("Inhibit"), what, QStringLiteral("Kaeru"),
            reason.isEmpty() ? QStringLiteral("keep awake requested by user") : reason.left(120),
            QStringLiteral("block"));

    auto* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, bindingId, capabilities](QDBusPendingCallWatcher* call) {
        QDBusPendingReply<QDBusUnixFileDescriptor> reply = *call;
        call->deleteLater();
        if (reply.isError()) {
            const QDBusError err = reply.error();
            const bool denied = err.type() == QDBusError::AccessDenied
                || err.name().contains(QStringLiteral("AccessDenied"));
            emit bindingFailed(bindingId, denied, QStringLiteral("logind Inhibit failed: %1").arg(err.message()));
            return;
        }
        QDBusUnixFileDescriptor fd = reply.value();
        if (!fd.isValid()) {
            emit bindingFailed(bindingId, false, QStringLiteral("logind returned invalid fd"));
            return;
        }
        // Ensure children never inherit the inhibitor lock (spec §6.1).
        const int rawFd = fd.fileDescriptor();
        fcntl(rawFd, F_SETFD, FD_CLOEXEC);
        m_fds.insert(bindingId, fd);
        m_flags.insert(bindingId, quint32(capabilities));
        emit bindingActivated(bindingId);
    });
    return bindingId;
}

void LogindInhibitBackend::release(const QString& bindingId)
{
    // Dropping the descriptor closes the fd and releases the lock.
    m_fds.remove(bindingId);
    m_flags.remove(bindingId);
}

void LogindInhibitBackend::onServiceOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner)
{
    Q_UNUSED(oldOwner)
    if (name != QString::fromUtf8(kLogindService))
        return;
    m_reachable = !newOwner.isEmpty();
    if (!m_reachable) {
        markLost(QStringLiteral("logind service stopped"));
        m_detail = QStringLiteral("logind service unreachable");
    } else {
        m_detail = QStringLiteral("logind service restarted");
    }
}

void LogindInhibitBackend::markLost(const QString& reason)
{
    const QStringList ids = m_fds.keys();
    for (const QString& id : ids) {
        m_fds.remove(id);
        m_flags.remove(id);
        emit bindingLost(id, reason);
    }
}

} // namespace kaeru
