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
    const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> reply =
        bus.interface()->registerService(
            QStringLiteral(KAERU_APP_ID),
            QDBusConnectionInterface::DontQueueService,
            QDBusConnectionInterface::AllowReplacement);
    m_primary = reply.isValid();
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
