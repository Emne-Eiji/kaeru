#include "SingleInstance.h"
#include "app/ControlService.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

namespace kaeru {

SingleInstance::SingleInstance(QObject* parent)
    : QObject(parent)
{
}

bool SingleInstance::tryAcquire()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    // No AllowReplacement: a second launch must forward to the running
    // instance and exit, never take over (spec §7 single instance). D-Bus
    // releases names of dead connections automatically, so replacement is
    // unnecessary even after a crash.
    const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> reply =
        bus.interface()->registerService(
            QStringLiteral(KAERU_APP_ID),
            QDBusConnectionInterface::DontQueueService);
    // A name clash is NOT a D-Bus error: it comes back as a valid reply with
    // ServiceNotRegistered — check the value, not just validity.
    m_primary = reply.isValid()
        && reply.value() == QDBusConnectionInterface::ServiceRegistered;
    return m_primary;
}

bool SingleInstance::activateExisting()
{
    QDBusInterface control(QStringLiteral(KAERU_APP_ID),
        QStringLiteral("/io/github/kaeru/Kaeru"),
        QStringLiteral("io.github.kaeru.Kaeru.Control1"),
        QDBusConnection::sessionBus());
    if (!control.isValid())
        return false;
    control.asyncCall(QStringLiteral("ShowWindow"));
    return true;
}

} // namespace kaeru
