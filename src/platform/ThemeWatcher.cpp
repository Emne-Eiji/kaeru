#include "ThemeWatcher.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>

namespace kaeru {

ThemeWatcher::ThemeWatcher(QObject* parent)
    : QObject(parent)
{
    QDBusConnection session = QDBusConnection::sessionBus();
    m_available = session.connect(QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"), this,
        SLOT(onSettingChanged(QString, QString, QDBusVariant)));
    if (m_available)
        readInitial();
}

void ThemeWatcher::readInitial()
{
    QDBusInterface settings(QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QDBusConnection::sessionBus(), this);
    if (!settings.isValid())
        return;
    QDBusPendingReply<QVariantMap> reply =
        settings.asyncCall(QStringLiteral("ReadOne"), QStringLiteral("org.freedesktop.appearance"),
            QStringLiteral("color-scheme"));
    auto* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* call) {
        QDBusPendingReply<QVariantMap> reply = *call;
        call->deleteLater();
        if (reply.isError())
            return;
        const QVariant v = reply.value().value(QStringLiteral("color-scheme"));
        setScheme(v.toUInt());
    });
}

void ThemeWatcher::onSettingChanged(const QString& namespaceName, const QString& key, const QDBusVariant& value)
{
    if (namespaceName != QStringLiteral("org.freedesktop.appearance") || key != QStringLiteral("color-scheme"))
        return;
    setScheme(value.variant().toUInt());
}

void ThemeWatcher::setScheme(quint32 value)
{
    const ColorScheme next = value == 1 ? ColorScheme::Dark : ColorScheme::Light;
    if (next != m_scheme) {
        m_scheme = next;
        emit schemeChanged(m_scheme);
    }
}

} // namespace kaeru
