#include "Notifications.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QStringList>
#include <QVariantList>

namespace kaeru {

Notifications::Notifications(QObject* parent)
    : QObject(parent)
{
    m_available = QDBusConnection::sessionBus().interface()->isServiceRegistered(
        QStringLiteral("org.freedesktop.Notifications"));
}

bool Notifications::available() const
{
    return m_available;
}

void Notifications::notify(const QString& title, const QString& body, const QString& iconName,
                           const QString& soundName)
{
    if (!m_available)
        return;
    QDBusInterface notify(QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QDBusConnection::sessionBus(), this);
    if (!notify.isValid())
        return;

    QVariantMap hints;
    if (!soundName.isEmpty())
        hints.insert(QStringLiteral("sound-name"), soundName);

    QVariantList args;
    args << QStringLiteral("io.github.kaeru.Kaeru")       // app_name
         << uint(0)                                       // replaces_id
         << (iconName.isEmpty() ? QStringLiteral("io.github.kaeru.Kaeru") : iconName)
         << title << body
         << QStringList()                                 // actions
         << hints
         << int(6000);                                    // expire_timeout ms
    notify.asyncCallWithArgumentList(QStringLiteral("Notify"), args);
}

} // namespace kaeru
